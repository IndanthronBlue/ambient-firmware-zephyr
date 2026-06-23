/*
 * RTC Task - wraps the Zephyr RTC driver following the official sample pattern.
 *
 * Public API (declared in tasks.h):
 *   task_rtc_init()          - verify device is ready
 *   task_rtc_set_time()      - write an rtc_time struct
 *   task_rtc_get_time()      - read current time and log it
 *   task_rtc_set_from_gps()  - sync RTC from app_state France-local GPS time fields
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include <string.h>
#include <soc.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rtc.h>

#include "tasks.h"

LOG_MODULE_REGISTER(rtc_task, LOG_LEVEL_INF);

#define RTC_DEFAULT_YEAR   2000
#define RTC_DEFAULT_MONTH  1
#define RTC_DEFAULT_DAY    2
#define RTC_DEFAULT_HOUR   0
#define RTC_DEFAULT_MINUTE 0
#define RTC_DEFAULT_SECOND 0
#define RTC_DEFAULT_WDAY   0
#define RTC_DEFAULT_SEED_MAGIC 0x52544353U /* "RTCS" */
#define RTC_DEFAULT_SEED_REG   LL_RTC_BKP_DR1

#if defined(CONFIG_SOC_SERIES_STM32U5X)
#define RTC_SEED_SET_REG(rtc, reg, val) LL_RTC_BKP_SetRegister((rtc), (reg), (val))
#define RTC_SEED_GET_REG(rtc, reg)      LL_RTC_BKP_GetRegister((rtc), (reg))
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
#define RTC_SEED_SET_REG(rtc, reg, val) LL_RTC_BAK_SetRegister((rtc), (reg), (val))
#define RTC_SEED_GET_REG(rtc, reg)      LL_RTC_BAK_GetRegister((rtc), (reg))
#else
#error "Unsupported STM32 series for RTC backup register API"
#endif

/* Bind RTC device using the same alias as the official Zephyr RTC sample */
static const struct device *const rtc_dev = DEVICE_DT_GET(DT_ALIAS(rtc));
static int64_t rtc_default_seed_uptime_ms = -1;
static bool rtc_storage_default_fallback_logged;

#if defined(CONFIG_RTC_ALARM)
static void rtc_alarm_cb(const struct device *dev, uint16_t id, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);
	ARG_UNUSED(user_data);

	power_fsm_request_wakeup(POWER_WAKE_SRC_RTC_ALARM);
}
#endif

static bool rtc_is_leap_year(int year)
{
	if ((year % 400) == 0) {
		return true;
	}

	if ((year % 100) == 0) {
		return false;
	}

	return (year % 4) == 0;
}

static bool rtc_datetime_to_epoch_local(const struct rtc_time *tm, uint32_t *epoch)
{
	if ((tm == NULL) || (epoch == NULL)) {
		return false;
	}

	int year = tm->tm_year + 1900;
	int month = tm->tm_mon + 1;
	int day = tm->tm_mday;

	if ((year < 1970) || (month < 1) || (month > 12) ||
	    (day < 1) || (day > 31) ||
	    (tm->tm_hour < 0) || (tm->tm_hour > 23) ||
	    (tm->tm_min < 0) || (tm->tm_min > 59) ||
	    (tm->tm_sec < 0) || (tm->tm_sec > 59)) {
		return false;
	}

	static const uint8_t month_days[12] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};

	uint8_t max_day = month_days[month - 1];
	if ((month == 2) && rtc_is_leap_year(year)) {
		max_day = 29;
	}

	if (day > max_day) {
		return false;
	}

	uint32_t days = 0U;
	for (int y = 1970; y < year; y++) {
		days += rtc_is_leap_year(y) ? 366U : 365U;
	}

	for (int m = 1; m < month; m++) {
		days += month_days[m - 1];
		if ((m == 2) && rtc_is_leap_year(year)) {
			days += 1U;
		}
	}

	days += (uint32_t)(day - 1);

	uint64_t seconds = ((uint64_t)days * 86400ULL) +
		((uint64_t)tm->tm_hour * 3600ULL) +
		((uint64_t)tm->tm_min * 60ULL) +
		(uint64_t)tm->tm_sec;

	if (seconds > UINT32_MAX) {
		return false;
	}

	*epoch = (uint32_t)seconds;
	return true;
}

