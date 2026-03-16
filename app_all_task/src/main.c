#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/device.h>

#include "tasks.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define SLEEP_WAKEUP_SEC 20
#define ACTIVE_WINDOW_MS (40U * 1000U)
#define PERIOD_10S_MS    10000U
#define IDLE_SLEEP_MAX_MS 2000U
#define GPS_BOOTSTRAP_MAX_TRIES 3000U
#define GPS_BOOTSTRAP_DELAY_MS  1000U
#define LORA_CONNECT_MAX_TRIES 3000U
#define LORA_CONNECT_DELAY_MS  10000U

static inline bool time_reached(uint32_t now, uint32_t deadline)
{
	return (int32_t)(now - deadline) >= 0;
}

static inline uint32_t ms_until(uint32_t now, uint32_t deadline)
{
	if (time_reached(now, deadline)) {
		return 0U;
	}
	return (uint32_t)(deadline - now);
}

static void advance_deadline(uint32_t *deadline, uint32_t period_ms, uint32_t now)
{
	do {
		*deadline += period_ms;
	} while (time_reached(now, *deadline));
}

static uint32_t min_u32(uint32_t a, uint32_t b)
{
	return (a < b) ? a : b;
}

static void log_task_timing(const char *task_name,
			    uint32_t scheduled_deadline_ms,
			    uint32_t start_ms,
			    uint32_t end_ms)
{
	int32_t schedule_lag_ms = (int32_t)(start_ms - scheduled_deadline_ms);
	uint32_t exec_ms = (uint32_t)(end_ms - start_ms);
	LOG_INF("[%s] sched=%u start=%u lag=%dms exec=%ums",
		task_name,
		(unsigned int)scheduled_deadline_ms,
		(unsigned int)start_ms,
		(int)schedule_lag_ms,
		(unsigned int)exec_ms);
}

static void enter_sleep_window(uint32_t delay_sec)
{
	k_sleep(K_SECONDS(delay_sec));
}

