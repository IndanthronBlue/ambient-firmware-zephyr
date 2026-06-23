#include <zephyr/kernel.h>

#include "tasks.h"

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
