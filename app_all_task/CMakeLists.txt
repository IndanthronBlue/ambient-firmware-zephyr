# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app_bird)

if(NOT DEFINED CONFIG_APP_INFERENCE_LABEL_CNT)
    message(FATAL_ERROR "CONFIG_APP_INFERENCE_LABEL_CNT is not defined")
endif()

if(NOT DEFINED CONFIG_APP_INFERENCE_LABELS_FILE OR "${CONFIG_APP_INFERENCE_LABELS_FILE}" STREQUAL "")
    message(FATAL_ERROR "CONFIG_APP_INFERENCE_LABELS_FILE is not defined")
endif()

set(APP_INFERENCE_LABELS_FILE "${CONFIG_APP_INFERENCE_LABELS_FILE}")
if(IS_ABSOLUTE "${APP_INFERENCE_LABELS_FILE}")
    set(APP_INFERENCE_LABELS_PATH "${APP_INFERENCE_LABELS_FILE}")
else()
    set(APP_INFERENCE_LABELS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${APP_INFERENCE_LABELS_FILE}")
endif()

if(NOT EXISTS "${APP_INFERENCE_LABELS_PATH}")
    message(FATAL_ERROR "Inference label list file not found: ${APP_INFERENCE_LABELS_PATH}")
endif()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${APP_INFERENCE_LABELS_PATH}")

file(READ "${APP_INFERENCE_LABELS_PATH}" APP_INFERENCE_LABELS_CONTENT)
string(REPLACE "\r\n" "\n" APP_INFERENCE_LABELS_CONTENT "${APP_INFERENCE_LABELS_CONTENT}")
string(REPLACE "\r" "\n" APP_INFERENCE_LABELS_CONTENT "${APP_INFERENCE_LABELS_CONTENT}")
string(REPLACE "\n" ";" APP_INFERENCE_LABEL_LINES "${APP_INFERENCE_LABELS_CONTENT}")

set(APP_INFERENCE_LABELS)
foreach(APP_INFERENCE_LABEL_LINE IN LISTS APP_INFERENCE_LABEL_LINES)
    string(STRIP "${APP_INFERENCE_LABEL_LINE}" APP_INFERENCE_LABEL)
    if(NOT "${APP_INFERENCE_LABEL}" STREQUAL "")
        list(APPEND APP_INFERENCE_LABELS "${APP_INFERENCE_LABEL}")
    endif()
endforeach()

list(LENGTH APP_INFERENCE_LABELS APP_INFERENCE_LABELS_COUNT)
if(NOT APP_INFERENCE_LABELS_COUNT EQUAL CONFIG_APP_INFERENCE_LABEL_CNT)
    message(FATAL_ERROR
        "Inference label count mismatch: ${APP_INFERENCE_LABELS_PATH} has "
        "${APP_INFERENCE_LABELS_COUNT} labels, but CONFIG_APP_INFERENCE_LABEL_CNT="
        "${CONFIG_APP_INFERENCE_LABEL_CNT}")
endif()

set(APP_INFERENCE_LABELS_ARRAY "")
foreach(APP_INFERENCE_LABEL IN LISTS APP_INFERENCE_LABELS)
    string(REPLACE "\\" "\\\\" APP_INFERENCE_LABEL_ESCAPED "${APP_INFERENCE_LABEL}")
    string(REPLACE "\"" "\\\"" APP_INFERENCE_LABEL_ESCAPED "${APP_INFERENCE_LABEL_ESCAPED}")
    string(APPEND APP_INFERENCE_LABELS_ARRAY "    \"${APP_INFERENCE_LABEL_ESCAPED}\",\n")
endforeach()

set(APP_INFERENCE_LABELS_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/include/generated")
set(APP_INFERENCE_LABELS_HEADER "${APP_INFERENCE_LABELS_GENERATED_DIR}/app_inference_labels.h")
file(MAKE_DIRECTORY "${APP_INFERENCE_LABELS_GENERATED_DIR}")
file(WRITE "${APP_INFERENCE_LABELS_HEADER}"
"#pragma once\n"
"\n"
"#define APP_INFERENCE_LABELS_GENERATED_COUNT ${APP_INFERENCE_LABELS_COUNT}\n"
"\n"
"static const char *const kBirdLabels[APP_INFERENCE_LABELS_GENERATED_COUNT] = {\n"
"${APP_INFERENCE_LABELS_ARRAY}"
"};\n")

target_include_directories(app PRIVATE "${APP_INFERENCE_LABELS_GENERATED_DIR}")

target_sources(app PRIVATE
    src/main.c
    src/app_state.c
    src/dfu_task.c
    src/sensor_task.c
    src/imu_task.c
    src/ina3221_task.c
    src/storage_task.c
    src/microphone_task.c
    src/inference_task.c
    src/bird_detect_model_wrapper.c
    src/ezbird_model_wrapper.c
    src/ADC3101.c
    src/lorawan_task.c
    src/gps_task.c
    src/i2c_task.c
    src/rtc_task.c
    src/system_task.c
    src/status_led_task.c
    src/comm_task.c
    src/button_task.c
    src/sound_wakeup_task.c
    src/power_fsm.c
    src/retained_state.c
    src/power_ctrl.c
)

if(CONFIG_DT_HAS_BIRD_RETAINED_BBRAM_ENABLED)
    target_sources(app PRIVATE
        src/drivers/retained_mem_bbram.c
    )
endif()
