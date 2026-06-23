/*
 * GPS Polling (Quectel L96 over I2C)
 * Ambient-style NMEA over I2C (addr 0x10), with dynamic delay:
 * - Double LF ("\n\n") -> 500ms backoff
 * - Else -> 5ms minimal backoff
 * Parses $GNRMC/$GPRMC for lat/lon and updates app_state
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <stdlib.h>
#include <soc.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rtc.h>

LOG_MODULE_REGISTER(gps_task, LOG_LEVEL_INF);

#include "tasks.h"
#include "app_feature_flags.h"
#include "retained_state.h"

/* Bind I2C1 device for GPS per board DTS */
static const struct device *gps_i2c = DEVICE_DT_GET(DT_NODELABEL(i2c1));

/* GPS I2C address from board DTS cam_m8q node, fallback to 0x10 */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(cam_m8q), okay)
#define GPS_ADDR DT_REG_ADDR(DT_NODELABEL(cam_m8q))
#else
#define GPS_ADDR 0x10
#endif

/* GPS power control is centralized in power_ctrl.c to ensure
 * v_periph and GPS enable sequencing stay consistent.
 */

/* Internal state */
static bool initialized = false;
static bool gps_powered = false;
static uint32_t init_fail_count = 0;
static uint32_t retry_deadline_ms = 0;
static uint8_t nmea_buf[255];
static char line_buf[128];
static size_t line_len = 0;
static bool gps_debug_skip_logged;
static bool gps_last_rtc_sync_ok;
K_MUTEX_DEFINE(gps_i2c1_mutex);

#define GPS_RETRY_5MIN_MS   (5U * 60U * 1000U)
#define GPS_RETRY_10MIN_MS  (10U * 60U * 1000U)
#define GPS_RETRY_30MIN_MS  (30U * 60U * 1000U)

static void gps_power_off(void);

static uint32_t gps_backoff_ms(uint32_t fail_count)
{
	if (fail_count <= 1U) {
		return GPS_RETRY_5MIN_MS;
	}
	if (fail_count == 2U) {
		return GPS_RETRY_10MIN_MS;
	}
	return GPS_RETRY_30MIN_MS;
}

static void gps_log_microphone_debug_skip(const char *operation)
{
	if (!gps_debug_skip_logged) {
		LOG_INF("Microphone debug mode enabled: GPS real operations are skipped");
		gps_debug_skip_logged = true;
	}

	LOG_DBG("Microphone debug mode: GPS %s skipped", operation);
}

void task_i2c1_lock(void)
{
	(void)k_mutex_lock(&gps_i2c1_mutex, K_FOREVER);
}

void task_i2c1_unlock(void)
{
	(void)k_mutex_unlock(&gps_i2c1_mutex);
}

static void gps_mark_failure(uint32_t now_ms, const char *reason)
{
	init_fail_count++;
	uint32_t delay_ms = gps_backoff_ms(init_fail_count);
	retry_deadline_ms = now_ms + delay_ms;
	LOG_WRN("GPS init/acquire failed (%s), retry in %u s",
		reason, (unsigned int)(delay_ms / 1000U));
}

static int gps_power_on(void)
{
	if (APP_MICROPHONE_DEBUG_SKIP_GPS_LORA_ENABLED) {
		gps_log_microphone_debug_skip("power on");
		return -ENOTSUP;
	}

    int ret = power_ctrl_gps_on();
    if (ret < 0) {
        return ret;
    }
	gps_powered = true;

    /* Wait GPS startup; keep longer for runtime re-power validation */
    k_msleep(2000);

	/* Initialize I2C (verify bus ready) */
	if (!device_is_ready(gps_i2c)) {
		LOG_ERR("GPS I2C1 device not ready");
		gps_power_off();
		return -ENODEV;
	}

    task_i2c1_lock();
    (void)i2c_recover_bus(gps_i2c);
    task_i2c1_unlock();
    return 0;
}