int main(void)
{
	LOG_INF("System started with distributed initialization");

	task_sd_ensure_mounted();

	k_msleep(1000);

	if (task_microphone_init() < 0) {
		LOG_ERR("Mic init failed");
	} else {
		task_inference_resume_after_wakeup();
		LOG_INF("Mic ready (SAI1_B 16kHz)");
	}

	(void)task_watchdog_init();
	(void)task_comm_init();
	task_rtc_init();
	if (task_ina3221_init() < 0) {
		LOG_ERR("INA3221 worker init failed");
	}

	//try acquiring GPS fix at startup to speed up first location acquisition and get initial location for any early events; if it fails, we'll just continue and try again later in the periodic window
	bool gps_bootstrap_ok = false;
	for (uint32_t attempt = 1U; attempt <= GPS_BOOTSTRAP_MAX_TRIES; attempt++) {
		LOG_INF("Startup GPS bootstrap attempt %u/%u",
			(unsigned int)attempt,
			(unsigned int)GPS_BOOTSTRAP_MAX_TRIES);
		if (task_gps_acquire_for_uplink()) {
			gps_bootstrap_ok = true;
			LOG_INF("Startup GPS bootstrap succeeded on attempt %u",
				(unsigned int)attempt);
			break;
		}
		if (attempt < GPS_BOOTSTRAP_MAX_TRIES) {
			k_msleep(GPS_BOOTSTRAP_DELAY_MS);
		}
	}
	if (!gps_bootstrap_ok) {
		LOG_WRN("Startup GPS bootstrap failed after %u attempts; continuing without initial fix",
			(unsigned int)GPS_BOOTSTRAP_MAX_TRIES);
	}

	task_watchdog_feed();

	k_msleep(15000);

	// like gps, try connecting to LoRaWAN at startup to speed up first uplink; if it fails, we'll just continue and try again later in the periodic window
	bool lorawan_connected = false;
	for (uint32_t attempt = 1U; attempt <= LORA_CONNECT_MAX_TRIES; attempt++) {
		LOG_INF("Startup LoRaWAN connect attempt %u/%u", (unsigned int)attempt, (unsigned int)LORA_CONNECT_MAX_TRIES);
		if (task_lorawan_connect() == 0) {
			lorawan_connected = true;
			LOG_INF("Startup LoRaWAN connect succeeded on attempt %u", (unsigned int)attempt);
			break;
		}
		k_msleep(LORA_CONNECT_DELAY_MS);
	}
	if (!lorawan_connected) {
		LOG_WRN("Startup LoRaWAN connect failed after %u attempts; continuing without network", (unsigned int)LORA_CONNECT_MAX_TRIES);
	}

	uint32_t now = k_uptime_get_32();
	uint32_t active_window_deadline = now + ACTIVE_WINDOW_MS;
	uint32_t next_10s_deadline = now;
	bool sd_mount_checked_in_window = false;
	bool sd_checked_in_window = false;

	LOG_INF("Window scheduler started: active=%u ms, sleep=%u s(k_sleep)",
		(unsigned int)ACTIVE_WINDOW_MS, (unsigned int)SLEEP_WAKEUP_SEC);

	while (1) {

		now = k_uptime_get_32();
		task_watchdog_feed();
		task_system_periodic_reboot_check();

		if (time_reached(now, active_window_deadline)) {
			LOG_INF("Active window complete, entering k_sleep");
			enter_sleep_window(SLEEP_WAKEUP_SEC);
			if (!task_microphone_is_initialized()) {
				if (task_microphone_init() < 0) {
					LOG_ERR("Mic re-init failed after sleep");
				} else {
					LOG_INF("Mic re-initialized after sleep");
				}
			}
			now = k_uptime_get_32();
			active_window_deadline = now + ACTIVE_WINDOW_MS;
			next_10s_deadline = now;
			sd_mount_checked_in_window = false;
			sd_checked_in_window = false;
			continue;
		}

		/* At each active-window start: verify SD mount, try remount if needed. */
		if (!sd_mount_checked_in_window) {
			uint32_t t0 = k_uptime_get_32();
			task_sd_ensure_mounted();
			uint32_t t1 = k_uptime_get_32();
			log_task_timing("task_sd_ensure_mounted", now, t0, t1);
			sd_mount_checked_in_window = true;
		}

		/* Check SD free capacity once in each active window. */
		if (!sd_checked_in_window) {
			uint32_t t0 = k_uptime_get_32();
			task_sd_check_capacity();
			uint32_t t1 = k_uptime_get_32();
			log_task_timing("task_sd_check_capacity", now, t0, t1);
			sd_checked_in_window = true;
		}

		if (time_reached(now, next_10s_deadline)) {
			LOG_INF("Starting scheduled audio capture + inference");
			uint32_t slot_deadline = next_10s_deadline;
			uint32_t t0 = k_uptime_get_32();
			task_microphone_capture_once();
			uint32_t t1 = k_uptime_get_32();
			log_task_timing("task_microphone_capture_once", slot_deadline, t0, t1);

			task_watchdog_feed();
			advance_deadline(&next_10s_deadline, PERIOD_10S_MS, now);
		}

		/* Event chain: every 10 inferences -> queue comm worker. */
		if (app_state.infer_window_ready) {
			uint32_t t0 = k_uptime_get_32();
			task_sensor_sample();
			uint32_t t1 = k_uptime_get_32();
			log_task_timing("task_sensor_sample(pre_lora)", now, t0, t1);
			LOG_INF("Inference event(10): queue GPS->LoRa worker");
			task_inference_snapshot_uplink_window();
			task_comm_request_uplink();
		}

		now = k_uptime_get_32();
		uint32_t next_deadline = active_window_deadline;
		next_deadline = min_u32(next_deadline, next_10s_deadline);
		uint32_t sleep_ms = ms_until(now, next_deadline);
		if (sleep_ms > IDLE_SLEEP_MAX_MS) {
			sleep_ms = IDLE_SLEEP_MAX_MS;
		}

		if (sleep_ms > 0U) {
			k_msleep(sleep_ms);
		} else {
			k_yield();
		}
		
		// k_msleep(5000);
	}

	return 0;
}
