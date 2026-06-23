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
#define WATCHDOG_MAIN_STALL_MS 45000U
#define WATCHDOG_COMM_STALL_MS (5U * 60U * 1000U)
#define WATCHDOG_HEALTH_LOG_PERIOD_MS 5000U
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
static uint32_t watchdog_main_alive_ms;
static uint32_t watchdog_comm_alive_ms;
static atomic_t watchdog_comm_active;
static atomic_t watchdog_sd_op_active;
static uint32_t watchdog_sd_op_start_ms;
static uint32_t watchdog_sd_op_timeout_ms;
static const char *watchdog_sd_op_name;
static uint32_t watchdog_last_health_log_ms;

static bool watchdog_elapsed_over(uint32_t now, uint32_t then, uint32_t limit_ms)
{
	return (uint32_t)(now - then) > limit_ms;
}

void task_watchdog_mark_main_alive(void)
{
	watchdog_main_alive_ms = k_uptime_get_32();
}

void task_watchdog_set_comm_active(bool active)
{
	uint32_t now = k_uptime_get_32();

	watchdog_comm_alive_ms = now;
	atomic_set(&watchdog_comm_active, active ? 1 : 0);
}

void task_watchdog_mark_comm_alive(void)
{
	watchdog_comm_alive_ms = k_uptime_get_32();
}

void task_watchdog_note_sd_op_start(const char *op_name, uint32_t timeout_ms)
{
	watchdog_sd_op_name = op_name;
	watchdog_sd_op_timeout_ms = timeout_ms;
	watchdog_sd_op_start_ms = k_uptime_get_32();
	atomic_set(&watchdog_sd_op_active, 1);
}

void task_watchdog_note_sd_op_end(void)
{
	atomic_set(&watchdog_sd_op_active, 0);
	watchdog_sd_op_name = NULL;
	watchdog_sd_op_timeout_ms = 0U;
}

static bool watchdog_health_ok(void)
{
	uint32_t now = k_uptime_get_32();
	const char *reason = NULL;
	uint32_t age_ms = 0U;
	uint32_t limit_ms = 0U;

	if (watchdog_elapsed_over(now, watchdog_main_alive_ms, WATCHDOG_MAIN_STALL_MS)) {
		reason = "main";
		age_ms = now - watchdog_main_alive_ms;
		limit_ms = WATCHDOG_MAIN_STALL_MS;
	} else if ((atomic_get(&watchdog_comm_active) != 0) &&
		   watchdog_elapsed_over(now, watchdog_comm_alive_ms, WATCHDOG_COMM_STALL_MS)) {
		reason = "comm";
		age_ms = now - watchdog_comm_alive_ms;
		limit_ms = WATCHDOG_COMM_STALL_MS;
	} else if (atomic_get(&watchdog_sd_op_active) != 0) {
		uint32_t sd_timeout = watchdog_sd_op_timeout_ms;

		if ((sd_timeout == 0U) ||
		    watchdog_elapsed_over(now, watchdog_sd_op_start_ms, sd_timeout)) {
			reason = watchdog_sd_op_name != NULL ? watchdog_sd_op_name : "sd_op";
			age_ms = now - watchdog_sd_op_start_ms;
			limit_ms = sd_timeout;
		}
	}

	if (reason == NULL) {
		return true;
	}

	if (watchdog_elapsed_over(now, watchdog_last_health_log_ms,
				  WATCHDOG_HEALTH_LOG_PERIOD_MS)) {
		watchdog_last_health_log_ms = now;
		LOG_ERR("Watchdog health blocked feed: reason=%s age=%u ms limit=%u ms",
			reason, (unsigned int)age_ms, (unsigned int)limit_ms);
	}

	return false;
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
	task_watchdog_mark_main_alive();
	task_watchdog_set_comm_active(false);
	task_watchdog_note_sd_op_end();
	watchdog_last_health_log_ms = k_uptime_get_32();

	LOG_INF("Watchdog started: timeout=%u ms channel=%d health-gated",
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

	if (!watchdog_health_ok()) {
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