static void gps_power_off(void)
{
	initialized = false;
	if (APP_MICROPHONE_DEBUG_SKIP_GPS_LORA_ENABLED) {
		gps_log_microphone_debug_skip("power off");
		return;
	}

	if (!gps_powered) {
		return;
	}

	(void)power_ctrl_gps_off();
	gps_powered = false;
}

static bool gps_is_leap_year(int year)
{
	if ((year % 400) == 0) {
		return true;
	}

	if ((year % 100) == 0) {
		return false;
	}

	return (year % 4) == 0;
}

static uint8_t gps_days_in_month(int year, int month)
{
	static const uint8_t month_days[12] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};

	uint8_t days = month_days[month - 1];
	if ((month == 2) && gps_is_leap_year(year)) {
		days = 29;
	}

	return days;
}

static bool gps_datetime_valid(int year, int month, int day,
			       int hour, int minute, int second)
{
	if ((year < 2000) || (month < 1) || (month > 12) ||
	    (day < 1) || (hour < 0) || (hour > 23) ||
	    (minute < 0) || (minute > 59) ||
	    (second < 0) || (second > 59)) {
		return false;
	}

	return day <= gps_days_in_month(year, month);
}

static bool gps_datetime_to_epoch_utc(int year, int month, int day,
				      int hour, int minute, int second,
				      uint32_t *epoch)
{
	if ((epoch == NULL) ||
	    !gps_datetime_valid(year, month, day, hour, minute, second)) {
		return false;
	}

	uint32_t days = 0U;
	for (int y = 1970; y < year; y++) {
		days += gps_is_leap_year(y) ? 366U : 365U;
	}

	for (int m = 1; m < month; m++) {
		days += gps_days_in_month(year, m);
	}

	days += (uint32_t)(day - 1);

	uint64_t seconds = ((uint64_t)days * 86400ULL) +
		((uint64_t)hour * 3600ULL) +
		((uint64_t)minute * 60ULL) +
		(uint64_t)second;
	if (seconds > UINT32_MAX) {
		return false;
	}

	*epoch = (uint32_t)seconds;
	return true;
}

static bool gps_epoch_to_datetime(uint32_t epoch, int *year, int *month,
				  int *day, int *hour, int *minute,
				  int *second)
{
	if ((year == NULL) || (month == NULL) || (day == NULL) ||
	    (hour == NULL) || (minute == NULL) || (second == NULL)) {
		return false;
	}

	uint32_t days = epoch / 86400U;
	uint32_t rem = epoch % 86400U;

	*hour = (int)(rem / 3600U);
	rem %= 3600U;
	*minute = (int)(rem / 60U);
	*second = (int)(rem % 60U);

	int y = 1970;
	while (true) {
		uint32_t ydays = gps_is_leap_year(y) ? 366U : 365U;
		if (days < ydays) {
			break;
		}
		days -= ydays;
		y++;
	}

	int m = 1;
	while (m <= 12) {
		uint32_t mdays = gps_days_in_month(y, m);
		if (days < mdays) {
			break;
		}
		days -= mdays;
		m++;
	}

	*year = y;
	*month = m;
	*day = (int)days + 1;
	return true;
}

static uint8_t gps_weekday_sunday0(int year, int month, int day)
{
	uint32_t epoch = 0U;
	if (!gps_datetime_to_epoch_utc(year, month, day, 0, 0, 0, &epoch)) {
		return 0U;
	}

	/* 1970-01-01 was Thursday (4 with Sunday=0). */
	return (uint8_t)(((epoch / 86400U) + 4U) % 7U);
}

static uint8_t gps_last_sunday_day(int year, int month)
{
	uint8_t day = gps_days_in_month(year, month);
	while (gps_weekday_sunday0(year, month, day) != 0U) {
		day--;
	}
	return day;
}

