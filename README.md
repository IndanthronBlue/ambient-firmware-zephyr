# AMBIENT Zephyr Firmware Guide

This README covers the project overview, repository structure, build/flash flow, DFU, execution flow, module responsibilities, configurable parameters, and Devicetree configuration.

Current Zephyr version used by this project: `4.3.99`.

---

## 1. Purpose and Feature Overview

`AMBIENT` is a low-power edge sensing firmware based on Zephyr, running on `stm32u5_bird` (with retained compatibility for the `stm32l496g_bird` overlay), providing:

- Audio capture (SAI/I2S + ADC3101)
- MFCC + bird detection/classification inference (CMSIS-DSP + two-stage models)
- GPS acquisition and RTC synchronization
- LoRaWAN OTAA join and event uplink
- SD-card storage and offline DFU (MCUboot)
- Power state machine (ACTIVE / SUSPEND / DEEP_SLEEP)
- Low-battery protection, watchdog, and retained-state persistence

Core goal: continuously run the “capture → infer → communicate → store → sleep” loop under low-power duty-cycling.

---

## 2. Repository Structure

### 2.1 Application Directory

- `app_all_task/`
  - `CMakeLists.txt`: application source entry
  - `Kconfig`: app-level switches (low power, wake sources, GPS local-time offset)
  - `prj.conf`: Zephyr configuration (drivers, middleware, LoRaWAN, MCUboot, etc.)
  - `boards/`
    - `stm32u5_bird.overlay`: main board overlay (currently used for build)
    - `stm32l496g_bird.overlay`: compatibility overlay
  - `dts/bindings/sensor/kionix,kxtj3-1057.yaml`: KXTJ3 binding
  - `src/`
    - `main.c`: boot flow and main loop
    - `power_fsm.c` / `power_ctrl.c`: low-power state machine and power-domain control
    - `microphone_task.c` / `ADC3101.c`: audio capture and codec setup
    - `inference_task.c`: MFCC + model inference
    - `comm_task.c` / `lorawan_task.c` / `gps_task.c`: communication pipeline
    - `storage_task.c` / `dfu_task.c`: SD storage and DFU
    - `sensor_task.c` / `ina3221_task.c` / `rtc_task.c` / `system_task.c`

### 2.2 Related Upper-Level Directories

- `bootloader/mcuboot/`: MCUboot source (must be flashed first on initial deployment)
- `zephyr/boards/st/stm32u5_bird/`: board base DTS (partitions and peripheral nodes)
- `build/`: current build outputs (for example, `build_info.yml`)

---

## 3. Build and Flash

All commands below assume the workspace root: `/home/jzhang/zephyrproject`

### Environment Setup

1. Install Zephyr dependencies and make sure `west` is available
2. Install STM32CubeProgrammer (required by `west flash`)
3. Connect board power and ST-Link
4. Activate the Python virtual environment

```bash
source .venv/bin/activate
```

### First-Time Deployment (Recommended, bootloader-first)

### Step A: Full Chip Erase

Use STM32CubeProgrammer GUI, or CLI:

```bash
STM32CLI=/your/path/STM32CubeProgrammer/bin/STM32_Programmer_CLI
$STM32CLI -c port=SWD mode=UR -e all
```

### Step B: Build and Flash MCUboot First

Before building MCUboot, create the board overlay in the MCUboot project so that bootloader code is linked to `boot_partition`:

File: `bootloader/mcuboot/boot/zephyr/boards/stm32u5_bird.overlay`

```dts
/ {
  chosen {
    zephyr,console = &usart2;
    zephyr,shell-uart = &usart2;
    zephyr,code-partition = &boot_partition;
  };
};
```

> Note: `boot_partition`, `slot0_partition`, and `slot1_partition` are already defined in the board DTS `zephyr/boards/st/stm32u5_bird/stm32u5_bird.dts`. This overlay only selects `boot_partition` for MCUboot linking.

```bash
west build -p always -b stm32u5_bird bootloader/mcuboot/boot/zephyr
west flash
```

> If your target board is `stm32l496g_bird`, replace `-b` with the corresponding board name.

### Step C: Build and Flash Application

```bash
west build -p always -b stm32u5_bird app_all_task
west flash
```

> In `prj.conf`, this project enables `CONFIG_BOOTLOADER_MCUBOOT=y` and `CONFIG_USE_DT_CODE_PARTITION=y`, and the application links to DTS `slot0_partition`.

### Serial Log Monitoring

`stm32u5_bird.overlay` routes console to `USART2` at 115200 baud:

```bash
minicom -D /dev/ttyACM0 -b 115200
```

---

## 3.1 DFU Firmware Update (Offline via SD Card)

Offline SD-card DFU is implemented in `src/dfu_task.c`.

### Trigger Flow

