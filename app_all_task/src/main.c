#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/device.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/state.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/drivers/hwinfo.h>

#include "tasks.h"
#include "app_feature_flags.h"
#include "retained_state.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define APP_BOOT_COLD_POWERCYCLE_MS 1000U
#define APP_DEEP_SLEEP_WAKE_SEC     (1U * 60U * 60U)
// #define APP_DEEP_SLEEP_WAKE_SEC     (30U)
#define APP_BOOT_GPS_SYNC_TIMEOUT_MS (3U * 60U * 1000U)
#define APP_BOOT_GPS_RETRY_DELAY_MS  5000U

#define SLOT1_AREA_ID              FIXED_PARTITION_ID(slot1_partition)
#define APP_STARTUP_I2C_SCAN_ENABLED 0

static bool app_pm_boot_guard_locked;

static void app_pm_boot_guard_lock(void)
{
#if IS_ENABLED(CONFIG_PM)
	if (app_pm_boot_guard_locked) {
		return;
	}

	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	app_pm_boot_guard_locked = true;
	LOG_INF("[PWR] Boot guard: lock all SUSPEND_TO_IDLE substates during init");
#endif
}

static void app_pm_boot_guard_unlock(void)
{
#if IS_ENABLED(CONFIG_PM)
	if (!app_pm_boot_guard_locked) {
		return;
	}

	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	app_pm_boot_guard_locked = false;
	LOG_INF("[PWR] Boot guard: unlock SUSPEND_TO_IDLE substates (PM active)");
#endif
}

static const char *swap_type_to_str(int swap_type)
{
	switch (swap_type) {
	case BOOT_SWAP_TYPE_NONE:
		return "none";
	case BOOT_SWAP_TYPE_TEST:
		return "test";
	case BOOT_SWAP_TYPE_PERM:
		return "perm";
	case BOOT_SWAP_TYPE_REVERT:
		return "revert";
	case BOOT_SWAP_TYPE_FAIL:
		return "fail";
	default:
		return "unknown";
	}
}

static void log_boot_self_check(void)
{
	uint8_t active_slot = boot_fetch_active_slot();
	int swap_type = mcuboot_swap_type();
	bool img_confirmed = boot_is_img_confirmed();
	struct mcuboot_img_header slot1_hdr;
	int ret = boot_read_bank_header((uint8_t)SLOT1_AREA_ID, &slot1_hdr, sizeof(slot1_hdr));

	if (swap_type >= 0) {
		LOG_INF("Boot self-check: active_slot=%u swap_type=%d(%s) confirmed=%s",
			(unsigned int)active_slot,
			swap_type,
			swap_type_to_str(swap_type),
			img_confirmed ? "yes" : "no");
	} else {
		LOG_WRN("Boot self-check: active_slot=%u swap_type read failed: %d confirmed=%s",
			(unsigned int)active_slot,
			swap_type,
			img_confirmed ? "yes" : "no");
	}

	if (ret == 0) {
		LOG_INF("Boot self-check: slot1 version=%u.%u.%u+%u size=%u",
			(unsigned int)slot1_hdr.h.v1.sem_ver.major,
			(unsigned int)slot1_hdr.h.v1.sem_ver.minor,
			(unsigned int)slot1_hdr.h.v1.sem_ver.revision,
			(unsigned int)slot1_hdr.h.v1.sem_ver.build_num,
			(unsigned int)slot1_hdr.h.v1.image_size);
	} else {
		LOG_WRN("Boot self-check: slot1 header read failed: %d", ret);
	}
}