static int32_t gps_france_utc_offset_seconds(int year, int month, int day,
					     int hour, int minute, int second)
{
	if ((month < 3) || (month > 10)) {
		return 3600;
	}

	if ((month > 3) && (month < 10)) {
		return 7200;
	}

	uint8_t switch_day = gps_last_sunday_day(year, month);
	uint32_t sec_of_day = ((uint32_t)hour * 3600U) +
		((uint32_t)minute * 60U) +
		(uint32_t)second;

	/* Europe/Paris switches at 01:00 UTC on the last Sunday of March/October. */
	if (month == 3) {
		if ((day > switch_day) ||
		    ((day == switch_day) && (sec_of_day >= 3600U))) {
			return 7200;
		}
		return 3600;
	}

	if ((day < switch_day) ||
	    ((day == switch_day) && (sec_of_day < 3600U))) {
		return 7200;
	}
	return 3600;
}

static bool gps_utc_to_france_local(int utc_year, int utc_month, int utc_day,
				    int utc_hour, int utc_minute,
				    int utc_second, int *local_year,
				    int *local_month, int *local_day,
				    int *local_hour, int *local_minute,
				    int *local_second, int32_t *offset_sec)
{
	uint32_t utc_epoch = 0U;
	if (!gps_datetime_to_epoch_utc(utc_year, utc_month, utc_day,
				       utc_hour, utc_minute, utc_second,
				       &utc_epoch)) {
		return false;
	}

	int32_t offset = gps_france_utc_offset_seconds(utc_year, utc_month,
						      utc_day, utc_hour,
						      utc_minute, utc_second);
	uint64_t local_epoch = (uint64_t)utc_epoch + (uint64_t)offset;
	if (local_epoch > UINT32_MAX) {
		return false;
	}

	if (!gps_epoch_to_datetime((uint32_t)local_epoch, local_year,
				   local_month, local_day, local_hour,
				   local_minute, local_second)) {
		return false;
	}

	if (offset_sec != NULL) {
		*offset_sec = offset;
	}
	return true;
}

static float nmea_coord_to_decimal(const char *s)
{
    /* Convert ddmm.mmmm (lat) or dddmm.mmmm (lon) to decimal degrees */
    if (!s || !*s) return 0.0f;
    double val = strtod(s, NULL);
    int deg = (int)(val / 100.0);
    double minutes = val - (deg * 100.0);
    return (float)(deg + minutes / 60.0);
}

/* Extract RMC status ('A' valid, 'V' invalid). Returns 0 if not RMC. */
static char rmc_status(const char *line)
{
    if (!line) return 0;
    if (strncmp(line, "$GNRMC", 6) != 0 && strncmp(line, "$GPRMC", 6) != 0) {
        return 0;
    }
    const char *p = line;
    int field = 0;
    char token[8];
    int tok_i = 0;
    for (; *p; ++p) {
        if (*p == ',' || *p == '*') {
            token[tok_i] = '\0';
            if (field == 2) { /* status field */
                return token[0];
            }
            tok_i = 0;
            field++;
            if (*p == '*') break;
        } else if (tok_i < (int)sizeof(token) - 1) {
            token[tok_i++] = *p;
        }
    }
    return 0;
}

