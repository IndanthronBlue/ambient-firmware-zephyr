#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "tasks.h"

LOG_MODULE_REGISTER(watchdog_service, LOG_LEVEL_INF);

#define WATCHDOG_SERVICE_FEED_PERIOD_MS 2000U
#define WATCHDOG_SERVICE_STACK_SIZE     1024U
#define WATCHDOG_SERVICE_PRIORITY       K_PRIO_PREEMPT(1)

static void watchdog_service_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	LOG_INF("Watchdog service thread started (period=%u ms)",
		(unsigned int)WATCHDOG_SERVICE_FEED_PERIOD_MS);

	while (1) {
		task_watchdog_feed();
		k_msleep(WATCHDOG_SERVICE_FEED_PERIOD_MS);
	}
}

K_THREAD_DEFINE(watchdog_service_tid,
		WATCHDOG_SERVICE_STACK_SIZE,
		watchdog_service_thread,
		NULL, NULL, NULL,
		WATCHDOG_SERVICE_PRIORITY,
		0,
		0);