static void enter_deep_sleep_or_simulate(void)
{
	int alarm_ret = task_rtc_set_alarm_in_seconds(APP_DEEP_SLEEP_WAKE_SEC);
	LOG_INF("[PWR] low-bat recovery RTC alarm ret=%d", alarm_ret);
	if (alarm_ret < 0) {
		LOG_ERR("[PWR] low-bat recovery abort: rtc alarm setup failed, skip sys_poweroff");
		return;
	}

	if (APP_DEEP_SLEEP_ENABLED && !APP_DEEP_SLEEP_SIMULATE_ENABLED) {
		LOG_INF("[PWR] Entering deep sleep by sys_poweroff() -> STM32U5 shutdown");
		int prep_ret = power_ctrl_prepare_deep_sleep_all();
		if (prep_ret < 0) {
			LOG_ERR("[PWR] low-bat recovery abort: hardware prepare failed: %d, skip sys_poweroff",
				prep_ret);
			return;
		}
		task_rtc_prepare_shutdown_wakeup_route();
		k_msleep(200);
		sys_poweroff();
		return;
	}

	LOG_WRN("[PWR] Deep sleep simulate enabled; sleeping %u sec",
		(unsigned int)APP_DEEP_SLEEP_WAKE_SEC);
	k_sleep(K_SECONDS(APP_DEEP_SLEEP_WAKE_SEC));
}

static bool is_gps_time_legal(void)
{
	uint32_t now_epoch = 0U;
	struct retained_state_v1 retained;

	if (task_rtc_get_epoch_local(&now_epoch) < 0) {
		return false;
	}

	if (retained_state_load(&retained) < 0) {
		return false;
	}

	if (retained.last_gps_sync_epoch == 0U) {
		return false;
	}

	return now_epoch >= retained.last_gps_sync_epoch;
}

static void ensure_boot_gps_time_blocking(void)
{
	if (APP_MICROPHONE_DEBUG_SKIP_GPS_LORA_ENABLED) {
		LOG_INF("[BOOT] microphone debug mode: skip blocking GPS time sync");
		return;
	}

	uint32_t start_ms = k_uptime_get_32();

	while ((k_uptime_get_32() - start_ms) < APP_BOOT_GPS_SYNC_TIMEOUT_MS) {
		bool rtc_synced = false;
		uint32_t elapsed_ms = k_uptime_get_32() - start_ms;
		uint32_t remain_ms = APP_BOOT_GPS_SYNC_TIMEOUT_MS - elapsed_ms;

		LOG_INF("[BOOT] GPS time sync attempt start (remain=%u ms)",
			(unsigned int)remain_ms);
		if (task_gps_acquire_with_timeout_main_alive(remain_ms, &rtc_synced) && rtc_synced &&
		    is_gps_time_legal()) {
			LOG_INF("[BOOT] GPS legal time ready");
			return;
		}

		if ((k_uptime_get_32() - start_ms) >= APP_BOOT_GPS_SYNC_TIMEOUT_MS) {
			break;
		}

		LOG_WRN("[BOOT] GPS time sync not ready, retry in %u ms",
			(unsigned int)APP_BOOT_GPS_RETRY_DELAY_MS);
		k_sleep(K_MSEC(APP_BOOT_GPS_RETRY_DELAY_MS));
	}

	LOG_ERR("[BOOT] GPS time sync failed within %u ms",
		(unsigned int)APP_BOOT_GPS_SYNC_TIMEOUT_MS);
}

static bool app_boot_is_shutdown_wake_reboot(bool reset_cause_valid, uint32_t reset_cause)
{
	return reset_cause_valid && ((reset_cause & RESET_LOW_POWER_WAKE) != 0U);
}