static bool parse_rmc_line(const char *line, float *plat, float *plon)
{
    /* Minimal RMC parser: $GNRMC/$GPRMC, extract status + lat/lon */
    if (!line || !plat || !plon) return false;
    if (strncmp(line, "$GNRMC", 6) != 0 && strncmp(line, "$GPRMC", 6) != 0) {
        return false;
    }
    const char *p = line;
    int field = 0;
    char token[24];
    int tok_i = 0;
    bool valid = false;
    char ns='N', ew='E';
    float lat = 0.0f, lon = 0.0f;
    int utc_hour = -1, utc_minute = -1, utc_second = -1;
    int date_day = -1, date_month = -1, date_year = -1;
    for (; *p; ++p) {
        if (*p == ',' || *p == '*') {
            token[tok_i] = '\0';
            if (field == 2) { /* status */
                valid = (token[0] == 'A');
            } else if (field == 3) { /* lat */
                lat = nmea_coord_to_decimal(token);
            } else if (field == 4 && token[0]) { /* N/S */
                ns = token[0];
            } else if (field == 5) { /* lon */
                lon = nmea_coord_to_decimal(token);
            } else if (field == 6 && token[0]) { /* E/W */
                ew = token[0];
            } else if (field == 1) { /* time hhmmss.sss (UTC) */
                if (strlen(token) >= 6) {
                    utc_hour = (token[0]-'0')*10 + (token[1]-'0');
                    utc_minute = (token[2]-'0')*10 + (token[3]-'0');
                    utc_second = (token[4]-'0')*10 + (token[5]-'0');
                }
            } else if (field == 9) { /* date ddmmyy */
                if (strlen(token) == 6) {
                    date_day = (token[0]-'0')*10 + (token[1]-'0');
                    date_month = (token[2]-'0')*10 + (token[3]-'0');
                    date_year = 2000 + (token[4]-'0')*10 + (token[5]-'0');
                }
            }
            tok_i = 0;
            field++;
            if (*p == '*') break;
        } else if (tok_i < (int)sizeof(token) - 1) {
            token[tok_i++] = *p;
        }
    }
    if (!valid) return false;
    if (ns == 'S') lat = -lat;
    if (ew == 'W') lon = -lon;

    app_state.time_valid = false;
    if (gps_datetime_valid(date_year, date_month, date_day,
                           utc_hour, utc_minute, utc_second)) {
	int local_year = 0;
	int local_month = 0;
	int local_day = 0;
	int local_hour = 0;
	int local_minute = 0;
	int local_second = 0;
	int32_t france_offset_sec = 0;

	if (gps_utc_to_france_local(date_year, date_month, date_day,
				    utc_hour, utc_minute, utc_second,
				    &local_year, &local_month, &local_day,
				    &local_hour, &local_minute, &local_second,
				    &france_offset_sec)) {
		app_state.time_hour = (uint8_t)local_hour;
		app_state.time_minute = (uint8_t)local_minute;
		app_state.time_second = (uint8_t)local_second;
		app_state.time_day = (uint8_t)local_day;
		app_state.time_month = (uint8_t)local_month;
		app_state.time_year = (uint16_t)local_year;
		app_state.time_valid = true;
		LOG_INF("GPS RMC UTC %04u-%02u-%02u %02u:%02u:%02u -> France local %04u-%02u-%02u %02u:%02u:%02u (UTC%+d)",
			(unsigned int)date_year,
			(unsigned int)date_month,
			(unsigned int)date_day,
			(unsigned int)utc_hour,
			(unsigned int)utc_minute,
			(unsigned int)utc_second,
			(unsigned int)app_state.time_year,
			(unsigned int)app_state.time_month,
			(unsigned int)app_state.time_day,
			(unsigned int)app_state.time_hour,
			(unsigned int)app_state.time_minute,
			(unsigned int)app_state.time_second,
			(int)(france_offset_sec / 3600));
	} else {
		LOG_WRN("GPS RMC UTC to France local conversion failed, skip RTC sync fields");
	}
    } else {
        LOG_WRN("GPS RMC date/time invalid, skip RTC sync fields");
    }

    *plat = lat;
    *plon = lon;
    return (lat != 0.0f || lon != 0.0f);
}