static bool rtc_epoch_to_datetime_local(uint32_t epoch, struct rtc_time *tm)
{
	if (tm == NULL) {
		return false;
	}

	uint32_t days = epoch / 86400U;
	uint32_t rem = epoch % 86400U;

	tm->tm_hour = (int)(rem / 3600U);
	rem %= 3600U;
	tm->tm_min = (int)(rem / 60U);
	tm->tm_sec = (int)(rem % 60U);

	int year = 1970;
	while (true) {
		uint32_t ydays = rtc_is_leap_year(year) ? 366U : 365U;
		if (days < ydays) {
			break;
		}
		days -= ydays;
		year++;
	}

	static const uint8_t month_days[12] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};

	int month = 0;
	while (month < 12) {
		uint32_t mdays = month_days[month];
		if ((month == 1) && rtc_is_leap_year(year)) {
			mdays += 1U;
		}

		if (days < mdays) {
			break;
		}

		days -= mdays;
		month++;
	}

	tm->tm_year = year - 1900;
	tm->tm_mon = month;
	tm->tm_mday = (int)days + 1;
	tm->tm_wday = -1;
	tm->tm_yday = -1;
	tm->tm_isdst = -1;
	tm->tm_nsec = 0;

	return true;
}

static bool rtc_compute_wday(struct rtc_time *tm)
{
	if (tm == NULL) {
		return false;
	}

	uint32_t epoch = 0U;
	if (!rtc_datetime_to_epoch_local(tm, &epoch)) {
		return false;
	}

	/* 1970-01-01 was Thursday (4 with Sunday=0). */
	tm->tm_wday = (int)((epoch / 86400U + 4U) % 7U);
	return true;
}

static void rtc_fill_default_time(struct rtc_time *tm)
{
	if (tm == NULL) {
		return;
	}

	tm->tm_year = RTC_DEFAULT_YEAR - 1900;
	tm->tm_mon = RTC_DEFAULT_MONTH - 1;
	tm->tm_mday = RTC_DEFAULT_DAY;
	tm->tm_hour = RTC_DEFAULT_HOUR;
	tm->tm_min = RTC_DEFAULT_MINUTE;
	tm->tm_sec = RTC_DEFAULT_SECOND;
	tm->tm_wday = RTC_DEFAULT_WDAY;
	tm->tm_yday = -1;
	tm->tm_isdst = -1;
	tm->tm_nsec = 0;
}

static void rtc_default_seed_mark(bool enabled)
{
	LL_PWR_EnableBkUpAccess();
	RTC_SEED_SET_REG(RTC, RTC_DEFAULT_SEED_REG, enabled ? RTC_DEFAULT_SEED_MAGIC : 0U);
}

static bool rtc_default_seed_is_marked(void)
{
	LL_PWR_EnableBkUpAccess();
	return RTC_SEED_GET_REG(RTC, RTC_DEFAULT_SEED_REG) == RTC_DEFAULT_SEED_MAGIC;
}

static bool rtc_is_default_or_uninitialized(const struct rtc_time *tm)
{
	if (tm == NULL) {
		return true;
	}

	if (rtc_default_seed_is_marked()) {
		return true;
	}

	uint32_t epoch = 0U;
	return !rtc_datetime_to_epoch_local(tm, &epoch);
}

static int rtc_ensure_default_time_if_needed(bool *used_default)
{
	if (used_default != NULL) {
		*used_default = false;
	}

	if (!device_is_ready(rtc_dev)) {
		return -ENODEV;
	}

	struct rtc_time tm;
	memset(&tm, 0, sizeof(tm));
	int ret = rtc_get_time(rtc_dev, &tm);
	if ((ret == 0) && !rtc_is_default_or_uninitialized(&tm)) {
		return 0;
	}

	struct rtc_time default_tm;
	memset(&default_tm, 0, sizeof(default_tm));
	rtc_fill_default_time(&default_tm);
	if (!rtc_compute_wday(&default_tm)) {
		LOG_ERR("RTC default time weekday compute failed");
		return -EINVAL;
	}

	ret = rtc_set_time(rtc_dev, &default_tm);
	if (ret < 0) {
		LOG_ERR("RTC default time seed failed: %d", ret);
		return ret;
	}

	rtc_default_seed_mark(true);
	rtc_default_seed_uptime_ms = k_uptime_get();

	if (used_default != NULL) {
		*used_default = true;
	}

	LOG_WRN("RTC was unset/invalid, seeded default time: %04d-%02d-%02d %02d:%02d:%02d",
		RTC_DEFAULT_YEAR,
		RTC_DEFAULT_MONTH,
		RTC_DEFAULT_DAY,
		RTC_DEFAULT_HOUR,
		RTC_DEFAULT_MINUTE,
		RTC_DEFAULT_SECOND);
	return 0;
}

