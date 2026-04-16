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

/* Initialization flag */
static bool ina_initialized = false;
static bool ina_thread_started = false;

static atomic_t ina_active_guard;

static struct k_thread ina_thread_data;
K_THREAD_STACK_DEFINE(ina_thread_stack, 1024);

#define INA_MONITOR_PERIOD_MS 30000
#define INA_STARTUP_DELAY_MS 30000
#define INA_THREAD_PRIO K_IDLE_PRIO

/* Attribute alias to avoid magic number usage (driver-private) */
#ifndef SENSOR_ATTR_INA3221_SELECTED_CHANNEL
#define SENSOR_ATTR_INA3221_SELECTED_CHANNEL ((enum sensor_attribute)(SENSOR_ATTR_PRIV_START + 1))
#endif

static int init_ina3221(void)
{
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

void task_ina3221_block_active(bool block)
{
    atomic_set(&ina_active_guard, block ? 1 : 0);
}

void task_ina3221_monitor(void)
{
	if (task_ina3221_read_now() < 0) {
		return;
	}

	int32_t batt_mv = (int32_t)(app_state.ina_ch2_voltage_v * 1000.0f);
	float batt_ma = app_state.ina_ch2_current_ma;
	if ((batt_mv > 0) && (batt_mv < APP_LOW_BATTERY_MV) && (batt_ma < 0.0f)) {
		LOG_WRN("[INA] low battery detected: v=%d mV i=%.1f mA -> request deep sleep",
			(int)batt_mv,
			(double)batt_ma);
		power_fsm_request_wakeup(POWER_WAKE_SRC_LOW_BAT);
	}
}

int task_ina3221_read_now(void)
{
	/* First execution: initialize device */
	if (!ina_initialized) {
		if (init_ina3221() == 0) {
			ina_initialized = true;
			LOG_INF("TASK-9 (INA3221) initialized");
		} else {
			LOG_ERR("TASK-9 initialization failed");
			return -ENODEV;
		}
	}

	if (!device_is_ready(ina3221_dev)) {
		LOG_WRN("INA3221 device not ready");
		return -ENODEV;
	}

	float v;
	int ret;
	int read_ret;
	int current_ret;

	task_i2c2_lock();

	/* Channel 1 */
	ret = select_channel(1);
	read_ret = (ret == 0) ? read_bus_voltage(&v) : ret;
	if (read_ret == 0) {
		app_state.ina_ch1_voltage_v = v;
	} else {
		LOG_WRN("INA3221 ch1 select/read failed: %d", read_ret);
	}
	current_ret = (ret == 0) ? read_current_ma(&v) : ret;
	if (current_ret == 0) {
		app_state.ina_ch1_current_ma = v;
	} else {
		LOG_WRN("INA3221 ch1 current read failed: %d", current_ret);
	}

	/* Channel 2 */
	ret = select_channel(2);
	read_ret = (ret == 0) ? read_bus_voltage(&v) : ret;
	if (read_ret == 0) {
		app_state.ina_ch2_voltage_v = v;
	} else {
		LOG_WRN("INA3221 ch2 select/read failed: %d", read_ret);
	}
	current_ret = (ret == 0) ? read_current_ma(&v) : ret;
	if (current_ret == 0) {
		app_state.ina_ch2_current_ma = v;
	} else {
		LOG_WRN("INA3221 ch2 current read failed: %d", current_ret);
	}

	task_i2c2_unlock();

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

	return 0;
}

static void ina_worker_thread(void *a, void *b, void *c)
{
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    /* Avoid racing boot-time peripheral bring-up / early suspend transitions.
     * The INA worker can power-cycle v_periph when the system is not ACTIVE,
     * which is disruptive if started immediately from main().
     */
    k_msleep(INA_STARTUP_DELAY_MS);

    while (1) {
        bool powered_for_ina = false;

        if (atomic_get(&ina_active_guard) != 0) {
            k_msleep(INA_MONITOR_PERIOD_MS);
            continue;
        }

        int power_ret = power_ctrl_vperiph_on_for_i2c2();
        if (power_ret < 0) {
            LOG_WRN("INA3221 power-on before background read failed: %d", power_ret);
            k_msleep(INA_MONITOR_PERIOD_MS);
            continue;
        }
        powered_for_ina = true;

        if (atomic_get(&ina_active_guard) == 0) {
            task_ina3221_monitor();
        }

        if (powered_for_ina && atomic_get(&ina_active_guard) == 0) {
            (void)power_ctrl_vperiph_off();
        }

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
