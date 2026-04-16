#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "tasks.h"

LOG_MODULE_REGISTER(i2c_task, LOG_LEVEL_INF);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
static const struct device *const i2c1_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c2), okay)
static const struct device *const i2c2_dev = DEVICE_DT_GET(DT_NODELABEL(i2c2));
#endif

static struct k_mutex i2c2_lock;
static bool i2c2_lock_inited;

static void task_i2c2_lock_init_once(void)
{
	if (i2c2_lock_inited) {
		return;
	}

	k_mutex_init(&i2c2_lock);
	i2c2_lock_inited = true;
}

void task_i2c2_lock(void)
{
	task_i2c2_lock_init_once();
	k_mutex_lock(&i2c2_lock, K_FOREVER);
}

void task_i2c2_unlock(void)
{
	task_i2c2_lock_init_once();
	k_mutex_unlock(&i2c2_lock);
}

static void scan_bus(const char *name, const struct device *bus, bool use_i2c1_lock)
{
	if (!bus || !device_is_ready(bus)) {
		LOG_WRN("%s not ready, skip scan", name);
		return;
	}

	LOG_INF("%s scan start (7-bit 0x08..0x77)", name);
	if (use_i2c1_lock) {
		task_i2c1_lock();
	} else if (bus == i2c2_dev) {
		task_i2c2_lock();
	}

	int found = 0;
	for (uint8_t addr = 0x08U; addr < 0x78U; addr++) {
		int ret = i2c_write_read(bus, addr, NULL, 0, NULL, 0);
		if (ret == 0) {
			found++;
			LOG_INF("%s found device at 0x%02X", name, addr);
		}
	}

	if (use_i2c1_lock) {
		task_i2c1_unlock();
	} else if (bus == i2c2_dev) {
		task_i2c2_unlock();
	}
	LOG_INF("%s scan done, found %d device(s)", name, found);
}

void task_i2c_debug_scan_startup(void)
{
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
// 首先recover一下I2C1总线，看看能不能解决一些设备不响应的问题
    if (i2c1_dev && device_is_ready(i2c1_dev)) {
        LOG_INF("Attempting I2C1 bus recovery at startup");
        task_i2c1_lock();
        int rec = i2c_recover_bus(i2c1_dev);
        task_i2c1_unlock();
        if (rec == 0) {
            LOG_INF("I2C1 bus recovery successful");
        } else {
            LOG_WRN("I2C1 bus recovery failed: %d", rec);
        }
    } else {
        LOG_WRN("I2C1 device not ready at startup, skip recovery");
    }
	scan_bus("I2C1", i2c1_dev, true);
#else
	LOG_WRN("I2C1 node not enabled in devicetree");
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c2), okay)
// 首先recover一下I2C2总线，看看能不能解决一些设备不响应的问题
    if (i2c2_dev && device_is_ready(i2c2_dev)) {
        LOG_INF("Attempting I2C2 bus recovery at startup");
        task_i2c2_lock();
        int rec = i2c_recover_bus(i2c2_dev);
        task_i2c2_unlock();
        if (rec == 0) {
            LOG_INF("I2C2 bus recovery successful");
        } else {
            LOG_WRN("I2C2 bus recovery failed: %d", rec);
        }
    } else {
        LOG_WRN("I2C2 device not ready at startup, skip recovery");
    }
	scan_bus("I2C2", i2c2_dev, false);
#else
	LOG_WRN("I2C2 node not enabled in devicetree");
#endif
}
