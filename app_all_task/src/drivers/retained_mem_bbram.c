/*
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bird_retained_bbram

#include <string.h>
#include <sys/types.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bbram.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(retained_mem_bbram, CONFIG_RETAINED_MEM_LOG_LEVEL);

#define RETAINED_MEM_BBRAM_CLEAR_CHUNK_SIZE 16U

struct retained_mem_bbram_config {
	const struct device *bbram;
	size_t offset;
	size_t size;
};

struct retained_mem_bbram_data {
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct k_mutex lock;
#endif
};

static inline void retained_mem_bbram_lock_take(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct retained_mem_bbram_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
#else
	ARG_UNUSED(dev);
#endif
}

static inline void retained_mem_bbram_lock_release(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct retained_mem_bbram_data *data = dev->data;

	k_mutex_unlock(&data->lock);
#else
	ARG_UNUSED(dev);
#endif
}

static int retained_mem_bbram_init(const struct device *dev)
{
	const struct retained_mem_bbram_config *config = dev->config;
	size_t bbram_size = 0U;
	int ret;

	if (!device_is_ready(config->bbram)) {
		LOG_ERR("BBRAM device %s is not ready", config->bbram->name);
		return -ENODEV;
	}

	ret = bbram_get_size(config->bbram, &bbram_size);
	if (ret < 0) {
		LOG_ERR("BBRAM size query failed: %d", ret);
		return ret;
	}

	if ((config->offset > bbram_size) || (config->size > (bbram_size - config->offset))) {
		LOG_ERR("BBRAM retained window out of range: off=0x%x size=0x%x bbram=0x%x",
			(unsigned int)config->offset,
			(unsigned int)config->size,
			(unsigned int)bbram_size);
		return -EINVAL;
	}

#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct retained_mem_bbram_data *data = dev->data;

	k_mutex_init(&data->lock);
#endif

	return 0;
}

static ssize_t retained_mem_bbram_size(const struct device *dev)
{
	const struct retained_mem_bbram_config *config = dev->config;

	return (ssize_t)config->size;
}

static int retained_mem_bbram_read(const struct device *dev, off_t offset, uint8_t *buffer,
				   size_t size)
{
	const struct retained_mem_bbram_config *config = dev->config;
	int ret;

	retained_mem_bbram_lock_take(dev);
	ret = bbram_read(config->bbram, config->offset + (size_t)offset, size, buffer);
	retained_mem_bbram_lock_release(dev);

	return ret;
}

static int retained_mem_bbram_write(const struct device *dev, off_t offset,
				    const uint8_t *buffer, size_t size)
{
	const struct retained_mem_bbram_config *config = dev->config;
	int ret;

	retained_mem_bbram_lock_take(dev);
	ret = bbram_write(config->bbram, config->offset + (size_t)offset, size, buffer);
	retained_mem_bbram_lock_release(dev);

	return ret;
}

static int retained_mem_bbram_clear(const struct device *dev)
{
	const struct retained_mem_bbram_config *config = dev->config;
	uint8_t zero[RETAINED_MEM_BBRAM_CLEAR_CHUNK_SIZE];
	size_t pos = 0U;
	int ret = 0;

	memset(zero, 0, sizeof(zero));

	retained_mem_bbram_lock_take(dev);

	while (pos < config->size) {
		size_t chunk = MIN(sizeof(zero), config->size - pos);

		ret = bbram_write(config->bbram, config->offset + pos, chunk, zero);
		if (ret < 0) {
			break;
		}
		pos += chunk;
	}

	retained_mem_bbram_lock_release(dev);

	return ret;
}

static DEVICE_API(retained_mem, retained_mem_bbram_api) = {
	.size = retained_mem_bbram_size,
	.read = retained_mem_bbram_read,
	.write = retained_mem_bbram_write,
	.clear = retained_mem_bbram_clear,
};

#define RETAINED_MEM_BBRAM_DEVICE(inst)							\
	static struct retained_mem_bbram_data retained_mem_bbram_data_##inst;		\
	static const struct retained_mem_bbram_config retained_mem_bbram_config_##inst = {	\
		.bbram = DEVICE_DT_GET(DT_INST_PHANDLE(inst, bbram)),			\
		.offset = DT_INST_PROP(inst, offset),					\
		.size = DT_INST_PROP(inst, size),					\
	};										\
	DEVICE_DT_INST_DEFINE(inst,							\
			      retained_mem_bbram_init,					\
			      NULL,							\
			      &retained_mem_bbram_data_##inst,				\
			      &retained_mem_bbram_config_##inst,				\
			      POST_KERNEL,						\
			      CONFIG_RETAINED_MEM_INIT_PRIORITY,				\
			      &retained_mem_bbram_api);

DT_INST_FOREACH_STATUS_OKAY(RETAINED_MEM_BBRAM_DEVICE)
