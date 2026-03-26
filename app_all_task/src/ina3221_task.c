/*
 * INA3221 Power Monitor
 * Periodic triple-channel voltage monitoring with self-initialization
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ina3221_task, LOG_LEVEL_INF);

#include "tasks.h"

/* INA3221 device from Devicetree */
static const struct device *ina3221_dev = DEVICE_DT_GET(DT_NODELABEL(ina3221));
static const struct device *i2c2_dev = DEVICE_DT_GET(DT_NODELABEL(i2c2));

/* Initialization flag */
static bool ina_initialized = false;
static bool i2c_scanned_once = false;
static bool ina_thread_started = false;

static struct k_thread ina_thread_data;
K_THREAD_STACK_DEFINE(ina_thread_stack, 1024);

#define INA_MONITOR_PERIOD_MS 10000
#define INA_LOW_BATTERY_THRESHOLD_MV 3300
#define INA_LOW_BATTERY_WAKEUP_DELAY_SEC 600
#define INA_THREAD_PRIO K_IDLE_PRIO

/* Attribute alias to avoid magic number usage (driver-private) */
#ifndef SENSOR_ATTR_INA3221_SELECTED_CHANNEL
#define SENSOR_ATTR_INA3221_SELECTED_CHANNEL ((enum sensor_attribute)(SENSOR_ATTR_PRIV_START + 1))
#endif

static int init_ina3221(void)
{
    /* One-shot I2C2 bus scan to help diagnose address wiring */
    if (!i2c_scanned_once) {
        if (!device_is_ready(i2c2_dev)) {
            LOG_ERR("I2C2 bus not ready; skip scan");
        } else {
            LOG_INF("I2C2 scan start (7-bit addresses):");
            int found = 0;
            for (uint8_t addr = 0x01; addr <= 0x80; addr++) {
                int ret = i2c_write_read(i2c2_dev, addr, NULL, 0, NULL, 0);
                if (ret == 0) {
                    found++;
                    LOG_INF(" - found device at 0x%02X (8-bit 0x%02X)", addr, (addr << 1));
                }
            }
            LOG_INF("I2C2 scan done, found %d device(s)", found);
        }
        i2c_scanned_once = true;
    }

    if (!device_is_ready(ina3221_dev)) {
        LOG_WRN("INA3221 not ready at boot; initializing driver now");
        int ret = device_init(ina3221_dev);
        if (ret < 0) {
            LOG_ERR("device_init(ina3221) failed: %d", ret);
            return ret;
        }

        if (!device_is_ready(ina3221_dev)) {
            LOG_ERR("INA3221 still not ready after device_init()");
            return -ENODEV;
        }
    }

    LOG_INF("INA3221 device ready");
    return 0;
}

static int select_channel(int ch)
{
    struct sensor_value sel = {0};
    sel.val1 = ch;
    return sensor_attr_set(ina3221_dev, SENSOR_CHAN_ALL, SENSOR_ATTR_INA3221_SELECTED_CHANNEL, &sel);
}

static int read_bus_voltage(float *out_v)
{
    int ret;
    struct sensor_value v;

    ret = sensor_sample_fetch_chan(ina3221_dev, SENSOR_CHAN_VOLTAGE);
    if (ret < 0) {
        LOG_ERR("INA3221 fetch failed: %d", ret);
        return ret;
    }

    ret = sensor_channel_get(ina3221_dev, SENSOR_CHAN_VOLTAGE, &v);
    if (ret < 0) {
        LOG_ERR("INA3221 read failed: %d", ret);
        return ret;
    }

    *out_v = sensor_value_to_double(&v);
    return 0;
}

static int read_current_ma(float *out_ma)
{
    int ret;
    struct sensor_value i;

    ret = sensor_sample_fetch_chan(ina3221_dev, SENSOR_CHAN_CURRENT);
    if (ret < 0) {
        LOG_ERR("INA3221 current fetch failed: %d", ret);
        return ret;
    }

    ret = sensor_channel_get(ina3221_dev, SENSOR_CHAN_CURRENT, &i);
    if (ret < 0) {
        LOG_ERR("INA3221 current read failed: %d", ret);
        return ret;
    }

    *out_ma = (float)(sensor_value_to_double(&i) * 1000.0);
    return 0;
}