bool task_rtc_time_is_real(const struct rtc_time *tm)
{
	return !rtc_is_default_or_uninitialized(tm);
}

/* --------------------------------------------------------------------------
 * task_rtc_init
 * -------------------------------------------------------------------------- */
int task_rtc_init(void)
{
	if (!device_is_ready(rtc_dev)) {
		LOG_ERR("RTC device not ready");
		return -ENODEV;
	}

	bool used_default = false;
	int default_ret = rtc_ensure_default_time_if_needed(&used_default);
	if (default_ret < 0) {
		LOG_ERR("RTC default seed check failed: %d", default_ret);
		return default_ret;
	}

#if IS_ENABLED(CONFIG_PM_DEVICE)
	bool pm_wake_capable = pm_device_wakeup_is_capable(rtc_dev);
	bool pm_wake_enabled = false;
	// if (pm_wake_capable) {
	// 	pm_wake_enabled = pm_device_wakeup_enable(rtc_dev, true);
	// }
	pm_wake_enabled = pm_device_wakeup_enable(rtc_dev, true);
	LOG_INF("RTC PM wake capability: capable=%d enabled=%d",
		pm_wake_capable ? 1 : 0,
		pm_wake_enabled ? 1 : 0);
#endif

	LOG_INF("RTC deep-sleep wake path: alarm_api=%d dt_wakeup_source=%d",
		IS_ENABLED(CONFIG_RTC_ALARM) ? 1 : 0,
		DT_NODE_HAS_PROP(DT_ALIAS(rtc), wakeup_source) ? 1 : 0);
	if (used_default) {
		LOG_WRN("RTC currently uses seeded default time; GPS sync should replace it later");
	}
	LOG_INF("RTC device ready; RTC alarm wake works with sys_poweroff/shutdown, not PM standby");
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
	if (tm == NULL) {
		return -EINVAL;
	}

	struct rtc_time write_tm = *tm;
	if ((write_tm.tm_wday < 0) || (write_tm.tm_wday > 6)) {
		if (!rtc_compute_wday(&write_tm)) {
			LOG_ERR("Cannot compute RTC weekday from date time");
			return -EINVAL;
		}
	}

	int ret = rtc_set_time(rtc_dev, &write_tm);
	if (ret < 0) {
		LOG_ERR("Cannot write date time: %d", ret);
		return ret;
	}

	rtc_default_seed_mark(false);

    LOG_INF("RTC France local date and time set to: %04d-%02d-%02d %02d:%02d:%02d",
        write_tm.tm_year + 1900, write_tm.tm_mon + 1, write_tm.tm_mday,
        write_tm.tm_hour, write_tm.tm_min, write_tm.tm_sec);
	return ret;
}

/* --------------------------------------------------------------------------
 * task_rtc_get_time
 * -------------------------------------------------------------------------- */
