#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/poweroff.h>

#include "app_feature_flags.h"
#include "tasks.h"

LOG_MODULE_REGISTER(app_deep_sleep, LOG_LEVEL_INF);

#define DEEP_SLEEP_MIN_WAKE_SEC 1U
#define DEEP_SLEEP_SHUTDOWN_SETTLE_MS 200U
#define DEEP_SLEEP_SIM_FEED_SLICE_MS 1000U

static void deep_sleep_simulated_wait(uint32_t wake_sec)
{
	uint32_t remain_ms = wake_sec * 1000U;

	while (remain_ms > 0U) {
		uint32_t slice_ms = (remain_ms > DEEP_SLEEP_SIM_FEED_SLICE_MS) ?
			DEEP_SLEEP_SIM_FEED_SLICE_MS : remain_ms;

		task_watchdog_mark_main_alive();
		task_watchdog_feed();
		k_msleep(slice_ms);
		remain_ms -= slice_ms;
	}
}

int app_deep_sleep_enter(uint32_t wake_sec, const char *reason)
{
	const char *entry_reason = (reason != NULL) ? reason : "unspecified";

	if (wake_sec == 0U) {
		LOG_WRN("[PWR] deep_sleep(%s): wake_sec=0, clamp to %u sec",
			entry_reason, (unsigned int)DEEP_SLEEP_MIN_WAKE_SEC);
		wake_sec = DEEP_SLEEP_MIN_WAKE_SEC;
	}

	int alarm_ret = task_rtc_set_alarm_in_seconds(wake_sec);
	LOG_INF("[PWR] deep_sleep(%s): rtc_alarm ret=%d wake=%u sec",
		entry_reason, alarm_ret, (unsigned int)wake_sec);
	if (alarm_ret < 0) {
		LOG_ERR("[PWR] deep_sleep(%s): abort, rtc alarm setup failed: %d",
			entry_reason, alarm_ret);
		return alarm_ret;
	}

	if (!APP_DEEP_SLEEP_ENABLED || APP_DEEP_SLEEP_SIMULATE_ENABLED) {
		LOG_WRN("[PWR] deep_sleep(%s): simulate enabled, wait %u sec",
			entry_reason, (unsigned int)wake_sec);
		deep_sleep_simulated_wait(wake_sec);
		return 0;
	}

	int prep_ret = power_ctrl_prepare_deep_sleep_all();
	LOG_INF("[PWR] deep_sleep(%s): hardware_prepare ret=%d", entry_reason, prep_ret);
	if (prep_ret < 0) {
		LOG_ERR("[PWR] deep_sleep(%s): abort, hardware prepare failed: %d",
			entry_reason, prep_ret);
		return prep_ret;
	}

	int route_ret = task_rtc_prepare_shutdown_wakeup_route();
	LOG_INF("[PWR] deep_sleep(%s): rtc_wake_route ret=%d", entry_reason, route_ret);
	if (route_ret < 0) {
		LOG_ERR("[PWR] deep_sleep(%s): abort, RTC wake route failed: %d",
			entry_reason, route_ret);
		return route_ret;
	}

	LOG_WRN("[PWR] deep_sleep(%s): final state, alarm armed, wake flags cleared, rails prepared",
		entry_reason);
	k_msleep(DEEP_SLEEP_SHUTDOWN_SETTLE_MS);

	LOG_INF("[PWR] Entering deep sleep by sys_poweroff() -> STM32 shutdown");
	sys_poweroff();

	LOG_ERR("[PWR] deep_sleep(%s): sys_poweroff returned unexpectedly", entry_reason);
	return -EIO;
}