void task_ina3221_monitor(void)
{
    /* First execution: initialize device */
    if (!ina_initialized) {
        if (init_ina3221() == 0) {
            ina_initialized = true;
            LOG_INF("TASK-9 (INA3221) initialized");
            return;
        } else {
            LOG_ERR("TASK-9 initialization failed");
            return;
        }
    }

    if (!device_is_ready(ina3221_dev)) {
        LOG_WRN("INA3221 device not ready");
        return;
    }

    float v;
    int ret;

    /* Channel 1 */
    ret = select_channel(1);
    if (ret == 0 && read_bus_voltage(&v) == 0) {
        app_state.ina_ch1_voltage_v = v;
    } else {
        LOG_WRN("INA3221 ch1 select/read failed: %d", ret);
    }
    if (ret == 0 && read_current_ma(&v) == 0) {
        app_state.ina_ch1_current_ma = v;
    } else {
        LOG_WRN("INA3221 ch1 current read failed: %d", ret);
    }

    /* Channel 2 */
    ret = select_channel(2);
    if (ret == 0 && read_bus_voltage(&v) == 0) {
        app_state.ina_ch2_voltage_v = v;
    } else {
        LOG_WRN("INA3221 ch2 select/read failed: %d", ret);
    }
    if (ret == 0 && read_current_ma(&v) == 0) {
        app_state.ina_ch2_current_ma = v;
    } else {
        LOG_WRN("INA3221 ch2 current read failed: %d", ret);
    }

    /* Channel 3 is disabled in DT (enable-channel = <1 1 0>). */
    app_state.ina_ch3_voltage_v = 0.0f;
    app_state.ina_ch3_current_ma = 0.0f;

    app_state.ina_read_count++;

    /* Single-line status to reduce log bandwidth and interleaving. */
    LOG_INF("[INA #%04u] V=(%.3f,%.3f,%.3f)V I=(%.1f,%.1f,%.1f)mA",
            app_state.ina_read_count,
            (double)app_state.ina_ch1_voltage_v,
            (double)app_state.ina_ch2_voltage_v,
            (double)app_state.ina_ch3_voltage_v,
            (double)app_state.ina_ch1_current_ma,
            (double)app_state.ina_ch2_current_ma,
            (double)app_state.ina_ch3_current_ma);
}

static void ina_low_battery_guard(void)
{
    int32_t batt_mv = (int32_t)(app_state.ina_ch2_voltage_v * 1000.0f);
    if (batt_mv <= 0 || batt_mv >= INA_LOW_BATTERY_THRESHOLD_MV) {
        return;
    }

    LOG_WRN("Low battery detected: %d mV (threshold %d mV)",
        (int)batt_mv, INA_LOW_BATTERY_THRESHOLD_MV);
    LOG_WRN("Entering low-battery shutdown for %u seconds wakeup window",
        (unsigned int)INA_LOW_BATTERY_WAKEUP_DELAY_SEC);
    
    //TODO: add proper poweroff and RTC wakeup; for now just sleep to simulate
    k_msleep(INA_LOW_BATTERY_WAKEUP_DELAY_SEC * 1000);
}

static void ina_worker_thread(void *a, void *b, void *c)
{
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    while (1) {
        task_ina3221_monitor();
        ina_low_battery_guard();
        k_msleep(INA_MONITOR_PERIOD_MS);
    }
}

int task_ina3221_init(void)
{
    if (ina_thread_started) {
        return 0;
    }

    k_tid_t tid = k_thread_create(&ina_thread_data,
                      ina_thread_stack,
                      K_THREAD_STACK_SIZEOF(ina_thread_stack),
                      ina_worker_thread,
                      NULL, NULL, NULL,
                      INA_THREAD_PRIO,
                      0, K_NO_WAIT);
    k_thread_name_set(tid, "ina3221_worker");
    ina_thread_started = true;
    LOG_INF("INA3221 worker thread started (period=%d ms)", INA_MONITOR_PERIOD_MS);
    return 0;
}