1. Build a new application image (MCUboot-compatible)
2. Put the generated image in SD root as `update.bin`
3. On boot/reboot, `task_dfu_check_and_apply()` runs during startup

### In-App DFU Logic (Automatic)

- Check if `/SD:/update.bin` exists and has valid size
- Erase `slot1_partition`
- Write image to slot1 in chunks (4KB)
- Call `boot_request_upgrade(BOOT_UPGRADE_PERMANENT)`
- Rename `update.bin` to `update.done` (or delete if rename fails)
- Cold reboot; MCUboot switches to new image

### Notes

- For first deployment, MCUboot must already be flashed
- Update image size must not exceed `slot1_partition`
- Current code requests **PERMANENT** upgrade (not test/revert mode)

---

## 4. Detailed Execution Flow

## 4.1 Boot Stage (`main.c`)

1. Lock PM boot guard and read reset cause
2. Print firmware version, feature flags, and MCUboot self-check info
3. Execute `boot_write_img_confirmed()` to confirm current image
4. Initialize RTC, communication thread, and optional button/sound wakeup
5. Power-cycle `v_periph` (cold start), then initialize microphone
6. Mount SD and run DFU check/apply
7. Sensor sample, startup LoRaWAN connect attempt, INA3221 worker, watchdog
8. Initialize retained state and check battery voltage once (may re-enter deep sleep if low)
9. Block until GPS legal time sync is ready (continue after timeout)
10. Initialize `power_fsm`, then enter main loop: `tick -> wait_for_event`

## 4.2 Power State Machine (`power_fsm.c`)

- `INITIAL`: transition state
- `SUSPEND`: low-power waiting (periodic or external wake)
- `ACTIVE`: run capture/inference/uplink window
- `DEEP_SLEEP`: deep sleep for low-battery and related cases (configurable simulation)

Key transitions:

- Periodic timeout: `SUSPEND -> ACTIVE`
- button/sound/comm event: `SUSPEND -> ACTIVE`
- low-battery event: any state priority transition `-> DEEP_SLEEP`
- ACTIVE window complete: `ACTIVE -> SUSPEND`

## 4.3 ACTIVE Window Pipeline

1. `task_microphone_capture_once()` triggers one audio capture
2. Audio blocks feed inference pipeline (`task_inference_process_block`)
3. Every 6 windows completes one inference label update
4. When 10 labels are accumulated (`infer_window_ready=true`):
   - Sample environmental sensors
   - Snapshot uplink window
   - Submit comm worker
5. comm worker executes by type:
   - Optional GPS acquire / timeout wait
   - `task_lorawan_send_event_uplink()` sends 29-byte payload
6. On success, clear state and continue state-machine cycle

---

## 5. Module Responsibilities

- `main.c`
  - Boot orchestration, boot self-check, init ordering, main loop

- `power_fsm.c`
  - Low-power state scheduling, unified wake-source and periodic strategy

- `power_ctrl.c`
  - `v_periph`/GPS power-domain control, GPIO parking, peripheral sleep/default pinctrl switching

- `microphone_task.c`
  - SAI1_B 16kHz capture, ring buffer, dual consumer pipeline for inference and SD

- `ADC3101.c`
  - ADC3101 codec register initialization (analog front-end gains, etc.)

- `inference_task.c`
  - MFCC extraction + detector/classifier two-stage inference
  - 10-label window management and uplink snapshot

- `gps_task.c`
  - I2C NMEA reading, RMC/GGA parsing, coordinate update, RTC sync

- `lorawan_task.c`
  - LoRaWAN OTAA join, retry/backoff, uplink payload build and send

- `comm_task.c`
  - Dedicated worker chaining `GPS -> LoRa` to decouple from main state machine

- `storage_task.c`
  - SD mount retries, capacity query, PCM writes, DevEUI/GPS persistence

- `dfu_task.c`
  - Offline SD update to slot1 + MCUboot upgrade request

- `sensor_task.c`
  - BME280 temperature/pressure/humidity sampling

- `ina3221_task.c`
  - INA3221 periodic voltage/current monitoring and low-battery event reporting

- `rtc_task.c`
  - RTC init, default-time seed, GPS time sync, alarm wakeup

- `system_task.c`
  - Watchdog initialization and feeder thread

- `button_task.c` / `sound_wakeup_task.c`
  - External interrupt wake sources

- `retained_state.c`
  - Retention-region persistent state (boot count, GPS sync epoch, recovery counters, etc.)

---

## 6. Configurable Parameters (Purpose / Default / Recommended / Location)

> Notes:
> - “Default” means current value in this repository state.
> - “Recommended” targets current hardware and typical field deployment; tune as needed for power-latency tradeoff.

## 6.1 Kconfig (Application-Level Switches)

