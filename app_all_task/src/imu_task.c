#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include "tasks.h"

LOG_MODULE_REGISTER(imu_task, LOG_LEVEL_INF);

#define KXTJ3_NODE DT_NODELABEL(kxtj3)

#define KXTJ3_REG_XOUT_L   0x06U
#define KXTJ3_REG_WHO_AM_I 0x0FU
#define KXTJ3_REG_CNTL1    0x1BU
#define KXTJ3_REG_DATA_CTRL 0x21U

#define KXTJ3_WHO_AM_I_VALUE 0x35U

#define KXTJ3_CNTL1_PC1 BIT(7)
#define KXTJ3_CNTL1_RES BIT(6)

#define KXTJ3_ODR_25HZ 0x02U

#define KXTJ3_SENS_2G_14BIT_LSB_PER_G 4096.0f

#if DT_NODE_HAS_STATUS(KXTJ3_NODE, okay)
static const struct i2c_dt_spec imu_i2c = I2C_DT_SPEC_GET(KXTJ3_NODE);
#endif

static bool imu_initialized;

static int kxtj3_reg_read_with_fallback(uint8_t reg, uint8_t *value)
{
#if DT_NODE_HAS_STATUS(KXTJ3_NODE, okay)
	int ret = i2c_reg_read_byte_dt(&imu_i2c, reg, value);
	if (ret == 0) {
		return 0;
	}

	ret = i2c_write_dt(&imu_i2c, &reg, 1);
	if (ret < 0) {
		return ret;
	}

	return i2c_read_dt(&imu_i2c, value, 1);
#else
	ARG_UNUSED(reg);
	ARG_UNUSED(value);
	return -ENODEV;
#endif
}

static int kxtj3_reg_write(uint8_t reg, uint8_t value)
{
#if DT_NODE_HAS_STATUS(KXTJ3_NODE, okay)
	return i2c_reg_write_byte_dt(&imu_i2c, reg, value);
#else
	ARG_UNUSED(reg);
	ARG_UNUSED(value);
	return -ENODEV;
#endif
}

int task_imu_init(void)
{
#if !DT_NODE_HAS_STATUS(KXTJ3_NODE, okay)
	LOG_ERR("KXTJ3 node missing/disabled in devicetree");
	return -ENODEV;
#else
	if (!device_is_ready(imu_i2c.bus)) {
		LOG_ERR("KXTJ3 I2C bus not ready");
		return -ENODEV;
	}

	uint8_t who_am_i = 0U;
	int ret = -EIO;

	for (int attempt = 1; attempt <= 5; attempt++) {
		task_i2c1_lock();
		ret = kxtj3_reg_read_with_fallback(KXTJ3_REG_WHO_AM_I, &who_am_i);
		if (ret == 0) {
			task_i2c1_unlock();
			break;
		}

		(void)i2c_recover_bus(imu_i2c.bus);
		task_i2c1_unlock();
		k_msleep(20);
	}

	if (ret < 0) {
		LOG_ERR("KXTJ3 WHO_AM_I read failed after retries: %d", ret);
		return ret;
	}

	task_i2c1_lock();

	if (who_am_i != KXTJ3_WHO_AM_I_VALUE) {
		LOG_WRN("KXTJ3 WHO_AM_I mismatch: 0x%02x", who_am_i);
	}

	ret = kxtj3_reg_write(KXTJ3_REG_CNTL1, 0x00U);
	if (ret < 0) {
		task_i2c1_unlock();
		LOG_ERR("KXTJ3 standby set failed: %d", ret);
		return ret;
	}

	ret = kxtj3_reg_write(KXTJ3_REG_DATA_CTRL, KXTJ3_ODR_25HZ);
	if (ret < 0) {
		task_i2c1_unlock();
		LOG_ERR("KXTJ3 ODR config failed: %d", ret);
		return ret;
	}

	ret = kxtj3_reg_write(KXTJ3_REG_CNTL1, KXTJ3_CNTL1_PC1 | KXTJ3_CNTL1_RES);
	task_i2c1_unlock();
	if (ret < 0) {
		LOG_ERR("KXTJ3 active mode set failed: %d", ret);
		return ret;
	}

	imu_initialized = true;
	LOG_INF("IMU KXTJ3 initialized (addr=0x%02x, who_am_i=0x%02x)", imu_i2c.addr, who_am_i);
	return 0;
#endif
}

void task_imu_sample(void)
{
#if !DT_NODE_HAS_STATUS(KXTJ3_NODE, okay)
	return;
#else
	if (!imu_initialized) {
		if (task_imu_init() < 0) {
			return;
		}
	}

	uint8_t raw_buf[6] = {0};

	task_i2c1_lock();
	int ret = i2c_burst_read_dt(&imu_i2c, KXTJ3_REG_XOUT_L, raw_buf, sizeof(raw_buf));
	task_i2c1_unlock();
	if (ret < 0) {
		LOG_ERR("KXTJ3 read xyz failed: %d", ret);
		return;
	}

	int16_t raw_x = (int16_t)(((uint16_t)raw_buf[1] << 8) | raw_buf[0]);
	int16_t raw_y = (int16_t)(((uint16_t)raw_buf[3] << 8) | raw_buf[2]);
	int16_t raw_z = (int16_t)(((uint16_t)raw_buf[5] << 8) | raw_buf[4]);

	raw_x >>= 2;
	raw_y >>= 2;
	raw_z >>= 2;

	app_state.imu_accel_x_g = (float)raw_x / KXTJ3_SENS_2G_14BIT_LSB_PER_G;
	app_state.imu_accel_y_g = (float)raw_y / KXTJ3_SENS_2G_14BIT_LSB_PER_G;
	app_state.imu_accel_z_g = (float)raw_z / KXTJ3_SENS_2G_14BIT_LSB_PER_G;
	app_state.imu_read_count++;

	LOG_INF("[IMU #%04u] ax=%.4fg ay=%.4fg az=%.4fg",
		app_state.imu_read_count,
		(double)app_state.imu_accel_x_g,
		(double)app_state.imu_accel_y_g,
		(double)app_state.imu_accel_z_g);
#endif
}