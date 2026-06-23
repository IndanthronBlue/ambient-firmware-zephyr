

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

#ifndef CONFIG_APP_INFERENCE_LABEL_CNT
#define CONFIG_APP_INFERENCE_LABEL_CNT 11
#endif

#ifndef CONFIG_APP_LORAWAN_UPLINK_INTERVAL_MS
#define CONFIG_APP_LORAWAN_UPLINK_INTERVAL_MS (30 * 60 * 1000)
#endif

#ifndef CONFIG_APP_INFERENCE_DETECT_CONF_THRESH_PERCENT
#define CONFIG_APP_INFERENCE_DETECT_CONF_THRESH_PERCENT 50
#endif

#define APP_INFERENCE_LABEL_CNT CONFIG_APP_INFERENCE_LABEL_CNT
#define APP_INFERENCE_LOW_CONF_BIRD_BUCKET APP_INFERENCE_LABEL_CNT
#define APP_INFERENCE_UPLINK_BUCKET_COUNT (APP_INFERENCE_LABEL_CNT + 1U)
#define APP_LORAWAN_UPLINK_INTERVAL_MS CONFIG_APP_LORAWAN_UPLINK_INTERVAL_MS
#define APP_INFERENCE_DETECT_CONF_THRESH \
	((float)CONFIG_APP_INFERENCE_DETECT_CONF_THRESH_PERCENT / 100.0f)

#ifndef CONFIG_APP_GPS_UPLINK_WAIT_TIMEOUT_MS
#define CONFIG_APP_GPS_UPLINK_WAIT_TIMEOUT_MS 30000
#endif

#define APP_GPS_UPLINK_WAIT_TIMEOUT_MS CONFIG_APP_GPS_UPLINK_WAIT_TIMEOUT_MS

#if (APP_INFERENCE_LABEL_CNT < 1) || (APP_INFERENCE_LABEL_CNT > 254)
#error "APP_INFERENCE_LABEL_CNT must leave room for the low-confidence bird bucket"
#endif

#if (APP_INFERENCE_UPLINK_BUCKET_COUNT > 255)
#error "APP_INFERENCE_UPLINK_BUCKET_COUNT must fit in one payload byte"
#endif

#if (APP_LORAWAN_UPLINK_INTERVAL_MS < 1000)
#error "APP_LORAWAN_UPLINK_INTERVAL_MS must be at least 1000 ms"
#endif

#if (CONFIG_APP_INFERENCE_DETECT_CONF_THRESH_PERCENT < 0) || \
	(CONFIG_APP_INFERENCE_DETECT_CONF_THRESH_PERCENT > 100)
#error "APP_INFERENCE_DETECT_CONF_THRESH_PERCENT must be between 0 and 100"
#endif

#define APP_SDON_NO_RECENT_WRITE 0U
#define APP_SDON_WRITE_OK 1U
#define APP_SDON_FUSED 2U

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
	bool mic_codec_zero_fault;
	bool lorawan_joined;
	uint32_t lorawan_join_attempts;
	uint32_t lorawan_uplink_count;
	int lorawan_last_error;
	uint16_t duty_cycle_fail_count;
	uint16_t dropped_window_count;
	/* GPS state */
	bool gps_valid;
	float gps_latitude;
	float gps_longitude;
	uint32_t gps_fix_count;
	/* Date/time from GPS RMC converted to Europe/Paris local time */
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
	float infer_avg_rms_uplink;    /* Snapshot average RMS paired with uplink counts */
	uint16_t infer_label_counts_window[APP_INFERENCE_UPLINK_BUCKET_COUNT]; /* Live timed count buckets */
	uint16_t infer_label_counts_uplink[APP_INFERENCE_UPLINK_BUCKET_COUNT]; /* Snapshot for LoRa uplink */
	uint32_t infer_total_count_uplink; /* Snapshot count paired with infer_label_counts_uplink */
	uint32_t infer_window_count;   /* Completed inference cycles in current timed window */
	bool infer_window_ready;       /* True when a timed inference-count snapshot is ready for uplink */
	/* Inference timing (for pipeline showcase) */
	uint16_t infer_mfcc_chunk_ms[6];     /* 6 chunks per series (6*31 MFCC rows = 186 rows) */
	uint32_t infer_mfcc_series_ms;       /* MFCC+model time for 6 chunks (workqueue side) */
	uint16_t infer_detector_ms;
	uint16_t infer_classifier_ms;
	uint32_t infer_capture_to_done_ms;   /* From capture start to series done */
	uint32_t infer_session_id_done;      /* Session id that produced timing above */
	/* Ambient payload helper fields */
	uint8_t sdon;                  /* 0 no recent write, 1 write ok, 2 SD fuse/backoff */
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