| Parameter | Purpose | Default | Recommended | Location |
|---|---|---:|---:|---|
| `CONFIG_APP_LOW_POWER_FSM` | Enable low-power FSM | `y` | `y` | `prj.conf`, `Kconfig` |
| `CONFIG_APP_DEEP_SLEEP_ENABLE` | Allow real deep sleep | `y` | `y` (production) / `n` (debug) | `prj.conf`, `Kconfig` |
| `CONFIG_APP_RETENTION_ENABLE` | Enable retention persistence | `y` | `y` | `prj.conf`, `Kconfig` |
| `CONFIG_APP_DEEP_SLEEP_SIMULATE` | Simulate deep sleep (no real power-off) | `n` | `n` (production) / `y` (bring-up) | `prj.conf`, `Kconfig` |
| `CONFIG_APP_WAKEUP_SRC_BUTTON` | Button wake source | `y` | `y` | `prj.conf`, `Kconfig` |
| `CONFIG_APP_WAKEUP_SRC_PERIODIC` | Periodic wake source | `y` | `y` | `prj.conf`, `Kconfig` |
| `CONFIG_APP_WAKEUP_SRC_COMM` | Communication wake source | `y` | `y` | `prj.conf`, `Kconfig` |
| `CONFIG_APP_WAKEUP_SRC_RTC_ALARM` | RTC alarm wake source | `y` | `y` | `prj.conf`, `Kconfig` |
| `CONFIG_APP_WAKEUP_SRC_SOUND` | Sound IRQ wake source | `n` | `n/y` depending on false-trigger profile | `prj.conf`, `Kconfig` |
| `CONFIG_APP_WAKEUP_SRC_LOW_BAT` | Low-battery deep-sleep trigger | `y` | `y` | `prj.conf`, `Kconfig` |
| `CONFIG_APP_GPS_LOCAL_TIME_OFFSET_HOURS` | GPS UTC to local-time offset | `2` | set by deployment timezone (France: winter 1, summer 2) | `prj.conf`, `Kconfig` |

## 6.2 Zephyr Stack and Middleware Parameters (`prj.conf`)

| Parameter | Purpose | Default | Recommended | Location |
|---|---|---:|---:|---|
| `CONFIG_LORAWAN_REGION_EU868` | LoRaWAN region | `y` | match your gateway/network | `prj.conf` |
| `CONFIG_LORAWAN` / `CONFIG_LORA` | LoRaWAN stack and LoRa PHY | `y/y` | `y/y` | `prj.conf` |
| `CONFIG_BOOTLOADER_MCUBOOT` | Enable MCUboot integration | `y` | `y` | `prj.conf` |
| `CONFIG_USE_DT_CODE_PARTITION` | Use DTS `slot0_partition` for code | `y` | `y` | `prj.conf` |
| `CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE` | Generate unsigned image | `y` | production should use signed-image flow | `prj.conf` |
| `CONFIG_MAIN_STACK_SIZE` | Main thread stack | `8192` | `8192` (increase if features grow) | `prj.conf` |
| `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` | System workqueue stack | `2048` | `2048+` for heavier LoRa control path | `prj.conf` |
| `CONFIG_LOG_DEFAULT_LEVEL` | Log level | `3(INFO)` | `4` debug, `2~3` production | `prj.conf` |
| `CONFIG_LOG_BUFFER_SIZE` | Log buffer size | `16384` | `16384` or larger | `prj.conf` |

## 6.3 Key Application Compile-Time Macros

| Parameter | Purpose | Default | Recommended | Location |
|---|---|---:|---:|---|
| `APP_LOW_BATTERY_MV` | Low-battery threshold | `3500` mV | `3400~3600` mV | `src/tasks.h` |
| `APP_BOOT_GPS_SYNC_TIMEOUT_MS` | Boot blocking GPS-sync timeout | `180000` ms | `60000~180000` | `src/main.c` |
| `APP_BOOT_GPS_RETRY_DELAY_MS` | Boot GPS retry interval | `5000` ms | `3000~10000` | `src/main.c` |
| `ACTIVE_BURST_MS` | ACTIVE window duration | `30000` ms | `20000~60000` | `src/power_fsm.c` |
| `SUSPEND_TO_ACTIVE_PERIOD_MS` | SUSPEND periodic wake interval | `60000` ms | tune by power budget | `src/power_fsm.c` |
| `DEEP_SLEEP_WAKE_PERIOD_MS` | Deep-sleep wake period | `30000` ms | production recommendation: >= `30min` | `src/power_fsm.c` |
| `GPS_UPLINK_WAIT_TIMEOUT_MS` | GPS wait timeout in comm worker | `20000` ms | `10000~60000` | `src/comm_task.c` / `src/power_fsm.c` |
| `LORAWAN_APP_PORT` | Uplink port | `2` | must match cloud decoder | `src/lorawan_task.c` |
| `TX_PAYLOAD_LENGTH` | LoRa uplink payload length | `29` bytes | keep aligned with parser | `src/lorawan_task.c` |
| `MIC_SAMPLE_FREQUENCY` | Audio sample rate | `16000` Hz | `16000` | `src/microphone_task.c` |
| `MIC_BLOCK_COUNT` | I2S slab block count | `6` | `6~12` (RAM dependent) | `src/microphone_task.c` |
| `MIC_RING_BLOCK_COUNT` | Capture ring block count | `30` | `30+` if throughput is tight | `src/microphone_task.c` |
| `INA_MONITOR_PERIOD_MS` | INA monitor period | `30000` ms | `10000~60000` | `src/ina3221_task.c` |
| `WATCHDOG_TIMEOUT_MS` | Watchdog timeout | `24000` ms | `> 2x feed period` | `src/system_task.c` |
| `WATCHDOG_FEED_PERIOD_MS` | Watchdog feed period | `10000` ms | `5000~12000` | `src/system_task.c` |
| `DFU_READ_CHUNK_SIZE` | DFU chunk size | `4096` bytes | `4096` | `src/dfu_task.c` |