int task_rtc_get_time(struct rtc_time *tm)
{
	if (!device_is_ready(rtc_dev)) {
		LOG_ERR("RTC device not ready");
		return -ENODEV;
	}

	if (tm == NULL) {
		return -EINVAL;
	}

	int ret = rtc_get_time(rtc_dev, tm);
	if (ret < 0) {
		LOG_ERR("Cannot read date time: %d", ret);
		return ret;
	}

	LOG_INF("RTC France local date and time: %04d-%02d-%02d %02d:%02d:%02d",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
	return 0;
}

static int rtc_fill_default_storage_time(struct rtc_time *tm)
{
	if (tm == NULL) {
		return -EINVAL;
	}

	rtc_fill_default_time(tm);

	uint32_t default_epoch = 0U;
	if (!rtc_datetime_to_epoch_local(tm, &default_epoch)) {
		return -EINVAL;
	}

	int64_t base_ms = rtc_default_seed_uptime_ms;
	int64_t now_ms = k_uptime_get();
	if ((base_ms < 0) || (now_ms < base_ms)) {
		base_ms = 0;
	}

	uint64_t storage_epoch = (uint64_t)default_epoch +
		(uint64_t)((now_ms - base_ms) / 1000);
	if (storage_epoch > UINT32_MAX) {
		return -EINVAL;
	}

	if (!rtc_epoch_to_datetime_local((uint32_t)storage_epoch, tm)) {
		return -EINVAL;
	}

	if (!rtc_compute_wday(tm)) {
		return -EINVAL;
	}

	return 0;
}

int task_rtc_get_time_for_storage(struct rtc_time *tm, bool *is_default_seed)
{
	if (tm == NULL) {
		return -EINVAL;
	}

	if (is_default_seed != NULL) {
		*is_default_seed = false;
	}

	if (!device_is_ready(rtc_dev)) {
		return -ENODEV;
	}

	(void)memset(tm, 0, sizeof(*tm));
	int ret = rtc_get_time(rtc_dev, tm);
	if (ret < 0) {
		if ((ret == -ENODATA) && rtc_default_seed_is_marked()) {
			ret = rtc_fill_default_storage_time(tm);
			if (ret < 0) {
				return ret;
			}
			if (is_default_seed != NULL) {
				*is_default_seed = true;
			}
			if (!rtc_storage_default_fallback_logged) {
				LOG_WRN("RTC readback unavailable, synthesizing storage time from boot-seeded default RTC");
				rtc_storage_default_fallback_logged = true;
			}
			return 0;
		}

		return ret;
	}

	uint32_t epoch = 0U;
	if (!rtc_datetime_to_epoch_local(tm, &epoch)) {
		if (rtc_default_seed_is_marked()) {
			ret = rtc_fill_default_storage_time(tm);
			if (ret < 0) {
				return ret;
			}
			if (is_default_seed != NULL) {
				*is_default_seed = true;
			}
			if (!rtc_storage_default_fallback_logged) {
				LOG_WRN("RTC readback invalid, synthesizing storage time from boot-seeded default RTC");
				rtc_storage_default_fallback_logged = true;
			}
			return 0;
		}

		return -EINVAL;
	}

	if (is_default_seed != NULL) {
		*is_default_seed = rtc_default_seed_is_marked();
	}

	return 0;
}

int task_rtc_get_epoch_local(uint32_t *local_epoch)
{
	if (local_epoch == NULL) {
		return -EINVAL;
	}

	struct rtc_time tm;
	memset(&tm, 0, sizeof(tm));

	int ret = rtc_get_time(rtc_dev, &tm);
	if (ret < 0) {
		LOG_ERR("Cannot read date time: %d", ret);
		return ret;
	}

	if (rtc_is_default_or_uninitialized(&tm)) {
		LOG_WRN("RTC date/time still using default/uninitialized seed");
		return -EINVAL;
	}

	if (!rtc_datetime_to_epoch_local(&tm, local_epoch)) {
		LOG_WRN("RTC date/time invalid for local epoch conversion");
		return -EINVAL;
	}

	return 0;
}

int task_rtc_set_alarm_in_seconds(uint32_t seconds_from_now)
{
#if !defined(CONFIG_RTC_ALARM)
	ARG_UNUSED(seconds_from_now);
	return -ENOTSUP;
#else
	const uint16_t required_mask = RTC_ALARM_TIME_MASK_MONTHDAY |
		RTC_ALARM_TIME_MASK_HOUR |
		RTC_ALARM_TIME_MASK_MINUTE |
		RTC_ALARM_TIME_MASK_SECOND;

	if (!device_is_ready(rtc_dev)) {
		return -ENODEV;
	}

	uint16_t supported_mask = 0U;
	int ret = rtc_alarm_get_supported_fields(rtc_dev, 0, &supported_mask);
	if (ret < 0) {
		LOG_ERR("rtc_alarm_get_supported_fields failed: %d", ret);
		return ret;
	}

	uint16_t mask = required_mask & supported_mask;
	if ((mask & required_mask) != required_mask) {
		LOG_ERR("RTC alarm required mask not supported: req=0x%04x sup=0x%04x use=0x%04x",
			(unsigned int)required_mask,
			(unsigned int)supported_mask,
			(unsigned int)mask);
		return -ENOTSUP;
	}

	bool used_default = false;
	int seed_ret = rtc_ensure_default_time_if_needed(&used_default);
	if (seed_ret < 0) {
		return seed_ret;
	}

	struct rtc_time now_tm;
	memset(&now_tm, 0, sizeof(now_tm));
	ret = rtc_get_time(rtc_dev, &now_tm);
	if (ret < 0) {
		if (used_default && (ret == -ENODATA)) {
			rtc_fill_default_time(&now_tm);
			if (!rtc_compute_wday(&now_tm)) {
				LOG_ERR("RTC local default snapshot invalid for alarm setup");
				return -EINVAL;
			}
			LOG_WRN("RTC readback unavailable right after default seed, using local seeded time snapshot for alarm setup");
		} else {
			LOG_ERR("Cannot read date time before alarm setup: %d", ret);
			return ret;
		}
	}

	uint32_t now_epoch = 0U;
	if (!rtc_datetime_to_epoch_local(&now_tm, &now_epoch)) {
		LOG_WRN("RTC date/time invalid for alarm setup");
		return -EINVAL;
	}

	uint32_t target_epoch = now_epoch + seconds_from_now;
	struct rtc_time alarm_tm;
	memset(&alarm_tm, 0, sizeof(alarm_tm));
	if (!rtc_epoch_to_datetime_local(target_epoch, &alarm_tm)) {
		return -EINVAL;
	}
	alarm_tm.tm_wday = (now_tm.tm_wday >= 0 && now_tm.tm_wday <= 6) ?
		((now_tm.tm_wday + (int)(seconds_from_now / 86400U)) % 7) : RTC_DEFAULT_WDAY;

	ret = rtc_alarm_set_time(rtc_dev, 0, 0, NULL);
	if ((ret < 0) && (ret != -ENOTSUP) && (ret != -EINVAL)) {
		LOG_WRN("rtc_alarm_set_time(cancel) failed: %d", ret);
	}

	ret = rtc_alarm_set_callback(rtc_dev, 0, rtc_alarm_cb, NULL);
	if (ret < 0) {
		LOG_ERR("rtc_alarm_set_callback failed: %d", ret);
		return ret;
	}

	ret = rtc_alarm_set_time(rtc_dev, 0, mask, &alarm_tm);
	if (ret < 0) {
		LOG_ERR("rtc_alarm_set_time failed: %d", ret);
		return ret;
	}

	LOG_INF("RTC alarm set +%u sec at %04d-%02d-%02d %02d:%02d:%02d",
		(unsigned int)seconds_from_now,
		alarm_tm.tm_year + 1900, alarm_tm.tm_mon + 1, alarm_tm.tm_mday,
		alarm_tm.tm_hour, alarm_tm.tm_min, alarm_tm.tm_sec);
	if (used_default) {
		LOG_WRN("RTC alarm based on seeded default time; GPS sync should replace RTC after reboot");
	}
	return 0;
#endif
}

/* --------------------------------------------------------------------------
 * task_rtc_set_from_gps
 * Reads app_state France-local GPS time fields and programs the RTC, then reads back to
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
		.tm_wday = -1,
		.tm_yday = -1,
		.tm_isdst = -1,
		.tm_nsec = 0,
	};

	int ret = task_rtc_set_time(&tm);
	if (ret < 0) {
		return ret;
	}

	struct rtc_time validated_tm;
	memset(&validated_tm, 0, sizeof(validated_tm));
	return task_rtc_get_time(&validated_tm);
}

void task_rtc_prepare_shutdown_wakeup_route(void)
{
#if defined(CONFIG_SOC_SERIES_STM32U5X)
	if (!device_is_ready(rtc_dev)) {
		LOG_WRN("RTC wake route skipped: RTC device not ready");
		return;
	}

	uint32_t cr = RTC->CR;
	bool alarm_a_enabled = ((cr & RTC_CR_ALRAE) != 0U) && ((cr & RTC_CR_ALRAIE) != 0U);

	if (!alarm_a_enabled) {
		LOG_WRN("RTC wake route skipped: AlarmA internal wakeup not enabled");
		return;
	}

	LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN7);
	LL_PWR_SetWakeUpPinSignal3Selection(LL_PWR_WAKEUP_PIN7);
	LL_PWR_ClearFlag_SB();
	LL_PWR_ClearFlag_WU();

	LOG_INF("RTC wake route configured: WKUP7 <- signal3 (RTC AlarmA)");
#else
	LOG_INF("RTC wake route prep is not required on this SoC series");
#endif
}