static bool parse_gga_info(const char *line, int *fix_quality, int *satellites)
{
    /* Parse GGA fields:
     * field 6 = fix quality, field 7 = number of satellites.
     */
    if (!line || !fix_quality || !satellites) {
        return false;
    }
    if (strncmp(line, "$GNGGA", 6) != 0 && strncmp(line, "$GPGGA", 6) != 0) {
        return false;
    }

    const char *p = line;
    int field = 0;
    char token[16];
    int tok_i = 0;
    int fq = -1;
    int sats = -1;

    for (; *p; ++p) {
        if (*p == ',' || *p == '*') {
            token[tok_i] = '\0';
            if (field == 6 && token[0] != '\0') {
                fq = atoi(token);
            } else if (field == 7 && token[0] != '\0') {
                sats = atoi(token);
            }

            tok_i = 0;
            field++;
            if (*p == '*') {
                break;
            }
        } else if (tok_i < (int)sizeof(token) - 1) {
            token[tok_i++] = *p;
        }
    }

    if (fq < 0 || sats < 0) {
        return false;
    }

    *fix_quality = fq;
    *satellites = sats;
    return true;
}

static void process_nmea_stream(uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        char c = (char)buf[i];
        if (c == '\r') continue; /* ignore CR */
        if (c == '\n') {
            line_buf[line_len] = '\0';
            if (line_len >= 6) {
                char st = rmc_status(line_buf);
                if (st == 'V') {
                    LOG_INF("GPS valid fix: no");
                } else if (st == 'A') {
                    float lat = 0.0f, lon = 0.0f;
                    if (parse_rmc_line(line_buf, &lat, &lon)) {
                        app_state.gps_valid = true;
                        app_state.gps_latitude = lat;
                        app_state.gps_longitude = lon;
                        app_state.gps_fix_count++;
				LOG_INF("GPS fix: lat=%.5f lon=%.5f (count=%u)",
					(double)lat, (double)lon, app_state.gps_fix_count);
	                    }
	                }
                int fix_quality = 0;
                int satellites = 0;
                if (parse_gga_info(line_buf, &fix_quality, &satellites)) {
                    LOG_INF("GPS GGA: fix_quality=%d, satellites=%d",
                            fix_quality, satellites);
                }
            }
            line_len = 0;
        } else {
            if (line_len < sizeof(line_buf) - 1) {
                line_buf[line_len++] = c;
            } else {
                /* overflow: reset line */
                line_len = 0;
            }
        }
    }
}

static bool gps_try_sync_system_time(void)
{
    int ret = task_rtc_set_from_gps();
    if (ret < 0) {
        LOG_WRN("GPS->RTC sync failed: %d", ret);
        return false;
    }

    uint32_t epoch = 0U;
    ret = task_rtc_get_epoch_local(&epoch);
    if (ret < 0) {
        LOG_WRN("RTC local epoch read after GPS sync failed: %d", ret);
        return false;
    }

    ret = retained_state_update_gps_sync(epoch);
    if (ret < 0) {
        LOG_WRN("Retained GPS local epoch update failed: %d", ret);
        return false;
    }

    LOG_INF("GPS sync local epoch updated: %u", (unsigned int)epoch);
    return true;
}