typedef enum {
	STATUS_LED_BOOT_OK,
	STATUS_LED_ACTIVE_ENTER,
	STATUS_LED_CAPTURE_START,
	STATUS_LED_CODEC_RECORDING_OK,
	STATUS_LED_INFER_DONE,
	STATUS_LED_UPLINK_READY,
	STATUS_LED_GPS_WAIT,
	STATUS_LED_LORA_OK,
	STATUS_LED_ERROR,
} status_led_event_t;

int task_status_led_init(void);
void task_status_led_event(status_led_event_t event);
void task_status_led_prepare_sleep(void);
void task_status_led_set_enabled(bool enabled);

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
bool task_microphone_capture_once(void);
void task_microphone_prepare_sleep(void);

int codec_adc3101_init(void);


void task_voltage_monitor(void);

void task_sensor_sample(void);
void task_sensor_prepare_sleep(void);

int task_imu_init(void);
void task_imu_sample(void);

int task_ina3221_init(void);
void task_ina3221_monitor(void);
int task_ina3221_read_now(void);
void task_ina3221_block_active(bool block);
bool task_ina3221_wait_idle(uint32_t timeout_ms);


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
void task_storage_pcm_end_from_worker(void);
void task_storage_sd_fuse_init(void);
bool task_storage_sd_fuse_active(void);
bool task_storage_local_write_blocked(void);
task_storage_status_t task_storage_persist_dev_eui(const uint8_t dev_eui[8]);
bool task_storage_load_dev_eui(uint8_t dev_eui[8]);
int task_storage_reserve_lorawan_dev_nonce(uint16_t seed, uint16_t *dev_nonce);
task_storage_status_t task_storage_persist_gps_coords(float latitude, float longitude);
bool task_storage_load_gps_coords(float *latitude, float *longitude);
void task_storage_set_session_id(uint32_t session_id);
void task_sd_list_root(void);
void task_status_persist(void);

void task_dfu_check_and_apply(void);
void task_dfu_check_and_apply_periodic(void);


int task_lorawan_connect(void);
int task_lorawan_send_event_uplink(void);
bool task_lorawan_drop_uplink_if_backoff(void);


void task_gps_poll(void);
bool task_gps_acquire_for_uplink(void);
bool task_gps_acquire_with_timeout(uint32_t timeout_ms, bool *rtc_synced);
bool task_gps_acquire_with_timeout_main_alive(uint32_t timeout_ms, bool *rtc_synced);
void task_i2c1_lock(void);
void task_i2c1_unlock(void);
void task_i2c2_lock(void);
void task_i2c2_unlock(void);


int task_inference_init(void);
void task_inference_process_block(const void *rx_mem_block, size_t sz_bytes);
void task_inference_session_begin(uint32_t session_id, uint32_t capture_start_ms);
bool task_inference_wait_series_done(uint32_t session_id, uint32_t timeout_ms);
void task_inference_reset_periodic_stats(void);
void task_inference_consume_uplink_window(void);
void task_inference_snapshot_uplink_window(void);
void task_inference_prepare_codec_zero_fault_uplink(void);
void task_inference_prepare_sleep(void);
void task_inference_resume_after_wakeup(void);


int task_watchdog_init(void);
void task_watchdog_feed(void);
void task_watchdog_mark_main_alive(void);
void task_watchdog_set_comm_active(bool active);
void task_watchdog_mark_comm_alive(void);
void task_watchdog_note_sd_op_start(const char *op_name, uint32_t timeout_ms);
void task_watchdog_note_sd_op_end(void);
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
int task_rtc_get_time_for_storage(struct rtc_time *tm, bool *is_default_seed);
int task_rtc_set_from_gps(void);
int task_rtc_get_epoch_local(uint32_t *local_epoch);
bool task_rtc_time_is_real(const struct rtc_time *tm);
int task_rtc_set_alarm_in_seconds(uint32_t seconds_from_now);
int task_rtc_set_alarm_at_epoch(uint32_t epoch_local);
void task_rtc_prepare_shutdown_wakeup_route(void);

/* ========== Low-power preparation hooks ========== */
int task_storage_prepare_sleep(void);
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
