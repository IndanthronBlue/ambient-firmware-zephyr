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

/* Bind I2C1 device for GPS per board DTS */
static const struct device *gps_i2c = DEVICE_DT_GET(DT_NODELABEL(i2c1));

/* GPS I2C address from board DTS cam_m8q node, fallback to 0x10 */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(cam_m8q), okay)
#define GPS_ADDR DT_REG_ADDR(DT_NODELABEL(cam_m8q))
#else
#define GPS_ADDR 0x10
#endif

/* GPS main and backup power from board DTS gpio_keys labels */
static const struct gpio_dt_spec gps_en = GPIO_DT_SPEC_GET(DT_NODELABEL(gps_main_power), gpios);
static const struct gpio_dt_spec gps_backup = GPIO_DT_SPEC_GET(DT_NODELABEL(gps_backup_power), gpios);

/* Internal state */
static bool initialized = false;
static uint32_t init_fail_count = 0;
static uint32_t retry_deadline_ms = 0;
static uint8_t nmea_buf[255];
static char line_buf[128];
static size_t line_len = 0;
K_MUTEX_DEFINE(gps_i2c1_mutex);

#define GPS_RETRY_5MIN_MS   (5U * 60U * 1000U)
#define GPS_RETRY_10MIN_MS  (10U * 60U * 1000U)
#define GPS_RETRY_30MIN_MS  (30U * 60U * 1000U)

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
    int ret = 0;
    /* 1) Skip GPS backup-power GPIO.
     * On this board/profile PB6 is used by SAI1_B FS, and driving it as GPIO
     * breaks audio capture (RX all zeros).
     */
    if (gps_backup.port) {
        LOG_WRN("GPS backup power GPIO skipped to avoid SAI1_B FS pin conflict");
    }
    /* 2) Enable main power (PB11) */
    if (gps_en.port) {
        ret = gpio_pin_configure_dt(&gps_en, GPIO_OUTPUT);
        if (ret) {
            LOG_ERR("gps_en configure failed: %d", ret);
            return ret;
        }
        /* Ensure low, then high */
        gpio_pin_set_dt(&gps_en, 0);
        k_msleep(50);
        gpio_pin_set_dt(&gps_en, 1);
        LOG_INF("GPS main power enabled (PB11)");
    }
    /* 3) Wait GPS startup */
    k_msleep(500);
    /* 4) Initialize I2C (verify bus ready) */
    if (!device_is_ready(gps_i2c)) {
        LOG_ERR("GPS I2C1 device not ready");
        return -ENODEV;
    }
    task_i2c1_lock();
    (void)i2c_recover_bus(gps_i2c);
    task_i2c1_unlock();
    return 0;
}

static void gps_power_off(void)
{
    if (gps_en.port) {
        gpio_pin_set_dt(&gps_en, 0);
    }
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
                /* Extract hour, minute, second */
                if (strlen(token) >= 6) {
                    int hh = (token[0]-'0')*10 + (token[1]-'0');
                    int mm = (token[2]-'0')*10 + (token[3]-'0');
                    int ss = (token[4]-'0')*10 + (token[5]-'0');
                    app_state.time_hour = (uint8_t)hh + 1;
                    app_state.time_minute = (uint8_t)mm;
                    app_state.time_second = (uint8_t)ss;
                }
            } else if (field == 9) { /* date ddmmyy */
                if (strlen(token) == 6) {
                    int dd = (token[0]-'0')*10 + (token[1]-'0');
                    int MM = (token[2]-'0')*10 + (token[3]-'0');
                    int yy = (token[4]-'0')*10 + (token[5]-'0');
                    app_state.time_day = (uint8_t)dd;
                    app_state.time_month = (uint8_t)MM;
                    app_state.time_year = (uint16_t)(2000 + yy);
                    LOG_INF("GPS RMC date: %04u-%02u-%02u", (unsigned int)app_state.time_year,
                            (unsigned int)app_state.time_month, (unsigned int)app_state.time_day);
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
    *plat = lat;
    *plon = lon;
    app_state.time_valid = true;
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
                        /* After a valid fix, power-save and schedule next poll in 1 hour */
                        gps_power_off();
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

static void gps_try_sync_system_time(void)
{
	task_rtc_set_from_gps();
}

bool task_gps_acquire_for_uplink(void)
{
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

	if (gps_en.port) {
		gpio_pin_set_dt(&gps_en, 1);
	}
	k_msleep(120);

	int ret;
	task_i2c1_lock();
	ret = i2c_read(gps_i2c, nmea_buf, sizeof(nmea_buf), GPS_ADDR);
	task_i2c1_unlock();
	if (ret) {
		LOG_WRN("GPS I2C read failed: %d", ret);
		task_i2c1_lock();
		(void)i2c_recover_bus(gps_i2c);
		task_i2c1_unlock();
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
		gps_try_sync_system_time();
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
