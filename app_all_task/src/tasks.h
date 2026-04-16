

#ifndef APP_TASKS_H
#define APP_TASKS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/rtc.h>
#include "power_fsm.h"

#define APP_FW_VERSION_MAJOR 1U
#define APP_FW_VERSION_MINOR 0U
#define APP_FW_VERSION_PATCH 3U

/* Unified low-battery protection threshold used by FSM and INA worker. */
#define APP_LOW_BATTERY_MV 3500

/* ========== Task State Structure ========== */
typedef struct {
	uint16_t fw_version_major;
	uint16_t fw_version_minor;
	uint16_t fw_version_patch;
	uint32_t button_press_count;
	bool led_state;
	uint16_t last_solar_mv;
	uint16_t last_battery_mv;
	float last_temp_celsius;
	float last_press_kpa;
	float last_humidity_percent;
	float imu_accel_x_g;
	float imu_accel_y_g;
	float imu_accel_z_g;
	uint32_t voltage_read_count;
	uint32_t sensor_read_count;
	uint32_t imu_read_count;
	/* INA3221 readings */
	float ina_ch1_voltage_v;
	float ina_ch2_voltage_v;
	float ina_ch3_voltage_v;
	float ina_ch1_current_ma;
	float ina_ch2_current_ma;
	float ina_ch3_current_ma;
	uint32_t ina_read_count;
	bool sd_mounted;
	bool sd_listed_once;
	bool sd_capacity_valid;
	uint32_t sd_total_mib;
	uint32_t sd_free_mib;
	bool mic_recording;
	bool mic_record_request;
	bool lorawan_joined;
	uint32_t lorawan_join_attempts;
	uint32_t lorawan_uplink_count;
	int lorawan_last_error;
	/* GPS state */
	bool gps_valid;
	float gps_latitude;
	float gps_longitude;
	uint32_t gps_fix_count;
	/* Date/time from GPS RMC (UTC) */
	bool time_valid;
	uint16_t time_year; /* full year, e.g., 2026 */
	uint8_t time_month; /* 1..12 */
	uint8_t time_day;   /* 1..31 */
	uint8_t time_hour;  /* 0..23 */
	uint8_t time_minute;/* 0..59 */
	uint8_t time_second;/* 0..59 */
	/* Inference output state */
	int16_t infer_last_label;      /* -2/-1/0..10 */
	uint32_t infer_total_count;    /* Total completed inference cycles */
	float infer_last_rms;          /* RMS of latest processed audio window */
	float infer_avg_rms;           /* Running average RMS */
	int8_t infer_labels_window[10]; /* Latest 10 inference labels (live accumulation) */
	int8_t infer_labels_uplink[10]; /* Snapshot for LoRa uplink (overwritten each window) */
	uint32_t infer_total_count_uplink; /* Snapshot count paired with infer_labels_uplink */
	uint8_t infer_window_count;    /* Number of valid labels in window */
	bool infer_window_ready;       /* True when 10 labels are ready for uplink */
	/* Inference timing (for pipeline showcase) */
	uint16_t infer_mfcc_chunk_ms[6];     /* 6 chunks per series (each chunk = 31 MFCC rows) */
	uint32_t infer_mfcc_series_ms;       /* MFCC+model time for 6 chunks (workqueue side) */
	uint16_t infer_detector_ms;
	uint16_t infer_classifier_ms;
	uint32_t infer_capture_to_done_ms;   /* From capture start to series done */
	uint32_t infer_session_id_done;      /* Session id that produced timing above */
	/* Ambient payload helper fields */
	uint8_t sdon;                  /* 1 if SD write succeeded since last uplink */
	uint8_t sd_usage_percent;      /* 0..100 */
	/* SD PCM write timing (per capture session) */
	bool sd_pcm_write_enabled;
	uint32_t sd_pcm_blocks;
	uint32_t sd_pcm_total_bytes;
	uint32_t sd_pcm_total_ms;
	uint32_t sd_pcm_max_ms;
	uint32_t sd_pcm_avg_ms;
	uint32_t sd_pcm_last_block_ms;
} app_state_t;

/* ========== Global Application State ========== */
extern app_state_t app_state;

int task_button_init(void);
int task_sound_wakeup_init(void);
void task_sound_wakeup_prepare_sleep(void);

int task_microphone_init(void);
bool task_microphone_is_initialized(void);
/**
 * @brief Trigger a single 3s recording (safe to call in ISR)
 */
void task_microphone_trigger_recording(void);

/**
 * @brief Perform a single 3s audio capture serially in main loop
 *        This runs in thread context (non-ISR) and blocks sequentially.
 */
void task_microphone_capture_once(void);
void task_microphone_prepare_sleep(void);