bool task_gps_acquire_for_uplink(void)
{
	gps_last_rtc_sync_ok = false;

	if (APP_MICROPHONE_DEBUG_SKIP_GPS_LORA_ENABLED) {
		initialized = false;
		gps_log_microphone_debug_skip("acquire");
		return false;
	}

	uint32_t now = k_uptime_get_32();
	uint32_t old_fix_count = app_state.gps_fix_count;

    //skip GPS acquisition if we're still in backoff from previous failures
	// if ((int32_t)(now - retry_deadline_ms) < 0) {
	// 	uint32_t remain_ms = retry_deadline_ms - now;
	// 	LOG_WRN("GPS retry backoff active, skip event (remain=%u ms)",
	// 		(unsigned int)remain_ms);
	// 	return false;
	// }

	if (!initialized) {
		if (!device_is_ready(gps_i2c)) {
			LOG_ERR("GPS I2C device not ready");
			gps_mark_failure(now, "i2c not ready");
			return false;
		}

		if (gps_power_on() != 0) {
			gps_mark_failure(now, "power on failed");
			return false;
		}

		initialized = true;
		LOG_INF("GPS initialized for event trigger (I2C1 addr 0x%02x)", GPS_ADDR);
	}

	// if (gps_en.port) {
	// 	gpio_pin_set_dt(&gps_en, 1);
	// }
	// k_msleep(120);

	int ret;
	task_i2c1_lock();
	ret = i2c_read(gps_i2c, nmea_buf, sizeof(nmea_buf), GPS_ADDR);
	task_i2c1_unlock();
	if (ret) {
		LOG_WRN("GPS I2C read failed: %d", ret);
		task_i2c1_lock();
		(void)i2c_recover_bus(gps_i2c);
		task_i2c1_unlock();
		gps_power_off();
		initialized = false;
		gps_mark_failure(now, "i2c read failed");
		return false;
	}

	char prev = 0;
	size_t got = 0;
	for (size_t i = 0; i < sizeof(nmea_buf); i++) {
		char c = (char)nmea_buf[i];
		got++;
		if (c == '\n' && prev == '\n') {
			break;
		}
		prev = c;
	}
	process_nmea_stream(nmea_buf, got);

	if (app_state.gps_fix_count > old_fix_count) {
		init_fail_count = 0U;
		retry_deadline_ms = 0U;
		gps_last_rtc_sync_ok = gps_try_sync_system_time();
		if (!gps_last_rtc_sync_ok) {
			LOG_WRN("GPS fix acquired but RTC/retained sync failed");
		}
		if (app_state.time_valid) {
			(void)task_storage_persist_gps_coords(app_state.gps_latitude,
							      app_state.gps_longitude);
		} else {
			LOG_WRN("GPS fix acquired without valid time, skip gps.txt append");
		}
		gps_power_off();
		return true;
	}

	gps_mark_failure(now, "no satellite fix");
	return false;
}

void task_gps_poll(void)
{
	/* Deprecated periodic API: GPS is now event-driven. */
	(void)task_gps_acquire_for_uplink();
}

void task_gps_prepare_sleep(void)
{
	gps_power_off();
}

static bool gps_acquire_with_timeout(uint32_t timeout_ms, bool *rtc_synced,
				     bool mark_main_alive)
{
	if (rtc_synced != NULL) {
		*rtc_synced = false;
	}

	if (APP_MICROPHONE_DEBUG_SKIP_GPS_LORA_ENABLED) {
		initialized = false;
		gps_log_microphone_debug_skip("timed acquire");
		return false;
	}

	if (timeout_ms == 0U) {
		return false;
	}

	task_ina3221_block_active(true);

	uint32_t start_ms = k_uptime_get_32();
	bool got_fix = false;

	while ((k_uptime_get_32() - start_ms) < timeout_ms) {
		if (mark_main_alive) {
			task_watchdog_mark_main_alive();
		}
		task_watchdog_feed();

		if (task_gps_acquire_for_uplink()) {
			got_fix = true;
			if (rtc_synced != NULL) {
				*rtc_synced = gps_last_rtc_sync_ok;
			}
			break;
		}

		if (mark_main_alive) {
			task_watchdog_mark_main_alive();
		}
		k_msleep(1000);
	}

	task_gps_prepare_sleep();
	if (mark_main_alive) {
		task_watchdog_mark_main_alive();
	}

	if (!got_fix) {
		LOG_WRN("GPS timed acquisition timeout (%u ms)", (unsigned int)timeout_ms);
	}

	task_ina3221_block_active(false);
	return got_fix;
}

bool task_gps_acquire_with_timeout(uint32_t timeout_ms, bool *rtc_synced)
{
	return gps_acquire_with_timeout(timeout_ms, rtc_synced, false);
}

bool task_gps_acquire_with_timeout_main_alive(uint32_t timeout_ms, bool *rtc_synced)
{
	return gps_acquire_with_timeout(timeout_ms, rtc_synced, true);
}
