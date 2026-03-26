# Ambient Firmware with Zephyr

Application for audio recording and bird-song classification on the UCA DKAIoT Ambient sensor node.
> Align with [Arduino bare-metal version](https://bitbucket.org/edge-team-leat/ambient_firmware/src/master/)

This firmware runs on `stm32l496g_bird` and integrates microphone capture, MFCC + CNN inference, GPS, LoRaWAN uplink, SD card storage, environmental sensing, power monitoring, RTC sync, and watchdog.

## 1. Repository Layout

- Application: `app/ambient_firmware_zephyr/app_all_task`
    - Zephyr app entry and scheduler: `src/main.c`
    - Task APIs and shared state: `src/tasks.h`, `src/app_state.c`
- Board definition: `app/ambient_firmware_zephyr/stm32l496g_bird`
    - Base board DTS: `stm32l496g_bird.dts`
    - App overlay: `app_all_task/boards/stm32l496g_bird.overlay`

## 2. Runtime Architecture

### 2.1 Main Scheduling Logic

Main loop behavior (from `src/main.c`):

- Active window: 40s
- Sleep window: 20s (`k_sleep`)
- During active window:
    - Capture + inference scheduling every 10s
    - SD mount check once
    - SD capacity check once
- Event trigger:
    - Every 10 inference labels, queue one GPS -> LoRa uplink event

### 2.2 Core Task Modules

- `microphone_task.c`
    - SAI1_B RX audio capture at 16 kHz, 16-bit
    - Ring-buffer pipeline with separate inference and SD consumers
    - ADC3101 codec initialization on I2C2
- `inference_task.c`
    - CMSIS-DSP MFCC extraction
    - Bird detector (`cnn_detect`) + classifier (`cnn`)
    - Collect 10 labels as one uplink window
- `comm_task.c`
    - Worker thread executes chained `GPS acquire -> LoRa uplink`
- `lorawan_task.c`
    - OTAA join and event-driven uplink
    - Region configured as `EU868` in `prj.conf`
    - Uplink payload length: 29 bytes
- `gps_task.c`
    - NMEA over I2C from GNSS module (addr `0x10`)
    - Parses RMC/GGA and updates location/time
    - Uses I2C1 mutex to avoid bus contention with BME280
- `storage_task.c`
    - FATFS mount on SPI SD slot (`/SD:`)
    - PCM audio chunk write and SD capacity tracking
- `sensor_task.c`
    - BME280 temperature/pressure/humidity sampling
- `ina3221_task.c`
    - INA3221 channel voltage/current monitoring thread
- `rtc_task.c`
    - RTC init/get/set and GPS time synchronization
- `system_task.c`
    - Independent watchdog setup and periodic feeding

## 3. Board and Devicetree Configuration

### 3.1 Enabled Peripherals (Board DTS)

Key nodes from `stm32l496g_bird.dts` and app overlay:

- UART
    - `usart2` enabled for console/shell in app overlay
    - USB CDC disabled in app overlay for debug stability
- Audio
    - `sai1_b` enabled (MCLK/FS/SCK/SD configured)
    - Audio PLL (`pllsai1`) tuned for ~16 kHz capture path
- I2C
    - `i2c1`: BME280 (`0x76`) + GNSS (`0x10`)
    - `i2c2`: INA3221 (`0x40`)
- SPI3
    - `sx1262@0` LoRa radio
    - `sdhc@1` SD card over SPI (`zephyr,sdhc-spi-slot`)
- ADC
    - `adc2`/`adc3` used for voltage-divider channels (solar/battery)
- RTC
    - STM32 RTC enabled with LSI clock source

### 3.2 Notable Hardware Notes

- GPS backup power pin is intentionally skipped in code to avoid pin conflict with SAI1_B FS.
- SD card path is SPI SDHC (not native SDMMC).

## 4. Build and Flash

## Prerequisites

1. Install [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)
2. Follow Zephyr setup: [Zephyr Getting Started](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
3. Connect board power and ST-Link to host

### Build

From workspace root (`/home/jzhang/zephyrproject`):

```bash
source .venv/bin/activate
west build -p always -b stm32l496g_bird app/ambient_firmware_zephyr/app_all_task
```

### Flash

```bash
west flash
```

## 4.1 SD Card DFU Update Flow (New)

SD-card-based DFU is now supported. For first-time deployment, follow this order:

1. Full chip erase (required)
    - Erase the entire MCU flash first to avoid boot issues from old partitions/bootloader remnants.
    - You can do this in STM32CubeProgrammer, or use a flashing flow that supports full erase 
    ```
    STM32CLI=/your/path/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI
    `$STM32CLI -c port=SWD mode=UR -e all`.
    ```

2. Build and flash the official MCUboot first
    - Build MCUboot and flash it to the board before the application firmware.
    - Use an overlay that sets the firmware partition layout to `boot_partition` (bootloader-first layout).

    ```
    / {
        chosen {
            zephyr,console = &usart2;
            zephyr,shell-uart = &usart2;
            zephyr,code-partition = &boot_partition;
        };
    };
    ```
    ```
    ~/zephyrproject/bootloader/mcuboot/boot/zephyr$ west build -p always -b stm32l496g_bird
    ```

3. Build and flash the application firmware
    - After MCUboot is in place, build and flash the `app_all_task` firmware.

4. Future firmware updates (offline via SD card)
    - In the application build output folder, find `zephyr.signed.bin`.
    - Rename it to `update.bin`.
    - Copy `update.bin` to the root directory of the SD card.
    - Insert the SD card into the board and reboot; DFU update will run automatically.

## 5. Serial Log Monitoring

The app overlay routes console to `USART2` at `115200` baud.

Example:

```bash
minicom -D /dev/ttyACM0 -b 115200
```

## 6. LoRaWAN Event Payload (29 Bytes)

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

## 7. Configuration Highlights (`prj.conf`)

- DSP/ML path: `CONFIG_CMSIS_DSP=y`, FPU enabled
- Audio capture: `CONFIG_I2S=y`, `CONFIG_I2S_STM32_SAI=y`
- Connectivity: `CONFIG_LORA=y`, `CONFIG_LORAWAN=y`, `CONFIG_LORAWAN_REGION_EU868=y`
- Storage: FATFS + SDHC + SPI enabled
- Reliability: watchdog, reboot support, thread analyzer enabled

## 8. Typical Runtime Flow

1. Boot and initialize storage/audio/watchdog/comm/RTC/INA worker
2. Try startup GPS acquisition and LoRa join
3. Enter periodic active-window scheduler
4. INA3221 worker runs independently and samples voltage/current every 10s
    - Low-voltage protection will be triggered at once when the voltage is lower than threshold.
5. Capture audio -> MFCC+CNN -> accumulate labels
6. On 10-label event: sample sensors, snapshot inference window, queue comm worker
7. Comm worker performs GPS acquire and LoRa event uplink
8. Repeat with active/sleep duty cycle

## 9. Troubleshooting

- `west flash` fails
    - Check ST-Link connection and ensure STM32CubeProgrammer is installed
- No SD mount
    - Verify SD card insertion and SPI SD slot wiring
- No LoRa uplink
    - Confirm gateway/network settings and OTAA credentials
    - Watch join retry/backoff logs in serial output
- No GPS fix
    - Ensure GNSS antenna and outdoor visibility
    - Check I2C1 device readiness and NMEA logs