int codec_adc3101_init(void);


void task_voltage_monitor(void);

void task_sensor_sample(void);

int task_imu_init(void);
void task_imu_sample(void);

int task_ina3221_init(void);
void task_ina3221_monitor(void);
int task_ina3221_read_now(void);
void task_ina3221_block_active(bool block);


void task_sd_mount(void);
void task_sd_mount_async(void);
void task_sd_ensure_mounted(void);
void task_sd_check_capacity(void);
typedef enum {
	TASK_STORAGE_STATUS_OK = 0,
	TASK_STORAGE_STATUS_NOT_MOUNTED = 1,
	TASK_STORAGE_STATUS_CAPACITY_UNAVAILABLE = 2,
	TASK_STORAGE_STATUS_NO_SPACE = 3,
	TASK_STORAGE_STATUS_IO_ERROR = 4,
	TASK_STORAGE_STATUS_INVALID_ARG = 5,
	TASK_STORAGE_STATUS_NOT_OPEN = 6,
} task_storage_status_t;
task_storage_status_t task_storage_pcm_begin(char *out_path, size_t out_sz);
task_storage_status_t task_storage_pcm_write(const void *data, size_t len);
void task_storage_pcm_end(void);
task_storage_status_t task_storage_persist_dev_eui(const uint8_t dev_eui[8]);
bool task_storage_load_dev_eui(uint8_t dev_eui[8]);
task_storage_status_t task_storage_persist_gps_coords(float latitude, float longitude);
bool task_storage_load_gps_coords(float *latitude, float *longitude);
void task_storage_set_session_id(uint32_t session_id);
void task_sd_list_root(void);
void task_status_persist(void);

void task_dfu_check_and_apply(void);


int task_lorawan_connect(void);
int task_lorawan_send_event_uplink(void);


void task_gps_poll(void);
bool task_gps_acquire_for_uplink(void);
bool task_gps_acquire_with_timeout(uint32_t timeout_ms, bool *rtc_synced);
void task_i2c1_lock(void);
void task_i2c1_unlock(void);
void task_i2c2_lock(void);
void task_i2c2_unlock(void);
void task_i2c_debug_scan_startup(void);


int task_inference_init(void);
void task_inference_process_block(const void *rx_mem_block, size_t sz_bytes);
void task_inference_session_begin(uint32_t session_id, uint32_t capture_start_ms);
bool task_inference_wait_series_done(uint32_t session_id, uint32_t timeout_ms);
void task_inference_reset_periodic_stats(void);
void task_inference_consume_uplink_window(void);
void task_inference_snapshot_uplink_window(void);
void task_inference_prepare_sleep(void);
void task_inference_resume_after_wakeup(void);


int task_watchdog_init(void);
void task_watchdog_feed(void);
void task_system_periodic_reboot_check(void);


int task_comm_init(void);
typedef enum {
	TASK_COMM_UPLINK_NO_GPS = 0,
	TASK_COMM_UPLINK_NEED_GPS = 1,
	TASK_COMM_UPLINK_WAIT_GPS = 2,
} task_comm_uplink_type_t;
void task_comm_request_uplink(void);
void task_comm_request_uplink_typed(task_comm_uplink_type_t type);
bool task_comm_is_pending_or_busy(void);


int task_rtc_init(void);
int task_rtc_set_time(const struct rtc_time *tm);
int task_rtc_get_time(struct rtc_time *tm);
int task_rtc_set_from_gps(void);
int task_rtc_get_epoch_utc(uint32_t *epoch);
bool task_rtc_time_is_real(const struct rtc_time *tm);
int task_rtc_set_alarm_in_seconds(uint32_t seconds_from_now);
int task_rtc_set_alarm_at_epoch(uint32_t epoch_utc);
void task_rtc_prepare_shutdown_wakeup_route(void);

/* ========== Low-power preparation hooks ========== */
void task_storage_prepare_sleep(void);
void task_sd_mount_async(void);
void task_sd_mount_async_cancel(void);
void task_gps_prepare_sleep(void);
void task_lorawan_prepare_sleep(void);

/* ========== Unified power control ========== */
int power_ctrl_vperiph_on(void);
int power_ctrl_vperiph_on_for_i2c1(void);
int power_ctrl_vperiph_on_for_i2c2(void);
int power_ctrl_vperiph_off(void);
int power_ctrl_gps_on(void);
int power_ctrl_gps_off(void);
int power_ctrl_prepare_suspend(void);
int power_ctrl_prepare_deep_sleep_all(void);

/* ========== Low-power state machine core ========== */
int power_fsm_init(void);
void power_fsm_tick(void);
void power_fsm_notify_active_done(void);


#endif /* APP_TASKS_H */