## 6.4 Credential Parameters (LoRaWAN OTAA)

| Parameter | Purpose | Default | Recommended | Location |
|---|---|---|---|---|
| `join_eui` | OTAA JoinEUI | all `0x00` (placeholder) | replace with network-issued value | `src/lorawan_task.c` |
| `dev_eui` | Device EUI | HWID-derived + SD persisted | keep deterministic generation + persistence | `src/lorawan_task.c` + `src/storage_task.c` |
| `app_key` | OTAA AppKey/NwkKey | currently hardcoded | production should use secure provisioning (not in repo) | `src/lorawan_task.c` |

---

## 7. Devicetree Configuration

## 7.1 Current Effective Devicetree Sources (`stm32u5`)

According to `build/build_info.yml`, current build uses:

- Base DTS: `zephyr/boards/st/stm32u5_bird/stm32u5_bird.dts`
- App Overlay: `app_all_task/boards/stm32u5_bird.overlay`

## 7.2 Key `chosen` and Partition Setup

In overlay:

- `zephyr,console = &usart2`
- `zephyr,shell-uart = &usart2`
- `zephyr,code-partition = &slot0_partition`
- `zephyr,retained-ram` / `zephyr,retained-mem` bound

In base DTS (`stm32u5_bird.dts`):

- `boot_partition`: `0x00000000` + `0x10000` (64KB)
- `slot0_partition`: `0x00010000` + `0x0E8000` (928KB)
- `slot1_partition`: `0x000F8000` + `0x0E8000` (928KB)
- `scratch_partition`: 64KB
- `storage_partition`: 64KB

## 7.3 Key Peripheral Node Mapping

- `&i2c1`
  - `bme280@76`
  - `cam_m8q@gps(0x10)`
- `&i2c2`
  - `ina3221@40`
- `&spi3`
  - `sx1262@0` (LoRa)
  - `sdhc@1` (SPI SD card)
- `&sai1_b`
  - audio capture interface (enabled in overlay with sleep pinctrl)
- `v_periph` fixed regulator control node
- `gps_on_off` / `gps_vbckp` GPIO power control
- `sw0`, `sound-wakeup-en`, `sound-wakeup-irq` wake-related aliases
- `&rtc` with `wakeup-source`

## 7.4 Extra Overlay Configuration

`stm32u5_bird.overlay` additionally configures:

- retained SRAM region (`retained_region` / `retained_mem` / `app_retention`)
- `sai1_b` `default/sleep` pinctrl states
- `i2c1/i2c2` 100kHz
- INA3221 shunt resistor and enabled-channel overrides
- SPI3 sleep-pin states and clock source refinements
- PLL2 tuning

---

## Appendix A: LoRaWAN Event Payload (29 Bytes)

`lorawan_task.c` builds a fixed 29-byte payload:

- `0..9`: 10 inference labels (uplink snapshot)
- `10..13`: battery/solar voltage (mV)
- `14`: temperature (int8 truncation)
- `15..16`: inference total count (low 16 bits)
- `17..20`: battery/solar current (mA)
- `21`: SD write success flag
- `22..25`: compressed latitude/longitude
- `26..27`: average RMS (compressed)
- `28`: SD usage percent

![Example](https://blue-image-1306182246.cos.ap-shanghai.myqcloud.com/typora/Weixin%20Image_20260316110047_18_266.png)

## Appendix B: Common Commands

```bash
# 1) Build application
west build -p always -b stm32u5_bird app_all_task

# 2) Flash
west flash

# 3) Check current build metadata
cat build/build_info.yml
```

If switching to `stm32l496g_bird`, verify in sync:

- `-b` board target
- matching overlay
- LoRa/GPS/SD/audio pinmux and power-control nodes
