# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app_bird)

target_sources(app PRIVATE
    src/main.c
    src/app_state.c
    src/sensor_task.c
    src/ina3221_task.c
    src/storage_task.c
    src/microphone_task.c
    src/inference_task.c
    src/ADC3101.c
    src/lorawan_task.c
    src/gps_task.c
    src/rtc_task.c
    src/system_task.c
    src/comm_task.c
)
