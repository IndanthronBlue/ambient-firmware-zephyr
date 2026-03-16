/*
 * RTC Task - wraps the Zephyr RTC driver following the official sample pattern.
 *
 * Public API (declared in tasks.h):
 *   task_rtc_init()          - verify device is ready
 *   task_rtc_set_time()      - write an rtc_time struct
 *   task_rtc_get_time()      - read current time and log it
 *   task_rtc_set_from_gps()  - sync RTC from app_state GPS time fields
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>

#include <string.h>

#include "tasks.h"

LOG_MODULE_REGISTER(rtc_task, LOG_LEVEL_INF);

/* Bind RTC device using the same alias as the official Zephyr RTC sample */
static const struct device *const rtc_dev = DEVICE_DT_GET(DT_ALIAS(rtc));

/* --------------------------------------------------------------------------
 * task_rtc_init
 * -------------------------------------------------------------------------- */
int task_rtc_init(void)
{
	if (!device_is_ready(rtc_dev)) {
		LOG_ERR("RTC device not ready");
		return -ENODEV;
	}
	LOG_INF("RTC device ready");
	return 0;
}

/* --------------------------------------------------------------------------
 * task_rtc_set_time
 * -------------------------------------------------------------------------- */
int task_rtc_set_time(const struct rtc_time *tm)
{
	if (!device_is_ready(rtc_dev)) {
		LOG_ERR("RTC device not ready");
		return -ENODEV;
	}

	int ret = rtc_set_time(rtc_dev, tm);
	if (ret < 0) {
		LOG_ERR("Cannot write date time: %d", ret);
	}

    LOG_INF("RTC date and time set to: %04d-%02d-%02d %02d:%02d:%02d",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);
	return ret;
}

/* --------------------------------------------------------------------------
 * task_rtc_get_time
 * -------------------------------------------------------------------------- */
int task_rtc_get_time(struct rtc_time *tm)
{
	int ret = rtc_get_time(rtc_dev, tm);
	if (ret < 0) {
		LOG_ERR("Cannot read date time: %d", ret);
		return ret;
	}

	LOG_INF("RTC date and time: %04d-%02d-%02d %02d:%02d:%02d",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
	return 0;
}

/* --------------------------------------------------------------------------
 * task_rtc_set_from_gps
 * Reads app_state GPS time fields and programs the RTC, then reads back to
 * confirm - mirrors the set_date_time / get_date_time pattern of the sample.
 * -------------------------------------------------------------------------- */
int task_rtc_set_from_gps(void)
{
	if (!app_state.time_valid || app_state.time_year < 2000U) {
		LOG_WRN("GPS time not valid, skip RTC sync");
		return -EINVAL;
	}

	struct rtc_time tm = {
		.tm_year = (int)app_state.time_year - 1900,
		.tm_mon  = (int)app_state.time_month - 1,
		.tm_mday = (int)app_state.time_day,
		.tm_hour = (int)app_state.time_hour,
		.tm_min  = (int)app_state.time_minute,
		.tm_sec  = (int)app_state.time_second,
	};

	int ret = task_rtc_set_time(&tm);
	if (ret < 0) {
		return ret;
	}

	struct rtc_time validated_tm;
	memset(&validated_tm, 0, sizeof(validated_tm));
	return task_rtc_get_time(&validated_tm);
}
