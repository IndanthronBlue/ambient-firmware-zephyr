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

LOG_MODULE_REGISTER(sensor_task, LOG_LEVEL_INF);

#include "tasks.h"

/* BME280 device from devicetree — node label "bme280" in board DTS */
static const struct device *const bme280_dev = DEVICE_DT_GET(DT_NODELABEL(bme280));

static bool sensor_initialized;

/**
 * Ensure the BME280 driver is ready.
 */
static int init_sensor_for_bme280(void)
{
	if (device_is_ready(bme280_dev)) {
		LOG_INF("BME280 ready");
		return 0;
	}

	/* Boot-time init failed — retry now that hardware is stable */
	LOG_WRN("BME280 not ready at boot; retrying driver init");
	int ret = device_init(bme280_dev);
	if (ret < 0) {
		LOG_ERR("device_init(bme280) failed: %d", ret);
		return ret;
	}

	if (!device_is_ready(bme280_dev)) {
		LOG_ERR("BME280 still not ready after device_init()");
		return -ENODEV;
	}

	LOG_INF("BME280 ready after runtime re-init");
	return 0;
}

void task_sensor_sample(void)
{
	/* Lazy init: run once on first call */
	if (!sensor_initialized) {
		if (init_sensor_for_bme280() == 0) {
			sensor_initialized = true;
			LOG_INF("TASK-4 (Sensor) initialized");
		} else {
			LOG_ERR("TASK-4 initialization failed");
		}
		return;
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

