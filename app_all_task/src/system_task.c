#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#if IS_ENABLED(CONFIG_WATCHDOG)
#include <zephyr/drivers/watchdog.h>
#endif

LOG_MODULE_REGISTER(system_task, LOG_LEVEL_INF);

#include "tasks.h"

#define WATCHDOG_TIMEOUT_MS 24000U
#define WATCHDOG_FEED_PERIOD_MS 10000U
/* Optional periodic reboot for long-term field stability; 0 disables. */
#define SOFT_REBOOT_PERIOD_MS 0U

#if IS_ENABLED(CONFIG_WATCHDOG) && DT_HAS_COMPAT_STATUS_OKAY(st_stm32_watchdog)
#define APP_WDT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(st_stm32_watchdog)
static const struct device *const wdt_dev = DEVICE_DT_GET(APP_WDT_NODE);
#else
static const struct device *const wdt_dev = NULL;
#endif

static int wdt_channel_id = -1;
static bool wdt_started = false;
static bool wdt_feeder_started = false;
static struct k_thread wdt_feeder_thread_data;
K_THREAD_STACK_DEFINE(wdt_feeder_stack, 512);

static void watchdog_feeder_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (1) {
		task_watchdog_feed();
		k_msleep(WATCHDOG_FEED_PERIOD_MS);
	}
}

int task_watchdog_init(void)
{
#if !IS_ENABLED(CONFIG_WATCHDOG)
	LOG_WRN("CONFIG_WATCHDOG disabled, watchdog inactive");
	return -ENOTSUP;
#else
	if (wdt_started) {
		return 0;
	}

	if (wdt_dev == NULL || !device_is_ready(wdt_dev)) {
		LOG_WRN("Watchdog device not ready, watchdog disabled");
		return -ENODEV;
	}

	struct wdt_timeout_cfg cfg = {
		.window = {
			.min = 0U,
			.max = WATCHDOG_TIMEOUT_MS,
		},
		.callback = NULL,
		.flags = WDT_FLAG_RESET_SOC,
	};

	wdt_channel_id = wdt_install_timeout(wdt_dev, &cfg);
	if (wdt_channel_id < 0) {
		LOG_ERR("wdt_install_timeout failed: %d", wdt_channel_id);
		return wdt_channel_id;
	}

	int ret = wdt_setup(wdt_dev, 0);
	if (ret < 0) {
		LOG_ERR("wdt_setup failed: %d", ret);
		return ret;
	}

	wdt_started = true;
	if (!wdt_feeder_started) {
		k_tid_t tid = k_thread_create(&wdt_feeder_thread_data,
					      wdt_feeder_stack,
					      K_THREAD_STACK_SIZEOF(wdt_feeder_stack),
					      watchdog_feeder_thread,
					      NULL, NULL, NULL,
					      K_LOWEST_APPLICATION_THREAD_PRIO,
					      0, K_NO_WAIT);
		k_thread_name_set(tid, "wdt_feeder");
		wdt_feeder_started = true;
		LOG_INF("Watchdog feeder thread started: period=%u ms",
			(unsigned int)WATCHDOG_FEED_PERIOD_MS);
	}
	LOG_INF("Watchdog started: timeout=%u ms channel=%d",
		(unsigned int)WATCHDOG_TIMEOUT_MS, wdt_channel_id);
	return 0;
#endif
}

void task_watchdog_feed(void)
{
#if !IS_ENABLED(CONFIG_WATCHDOG)
	return;
#else
	if (!wdt_started || wdt_channel_id < 0 || wdt_dev == NULL) {
		return;
	}

	int ret = wdt_feed(wdt_dev, wdt_channel_id);
	if (ret < 0) {
		LOG_WRN("wdt_feed failed: %d", ret);
	}
#endif
}

void task_system_periodic_reboot_check(void)
{
	if (SOFT_REBOOT_PERIOD_MS == 0U) {
		return;
	}

	int64_t now = k_uptime_get();
	if (now >= (int64_t)SOFT_REBOOT_PERIOD_MS) {
		LOG_WRN("Periodic soft reboot triggered at %lld ms", now);
		sys_reboot(SYS_REBOOT_COLD);
	}
}
