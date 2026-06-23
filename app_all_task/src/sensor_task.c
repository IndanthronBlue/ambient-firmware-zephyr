/*
 * Environmental Sensor Sampling
 * Periodic BME280 data acquisition using the standard Zephyr sensor API.
 *
 * Reference: zephyr/samples/sensor/bme280
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h> /* 引入电源管理 API */

LOG_MODULE_REGISTER(sensor_task, LOG_LEVEL_INF);

#include "tasks.h"

/* BME280 device from devicetree — node label "bme280" in board DTS */
static const struct device *const bme280_dev = DEVICE_DT_GET(DT_NODELABEL(bme280));

static bool sensor_initialized = false;
static bool sensor_power_cycled = false; /* 记录是否发生了掉电，需要恢复硬件 */

/**
 * Ensure the BME280 driver is ready.
 */
static int init_sensor_for_bme280(void)
{
	/* 如果设备就绪，且没有发生掉电，直接返回 */
	if (!sensor_power_cycled && device_is_ready(bme280_dev)) {
		LOG_INF("BME280 ready");
		return 0;
	}

	int ret = 0;
	LOG_WRN("BME280 init or recovery requested");

	if (sensor_power_cycled) {
		/* --- 掉电恢复逻辑 (绕开 device_init 导致的 -120 错误) --- */
		task_i2c1_lock();
#ifdef CONFIG_PM_DEVICE
		ret = pm_device_action_run(bme280_dev, PM_DEVICE_ACTION_RESUME);
#else
		struct sensor_value oversampling = { .val1 = 16, .val2 = 0 };
		ret = sensor_attr_set(bme280_dev, SENSOR_CHAN_PRESS, SENSOR_ATTR_OVERSAMPLING, &oversampling);
#endif
		task_i2c1_unlock();
		
		/* 忽略 -EALREADY，因为我们就是要唤醒它 */
		if (ret < 0 && ret != -EALREADY) {
			LOG_ERR("BME280 recovery failed: %d", ret);
			return ret;
		}
	} else {
		/* --- 原始开机初始化逻辑 --- */
		ret = device_init(bme280_dev);
		if (ret < 0) {
			LOG_ERR("device_init(bme280) failed: %d", ret);
			return ret;
		}
	}

	if (!device_is_ready(bme280_dev)) {
		LOG_ERR("BME280 still not ready after init/recovery");
		return -ENODEV;
	}

	sensor_power_cycled = false; /* 恢复成功，清除标志位 */
	LOG_INF("BME280 ready after runtime init/recovery");
	return 0;
}

void task_sensor_sample(void)
{
	/* Lazy init: 初次调用或掉电后重新唤醒 */
	if (!sensor_initialized || sensor_power_cycled) {
		if (init_sensor_for_bme280() == 0) {
			sensor_initialized = true;
			LOG_INF("TASK-4 (Sensor) initialized/recovered");
		} else {
			LOG_ERR("TASK-4 initialization/recovery failed");
			return; /* 失败则退出，等下个周期再试 */
		}
	}

	if (!device_is_ready(bme280_dev)) {
		LOG_WRN("BME280 sensor not ready");
		return;
	}

	/*
	 * Fetch all channels in one transaction.
	 * Lock I2C1 bus shared with the GPS task (gps_i2c1_mutex).
	 */
	task_i2c1_lock();
	int ret = sensor_sample_fetch(bme280_dev);
	if (ret < 0) {
		task_i2c1_unlock();
		LOG_ERR("sensor_sample_fetch failed: %d", ret);
		
		/* 核心修改：读取失败（例如外设电源被切断），标记为掉电，下个周期自动走恢复逻辑 */
		sensor_power_cycled = true; 
		return;
	}

	struct sensor_value temp, press, humidity;

	ret = sensor_channel_get(bme280_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	if (ret < 0) {
		task_i2c1_unlock();
		LOG_ERR("Failed to get temperature: %d", ret);
		return;
	}

	ret = sensor_channel_get(bme280_dev, SENSOR_CHAN_PRESS, &press);
	if (ret < 0) {
		task_i2c1_unlock();
		LOG_ERR("Failed to get pressure: %d", ret);
		return;
	}

	ret = sensor_channel_get(bme280_dev, SENSOR_CHAN_HUMIDITY, &humidity);
	if (ret < 0) {
		task_i2c1_unlock();
		LOG_ERR("Failed to get humidity: %d", ret);
		return;
	}
	task_i2c1_unlock();

	app_state.last_temp_celsius     = sensor_value_to_double(&temp);
	app_state.last_press_kpa        = sensor_value_to_double(&press);
	app_state.last_humidity_percent = sensor_value_to_double(&humidity);
	app_state.sensor_read_count++;

	LOG_INF("[SENSOR #%04u] temp=%.2fC press=%.2fkPa hum=%.1f%%",
		app_state.sensor_read_count,
		(double)app_state.last_temp_celsius,
		(double)app_state.last_press_kpa,
		(double)app_state.last_humidity_percent);
}

void task_sensor_prepare_sleep(void)
{
	/* 1. 在断开物理电源前，通过 PM API 将设备安全挂起以同步内部状态机 */
	if (device_is_ready(bme280_dev)) {
		task_i2c1_lock();
		
#ifdef CONFIG_PM_DEVICE
		int ret = pm_device_action_run(bme280_dev, PM_DEVICE_ACTION_SUSPEND);
		if (ret < 0 && ret != -EALREADY) {
			LOG_ERR("BME280 suspend failed: %d", ret);
		} else {
			LOG_INF("BME280 safely suspended via PM");
		}
#else
		LOG_INF("Preparing BME280 for sleep (No PM device enabled)");
#endif

		task_i2c1_unlock();
	}

	/* 2. 在这里执行你原本切断 v_periph 电源以及其他的睡眠准备逻辑 */
	sensor_initialized = false;
	sensor_power_cycled = true;
}