int main(void)
{
	// k_sleep(K_SECONDS(3));
	// (void)task_rtc_init();

	// // 直接进入休眠，不进入正常的active运行态，来验证RTC唤醒和低电压唤醒功能
	// enter_deep_sleep_or_simulate();

	app_pm_boot_guard_lock();

	uint32_t reset_cause = 0U;
	bool reset_cause_valid = (hwinfo_get_reset_cause(&reset_cause) == 0);
	if (reset_cause_valid) {
		hwinfo_clear_reset_cause();
	}

	LOG_INF("FW version: %u.%u.%u",
		(unsigned int)app_state.fw_version_major,
		(unsigned int)app_state.fw_version_minor,
		(unsigned int)app_state.fw_version_patch);
	LOG_INF("Feature flags: fsm=%d deep=%d retention=%d deep_sim=%d",
		APP_LOW_POWER_FSM_ENABLED,
		APP_DEEP_SLEEP_ENABLED,
		APP_RETENTION_ENABLED,
		APP_DEEP_SLEEP_SIMULATE_ENABLED);
	LOG_INF("Debug flags: mic_skip_gps_lora=%d",
		APP_MICROPHONE_DEBUG_SKIP_GPS_LORA_ENABLED);
	LOG_INF("Wake source flags: button=%d periodic=%d comm=%d rtc=%d sound=%d low_bat=%d",
		APP_WAKEUP_SRC_BUTTON_ENABLED,
		APP_WAKEUP_SRC_PERIODIC_ENABLED,
		APP_WAKEUP_SRC_COMM_ENABLED,
		APP_WAKEUP_SRC_RTC_ALARM_ENABLED,
		APP_WAKEUP_SRC_SOUND_ENABLED,
		APP_WAKEUP_SRC_LOW_BAT_ENABLED);
	if (reset_cause_valid) {
		LOG_INF("[BOOT] reset_cause=0x%08x low_power=%d shutdown_wake_reboot=%d",
			(unsigned int)reset_cause,
			(reset_cause & RESET_LOW_POWER_WAKE) != 0U ? 1 : 0,
			app_boot_is_shutdown_wake_reboot(reset_cause_valid, reset_cause) ? 1 : 0);
		LOG_INF("[BOOT] reset flags: pin=%d software=%d brownout=%d por=%d watchdog=%d debug=%d",
			(reset_cause & RESET_PIN) != 0U ? 1 : 0,
			(reset_cause & RESET_SOFTWARE) != 0U ? 1 : 0,
			(reset_cause & RESET_BROWNOUT) != 0U ? 1 : 0,
			(reset_cause & RESET_POR) != 0U ? 1 : 0,
			(reset_cause & RESET_WATCHDOG) != 0U ? 1 : 0,
			(reset_cause & RESET_DEBUG) != 0U ? 1 : 0);
	}
	log_boot_self_check();

	int confirm_ret = boot_write_img_confirmed();
	if (confirm_ret == 0) {
		LOG_INF("MCUboot image confirmed");
	} else {
		LOG_WRN("MCUboot confirm skipped/failed: %d", confirm_ret);
	}

	if (app_boot_is_shutdown_wake_reboot(reset_cause_valid, reset_cause)) {
		LOG_INF("[BOOT] Detected RTC/deep-sleep shutdown wake reboot; boot flow restarts from main()");
	}

	/* Cold power-cycle before codec/mic init to discharge
	 * v_periph rail and start ADC3101 from a clean state.
	 */
	(void)power_ctrl_vperiph_off();
	k_msleep(APP_BOOT_COLD_POWERCYCLE_MS);
	(void)power_ctrl_vperiph_on();
	(void)task_status_led_init();

	if (task_ina3221_init() < 0) {
		LOG_ERR("INA3221 worker init failed");
	}

	if (task_ina3221_read_now() == 0) {
		int32_t batt_mv = (int32_t)(app_state.ina_ch2_voltage_v * 1000.0f);
		LOG_INF("[BOOT] battery after wake = %d mV", (int)batt_mv);
		if (batt_mv < APP_LOW_BATTERY_MV) {
			LOG_WRN("[BOOT] battery still low after wake, re-enter shutdown");
			enter_deep_sleep_or_simulate();
			return 0;
		}
	}

	(void)task_rtc_init();

	if (APP_RETENTION_ENABLED) {
		int retained_ret = retained_state_init_or_reset();
		if (retained_ret < 0) {
			LOG_WRN("[BOOT] retained_state_init_or_reset failed: %d", retained_ret);
		}
	}
	task_storage_sd_fuse_init();

	LOG_INF("[BOOT] defer synchronous SD mount until watchdog is active");

	(void)task_comm_init();

	if (APP_WAKEUP_SRC_BUTTON_ENABLED) {
		(void)task_button_init();
	} else {
		LOG_INF("[PWR] button wakeup source disabled by config");
	}

	if (APP_WAKEUP_SRC_SOUND_ENABLED) {
		(void)task_sound_wakeup_init();
	} else {
		LOG_INF("[PWR] sound wakeup source disabled by config");
	}

	uint32_t mic_init_start_ms = k_uptime_get_32();
	
	int mic_init_ret = task_microphone_init();

	uint32_t mic_init_cost_ms = k_uptime_get_32() - mic_init_start_ms;
	LOG_INF("[BOOT] microphone init ret=%d cost=%u ms",
		mic_init_ret,
		(unsigned int)mic_init_cost_ms);
	if (mic_init_ret < 0) {
		LOG_ERR("Mic init failed");
	}

	task_sensor_sample();

	
	// if (task_imu_init() < 0) {
	// 	LOG_ERR("IMU init failed");
	// }
	// task_imu_sample();

	if (APP_MICROPHONE_DEBUG_SKIP_GPS_LORA_ENABLED) {
		LOG_INF("[BOOT] microphone debug mode: skip startup LoRaWAN connect");
	} else if (task_lorawan_connect() != 0) {
		LOG_WRN("Startup LoRaWAN connect failed; will retry during runtime");
	}

	int wdt_ret = task_watchdog_init();
	if (wdt_ret == 0) {
		LOG_INF("[BOOT] All devices initialized; watchdog active");
	} else {
		LOG_WRN("[BOOT] All devices initialized; watchdog inactive (init ret=%d)", wdt_ret);
	}

	if (wdt_ret == 0) {
		task_watchdog_mark_main_alive();
		uint32_t sd_init_start_ms = k_uptime_get_32();
		task_sd_ensure_mounted();
		uint32_t sd_init_cost_ms = k_uptime_get_32() - sd_init_start_ms;
		task_watchdog_mark_main_alive();
		LOG_INF("[BOOT] watchdog-protected SD ensure mounted cost=%u ms mounted=%u fuse=%u",
			(unsigned int)sd_init_cost_ms,
			(unsigned int)(app_state.sd_mounted ? 1U : 0U),
			(unsigned int)(task_storage_sd_fuse_active() ? 1U : 0U));
		task_dfu_check_and_apply();
		task_watchdog_mark_main_alive();
	} else {
		LOG_WRN("[BOOT] skip synchronous SD mount/DFU because watchdog is inactive");
	}

	uint32_t now_epoch = 0U;
	if (task_rtc_get_epoch_local(&now_epoch) < 0) {
		LOG_WRN("[PWR] GPS resync check failed to get RTC local epoch");
		ensure_boot_gps_time_blocking();
	} else {
		LOG_INF("[BOOT] RTC local epoch read at startup: %u", (unsigned int)now_epoch);
	}

	task_status_led_event(STATUS_LED_BOOT_OK);
	k_msleep(150);

	if (power_fsm_init() < 0) {
		LOG_ERR("power_fsm_init failed");
	}
	LOG_INF("Power FSM scheduler started");

	app_pm_boot_guard_unlock();
	LOG_INF("[BOOT] Init complete, entering normal runtime");

	while (1) {
		task_watchdog_mark_main_alive();
		power_fsm_tick();
		task_watchdog_mark_main_alive();
		k_timeout_t timeout = power_fsm_next_wait_timeout();
		uint32_t timeout_ms = k_ticks_to_ms_floor32(timeout.ticks);
		LOG_INF("Power FSM waiting for event or timeout (%u ms)",
			(unsigned int)timeout_ms);
		power_fsm_wait_for_event(timeout);
	}

	return 0;
}
