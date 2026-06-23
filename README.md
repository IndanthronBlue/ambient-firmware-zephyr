# AMBIENT Bird Firmware

[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE.txt)
![Zephyr](https://img.shields.io/badge/Zephyr-4.3.99-7c3aed.svg)
![MCUs](https://img.shields.io/badge/MCU-STM32L496%20%7C%20STM32U595R-03234b.svg)
![LoRaWAN](https://img.shields.io/badge/LoRaWAN-OTAA%20EU868-00a3a3.svg)
![DFU](https://img.shields.io/badge/DFU-MCUboot%20signed-orange.svg)

Firmware for the AMBIENT bird-monitoring node based on the
[LEAT-EDGE/ambient](https://github.com/LEAT-EDGE/ambient) open hardware.
This Zephyr firmware has been tested on STM32L496 and STM32U595R MCUs:
STM32L496 uses the Zephyr board `stm32l496g_bird` and Devicetree file
[stm32l496g_bird/stm32l496g_bird.dts](stm32l496g_bird/stm32l496g_bird.dts);
STM32U595R uses the Zephyr board `stm32u5_bird` and Devicetree file
[stm32u5_bird/stm32u5_bird.dts](stm32u5_bird/stm32u5_bird.dts). The firmware
wakes periodically, records audio, stores audio files, runs bird recognition,
sends a LoRaWAN summary, and then returns to a low-power state.

| Item | Value |
| --- | --- |
| Hardware | [LEAT-EDGE/ambient](https://github.com/LEAT-EDGE/ambient) |
| Zephyr version | `4.3.99` |
| Tested MCUs | STM32L496, STM32U595R |
| Zephyr boards | `stm32l496g_bird`, `stm32u5_bird` |
| Application | [app_all_task](app_all_task) |
| Update method | MCUboot signed image, PC flashing, SD-card DFU |
| LoRaWAN mode | OTAA, EU868 by default |

This firmware requires MCUboot signature verification. The initial PC flashing
flow and later SD-card DFU updates must both use an application image signed
with the private key that matches the public key compiled into MCUboot.

## Zephyr Version

The current project uses Zephyr `4.3.99`. You can check it in the workspace:

```bash
cat /home/usr_name/zephyrproject/zephyr/VERSION
```

## Purpose and Feature Overview

The goal of this firmware is low-power bird monitoring on AMBIENT hardware. The
device sleeps most of the time, wakes on schedule, captures audio, runs local
recognition, stores data, and sends a summary over LoRaWAN.

Main features:

- ADC3101 + SAI/I2S audio capture at 16 kHz mono.
- `.wav` audio storage on SD card.
- MFCC plus two-stage bird detection/classification models.
- BME280 and INA3221 environmental and power-state sampling.
- GPS location acquisition and RTC/time synchronization.
- LoRaWAN OTAA join and event uplink.
- MCUboot signed boot and offline SD-card DFU.
- ACTIVE / SUSPEND / DEEP_SLEEP low-power state machine.
- Low-battery protection, watchdog, and retained state.

## Repository Structure

| Path | Description |
| --- | --- |
| [app_all_task](app_all_task) | Main application, including CMake, Kconfig, prj.conf, board overlays, and task sources. |
| [app_all_task/src](app_all_task/src) | Firmware task code for audio, inference, LoRaWAN, GPS, storage, DFU, low power, and related features. |
| [app_all_task/config](app_all_task/config) | Application data files, currently including the inference label list. |
| [app_all_task/boards](app_all_task/boards) | Application overlays and board-specific configuration files. |
| [app_all_task/dts/bindings](app_all_task/dts/bindings) | Custom Devicetree bindings used by this application. |
| [stm32l496g_bird](stm32l496g_bird) | STM32L496 board definition and base Devicetree. |
| [stm32u5_bird](stm32u5_bird) | STM32U595R board definition and base Devicetree. |

## How It Runs

After power-on, the device automatically:

1. Initializes RTC, SD card, microphone, sensors, LoRaWAN, watchdog, and the low-power state machine.
2. Periodically enters an ACTIVE window to capture audio and run bird detection/classification.
3. Stores audio to the SD card as `.wav`.
4. Sends one summary payload over LoRaWAN when the uplink interval is reached.
5. Enters low power or deep sleep when needed, and wakes through RTC or configured wake sources.

Typical serial log monitoring:

```bash
minicom -D /dev/ttyACM0 -b 115200
```

If the serial device is different, check it with:

```bash
ls /dev/ttyACM* /dev/ttyUSB*
```

## Configurable Parameters

Most configuration is in [app_all_task/prj.conf](app_all_task/prj.conf). The
application Kconfig definitions are in [app_all_task/Kconfig](app_all_task/Kconfig).

| Parameter | Purpose | Current value | Recommended value | Location |
| --- | --- | --- | --- | --- |
| `CONFIG_APP_LOW_POWER_FSM` | Enable the low-power state machine. | `y` | Keep `y` for production. | `prj.conf` |
| `CONFIG_APP_DEEP_SLEEP_ENABLE` | Allow real deep-sleep / power-off paths. | `y` | `y` for production; temporarily disable only for power debugging. | `prj.conf` |
| `CONFIG_APP_RETENTION_ENABLE` | Enable retained state. | `y` | Keep `y` for production. | `prj.conf` |
| `CONFIG_APP_DEEP_SLEEP_SIMULATE` | Simulate deep sleep without actual power-off. | `n` | `n` for production; `y` for bring-up. | `prj.conf` |
| `CONFIG_APP_MICROPHONE_DEBUG_SKIP_GPS_LORA` | Skip GPS and LoRaWAN while debugging the microphone path. | `n` | `n` for normal operation; `y` for audio debugging. | `prj.conf` |
| `CONFIG_APP_STATUS_LED` | Enable status LED event pulses. | `n` | `n` for low-power deployment; `y` for field debugging. | `prj.conf` |
| `CONFIG_APP_INFERENCE_LABEL_CNT` | Number of classifier labels. | `11` | Must match the number of non-empty lines in the label file. | `prj.conf`, `Kconfig` |
| `CONFIG_APP_INFERENCE_LABELS_FILE` | Classifier label file. | `config/inference_labels.txt` | Update together with model labels. | `prj.conf`, `app_all_task/config` |
| `CONFIG_APP_INFERENCE_USE_INT16_QUANT_MODEL` | Use the int16 quantized model interface. | `y` | Keep `y` for the current model. | `prj.conf` |
| `CONFIG_APP_MIC_MICPGA_GAIN_DB` | ADC3101 analog microphone PGA gain applied to both channels. | `12` dB | Tune by recorded peak level; lower it if clipping appears. | `prj.conf`, `Kconfig` |
| `CONFIG_APP_INFERENCE_DETECT_CONF_THRESH_PERCENT` | Detector confidence threshold. | Kconfig default `50` | Tune based on false positives / false negatives. | `Kconfig` |
| `CONFIG_APP_LORAWAN_UPLINK_INTERVAL_MS` | LoRaWAN uplink interval. | `1800000` ms | Default is 30 minutes; tune by power budget and network capacity. | `prj.conf` |
| `CONFIG_APP_GPS_UPLINK_WAIT_TIMEOUT_MS` | Time to wait for GPS before uplink. | `60000` ms | `30000~60000` ms is typical. | `prj.conf` |
| `CONFIG_APP_WAKEUP_SRC_BUTTON` | Button wake source. | `y` | Keep `y` if manual wake is needed. | `prj.conf` |
| `CONFIG_APP_WAKEUP_SRC_PERIODIC` | Periodic wake source. | `y` | Keep `y` for production. | `prj.conf` |
| `CONFIG_APP_WAKEUP_SRC_COMM` | Communication-request wake source. | `y` | Keep `y` for production. | `prj.conf` |
| `CONFIG_APP_WAKEUP_SRC_RTC_ALARM` | RTC alarm wake source. | `y` | Keep `y` for production. | `prj.conf` |
| `CONFIG_APP_WAKEUP_SRC_SOUND` | External sound IRQ wake source. | `y` | Tune based on hardware false-trigger behavior. | `prj.conf` |
| `CONFIG_APP_WAKEUP_SRC_LOW_BAT` | Low-battery protection trigger. | `y` | Keep `y` for production. | `prj.conf` |
| `APP_LOW_BATTERY_MV` | Low-battery threshold. | `3500` mV | Tune according to battery and power policy. | `src/tasks.h` |
| `APP_BOOT_GPS_SYNC_TIMEOUT_MS` | GPS time-sync wait during boot. | `180000` ms | Keep for outdoor deployment; shorten for debugging. | `src/main.c` |
| `APP_DEEP_SLEEP_WAKE_SEC` | RTC wake period used when boot detects that the battery is still low and immediately powers down again. | `3600` s | If all low-battery recovery periods should match, align this with `DEEP_SLEEP_WAKE_PERIOD_MS`. | `src/main.c` |
| `ACTIVE_BURST_MS` | ACTIVE window duration. | `10000` ms | Tune by capture window and power budget. | `src/power_fsm.c` |
| `SUSPEND_TO_ACTIVE_PERIOD_MS` | SUSPEND-to-ACTIVE period. | `60000` ms | Current test value is 1 minute; increase for long-term deployment as needed. | `src/power_fsm.c` |
| `DEEP_SLEEP_WAKE_PERIOD_MS` | Default RTC wake period when the low-power FSM enters DEEP_SLEEP. | `1800000` ms | Default is 30 minutes; runtime low-battery events use this value. | `src/power_fsm.c` |
| `MIC_SAMPLE_FREQUENCY` | Audio sample rate. | `16000` Hz | Keep 16 kHz for the current model. | `src/microphone_task.c` |
| `STORAGE_WAV_SAMPLE_RATE` | WAV file sample rate. | `16000` Hz | Keep in sync with the microphone sample rate. | `src/storage_task.c` |
| `LORAWAN_APP_PORT` | LoRaWAN uplink port. | `2` | Keep cloud decoder in sync. | `src/lorawan_task.c` |
| `WATCHDOG_TIMEOUT_MS` | Hardware watchdog timeout. | `24000` ms | Do not shorten unless blocking paths are fully understood. | `src/system_task.c` |
| `DFU_READ_CHUNK_SIZE` | SD DFU read chunk size. | `4096` B | Keep default. | `src/dfu_task.c` |

`APP_DEEP_SLEEP_WAKE_SEC` and `DEEP_SLEEP_WAKE_PERIOD_MS` are used on different
paths. The former is used by `main.c` when the boot-time low-battery check fails
and the device immediately powers down again. The latter is the default period
used by `power_fsm.c` after the system is running and enters DEEP_SLEEP. They can
be different, but if the product policy requires one low-battery recovery period,
adjust both values together.

Classifier labels are stored in
[app_all_task/config/inference_labels.txt](app_all_task/config/inference_labels.txt).
If labels are changed, update `CONFIG_APP_INFERENCE_LABEL_CNT` and the cloud
payload decoder at the same time.

### LoRaWAN Join Parameters

LoRaWAN uses OTAA. The server-side configuration and firmware must use matching
parameters:

| Parameter | Source | Where to configure it |
| --- | --- | --- |
| `JoinEUI` | Assigned by the LoRaWAN network/application server. It may also be called `AppEUI`. | `join_eui[]` in [app_all_task/src/lorawan_task.c](app_all_task/src/lorawan_task.c). |
| `AppKey` / `NwkKey` | 16-byte key generated by, or created for, the LoRaWAN server. The current code uses the same `app_key` as `nwk_key`. | `app_key[]` in [app_all_task/src/lorawan_task.c](app_all_task/src/lorawan_task.c). |
| `DevEUI` | Device-unique ID. The firmware first loads it from `/dev.txt` on the SD card; if missing, it derives one from the MCU hardware ID and writes it back to `/dev.txt`. | Enter the same DevEUI when creating the device on the server. |

Recommended flow:

1. Create an application on the LoRaWAN server and obtain or set `JoinEUI` and `AppKey`.
2. Fill `JoinEUI` and `AppKey` in `lorawan_task.c` using MSB-first byte order.
3. Flash the firmware and open the serial log.
4. On first boot, read the `Generated DevEUI`, `Loaded DevEUI`, or `Fallback DevEUI` log line.
5. Enter that DevEUI in the server device profile / end-device page.
6. If the SD card already contains `/dev.txt`, the same device will continue using that DevEUI.

Example: if the server shows `JoinEUI = 0011223344556677`, configure it as:

```c
static uint8_t join_eui[] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
};
```

Do not commit real production `AppKey`, `NwkKey`, or other secret keys to a
public repository.

MCUboot signature configuration is in
[app_all_task/mcuboot_bird_ec256.conf](app_all_task/mcuboot_bird_ec256.conf).
`CONFIG_BOOT_SIGNATURE_KEY_FILE` points to the signing private key; do not commit
private keys to the repository.

## Build and Flash

### Build Environment

The commands below assume the Zephyr workspace is `/home/usr_name/zephyrproject`
and this repository is
`/home/usr_name/zephyrproject/git/ambient-firmware-zephyr`.

```bash
cd /home/usr_name/zephyrproject
source .venv/bin/activate

REPO=/home/usr_name/zephyrproject/git/ambient-firmware-zephyr
KEY=/home/usr_name/zephyrproject/keys/bird-mcuboot-ec256.pem
MCUBOOT_CONF=$REPO/app_all_task/mcuboot_bird_ec256.conf
STM32_CLI=/home/usr_name/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI
```

Check tools and key:

```bash
west --version
imgtool version
ls -l $KEY
```

If `west` is not found, the Python virtual environment is usually not active.

### Initial PC Flashing

For first-time flashing, or after changing the signing key, flash MCUboot with
the matching public key first, then write the signed application image to slot0.
Do not flash an unsigned `zephyr.bin` directly to the device.

#### 1. Prepare the MCUboot overlay

MCUboot must link to `boot_partition`. If this overlay does not already exist in
the workspace, create it once:

```bash
mkdir -p bootloader/mcuboot/boot/zephyr/boards

cat > bootloader/mcuboot/boot/zephyr/boards/stm32u5_bird.overlay <<'EOF'
/ {
    chosen {
        zephyr,console = &usart2;
        zephyr,shell-uart = &usart2;
        zephyr,code-partition = &boot_partition;
    };
};
EOF
```

For `stm32l496g_bird`, use the filename `stm32l496g_bird.overlay` with the same
contents.

#### 2. Optional full-chip erase

A full-chip erase is recommended for first deployment:

```bash
$STM32_CLI -c port=SWD mode=UR -e all
```

#### 3. Build and flash signed-key MCUboot

```bash
west build -p always \
  -b stm32u5_bird \
  bootloader/mcuboot/boot/zephyr \
  -d build_mcuboot_u5_ec256 \
  -- -DBOARD_ROOT=$REPO -DEXTRA_CONF_FILE=$MCUBOOT_CONF

west flash -d build_mcuboot_u5_ec256
```

Confirm that the MCUboot signature configuration is active:

```bash
grep -E "CONFIG_BOOT_SIGNATURE_TYPE|CONFIG_BOOT_SIGNATURE_KEY_FILE|CONFIG_BOOT_VALIDATE_SLOT0|CONFIG_BOOT_UPGRADE_ONLY" \
  build_mcuboot_u5_ec256/zephyr/.config
```

You should see `CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y` and the key path should
match `$KEY`.

#### 4. Build the application

```bash
west build -p always \
  -b stm32u5_bird \
  $REPO/app_all_task \
  -d build_app_u5 \
  -- -DBOARD_ROOT=$REPO -DDTS_ROOT=$REPO/app_all_task
```

#### 5. Use the auto-signed application image

`app_all_task/prj.conf` disables unsigned-only generation and sets
`CONFIG_MCUBOOT_SIGNATURE_KEY_FILE`, so `west build` automatically creates:

```text
build_app_u5/zephyr/zephyr.signed.bin
build_app_u5/zephyr/zephyr.signed.hex
```

Do not use the unsigned `build_app_u5/zephyr/zephyr.bin` for production flashing
or DFU.

#### 6. Verify the signature

```bash
imgtool verify -k $KEY build_app_u5/zephyr/zephyr.signed.bin
imgtool dumpinfo build_app_u5/zephyr/zephyr.signed.bin
```

Do not flash the image if `verify` fails.

#### 7. Write the signed application to slot0

The slot0 start address is `0x08010000`:

```bash
$STM32_CLI \
  -c port=SWD \
  -w build_app_u5/zephyr/zephyr.signed.bin 0x08010000 \
  -v \
  -rst
```

After reset, MCUboot verifies the slot0 signature. The application boots only if
verification succeeds.

#### L4 board

For `stm32l496g_bird`, replace the following in the commands above:

- `stm32u5_bird` with `stm32l496g_bird`
- `build_mcuboot_u5_ec256` with `build_mcuboot_l4_ec256`
- `build_app_u5` with `build_app_l4`

The slot0 start address is still `0x08010000`.

### SD-Card DFU Update

Later updates do not require reflashing MCUboot. Build and verify the
application, then place the auto-signed `.bin` in the SD-card root as
`update.bin`.

#### 1. Generate the DFU file

```bash
cd /home/usr_name/zephyrproject
source .venv/bin/activate

REPO=/home/usr_name/zephyrproject/git/ambient-firmware-zephyr
KEY=/home/usr_name/zephyrproject/keys/bird-mcuboot-ec256.pem

west build -p always \
  -b stm32u5_bird \
  $REPO/app_all_task \
  -d build_app_u5 \
  -- -DBOARD_ROOT=$REPO -DDTS_ROOT=$REPO/app_all_task

imgtool verify -k $KEY build_app_u5/zephyr/zephyr.signed.bin
```

For L4, replace the board and build directory as described above.

#### 2. Put the image on the SD card

Place this file in the SD-card root and rename it to:

```text
update.bin
```

Source file:

```text
build_app_u5/zephyr/zephyr.signed.bin
```

The SD-card root should look like:

```text
/update.bin
```

#### 3. Device-side update flow

1. Insert the SD card and boot the device, or let the device run until the next ACTIVE-exit check.
2. The application checks `/SD:/update.bin`.
3. The application writes the file into MCUboot slot1.
4. The application requests a permanent upgrade and renames `update.bin` to `update.done`.
5. The device reboots.
6. MCUboot verifies the slot1 signature.
7. If the signature is valid, MCUboot overwrites/starts the new firmware.

Notes:

- `update.bin` must be `zephyr.signed.bin`, not `zephyr.bin`.
- `update.bin` must be signed with the private key matching the public key compiled into MCUboot.
- U5 slot1 size is `0xE8000`; L4 slot1 size is `0x68000`.
- Keep power stable during the update.
- If the signature is wrong, the board target does not match, or the image is too large, MCUboot rejects the new image.

## Devicetree Configuration

This project combines the board base DTS with an application overlay:

| Target | Base DTS | App overlay |
| --- | --- | --- |
| STM32L496 | [stm32l496g_bird/stm32l496g_bird.dts](stm32l496g_bird/stm32l496g_bird.dts) | [app_all_task/boards/stm32l496g_bird.overlay](app_all_task/boards/stm32l496g_bird.overlay) |
| STM32U595R | [stm32u5_bird/stm32u5_bird.dts](stm32u5_bird/stm32u5_bird.dts) | [app_all_task/boards/stm32u5_bird.overlay](app_all_task/boards/stm32u5_bird.overlay) |

Key configuration:

- `zephyr,code-partition = &slot0_partition` in `chosen`, so the application links to MCUboot slot0.
- `boot_partition`, `slot0_partition`, `slot1_partition`, `scratch_partition`, and `storage_partition` are defined in the base DTS files.
- `USART2` is used as console/shell UART.
- `SAI1_B` is used for ADC3101 audio capture, with default/sleep pinctrl states.
- `SPI3` connects both LoRa SX1262 and the SPI SD card.
- `I2C1` connects GPS/BME280; `I2C2` connects ADC3101/INA3221-related peripherals.
- `v_periph`, GPS power GPIOs, and SPI/SD-related GPIOs are managed by `power_ctrl.c`.
- Retained memory stores boot, low-battery, SD fuse, and related state. U5 uses backup SRAM; L4 uses the BBRAM binding.
- RTC and LPTIM are used for low-power waiting, alarm wakeup, and system tick.

Partition sizes:

| Target | MCUboot | slot0 | slot1 | scratch | storage |
| --- | ---: | ---: | ---: | ---: | ---: |
| STM32L496 | `0x10000` | `0x68000` | `0x68000` | `0x10000` | `0x10000` |
| STM32U595R | `0x10000` | `0xE8000` | `0xE8000` | `0x10000` | `0x10000` |

Custom bindings are in [app_all_task/dts/bindings](app_all_task/dts/bindings).
They currently include `bird,power-ctrl-pins` and `bird,retained-bbram`.

## LoRaWAN Event Payload

LoRaWAN uses port `2`. The current payload format is "variable bucket count +
fixed section".

With the default configuration:

- `CONFIG_APP_INFERENCE_LABEL_CNT=11`
- bucket count `N = 12`
- total length `1 + N + 23 = 36` bytes

Byte 0 carries the bucket count, so the cloud decoder should read `payload[0]`
first instead of hard-coding the length.

| Byte(s) | Field | Encoding |
| --- | --- | --- |
| `0` | `bucket_count` | `N`, default `12`. |
| `1..N` | inference buckets | Saturated `uint8` count for each bucket. |
| `1+N .. 2+N` | battery voltage | `int16` big-endian, mV. |
| `3+N .. 4+N` | solar voltage | `int16` big-endian, mV. |
| `5+N` | temperature | 1 byte, interpret as signed integer, degC. |
| `6+N .. 7+N` | total inference count | `uint16` big-endian, low 16 bits. |
| `8+N .. 9+N` | battery current | `int16` big-endian, mA. |
| `10+N .. 11+N` | solar current | `int16` big-endian, mA. |
| `12+N` | SD status | `0` no recent write, `1` write succeeded, `2` SD fuse/backoff. |
| `13+N .. 14+N` | latitude | Legacy 2-byte compressed coordinate format. |
| `15+N .. 16+N` | longitude | Legacy 2-byte compressed coordinate format. |
| `17+N .. 18+N` | average RMS | Legacy 2-byte compressed RMS format. |
| `19+N` | SD usage | `uint8`, percentage `0..100`. |
| `20+N .. 21+N` | duty-cycle fail count | `uint16` big-endian, cumulative since boot. |
| `22+N .. 23+N` | dropped window count | `uint16` big-endian, cumulative since boot. |

With the default `N=12`, the fixed section starts at byte `13`, and the total
payload covers bytes `0..35`.

Default bucket meanings:

| Bucket | Meaning |
| ---: | --- |
| `0` | Eurasian Blackcap |
| `1` | Eurasian Wren |
| `2` | European Robin |
| `3` | Common Chaffinch |
| `4` | Eurasian Blackbird |
| `5` | Common Chiffchaff |
| `6` | Great Tit |
| `7` | Short-toed Treecreeper |
| `8` | Great Spotted Woodpecker |
| `9` | Long-tailed Tit |
| `10` | not_target |
| `11` | Detector reported bird, but confidence was below the threshold. |

## FAQ

**The application does not boot after flashing**

Check that MCUboot was built with the same key, and that slot0 contains
`zephyr.signed.bin` at address `0x08010000`.

**DFU did not upgrade the firmware**

Confirm that the SD-card root file is named `update.bin`, that it comes from
`zephyr.signed.bin`, and that `imgtool verify -k $KEY update.bin` passes.

**I only want to debug the microphone and avoid GPS/LoRaWAN waits**

Set this in `prj.conf`:

```conf
CONFIG_APP_MICROPHONE_DEBUG_SKIP_GPS_LORA=y
```

Set it back to `n` after debugging.
