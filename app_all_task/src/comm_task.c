#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/state.h>
#include <zephyr/sys/atomic.h>

#include "tasks.h"
#include "app_feature_flags.h"

LOG_MODULE_REGISTER(comm_task, LOG_LEVEL_INF);

static struct k_thread comm_thread_data;
K_THREAD_STACK_DEFINE(comm_thread_stack, 4096);
static K_SEM_DEFINE(comm_sem, 0, 1);
static atomic_t comm_pending_flags;
static atomic_t comm_busy;
static bool comm_started;

#define COMM_PENDING_NO_GPS    BIT(0)
#define COMM_PENDING_NEED_GPS  BIT(1)
#define COMM_PENDING_WAIT_GPS  BIT(2)
#define COMM_PM_POST_UPLINK_GRACE_MS 3000U

static void comm_pm_lock_suspend_to_idle(void)
{
#if IS_ENABLED(CONFIG_PM)
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	LOG_INF("Comm worker: SUSPEND_TO_IDLE locked during GPS/LoRa uplink");
#endif
}

static void comm_pm_unlock_suspend_to_idle(void)
{
#if IS_ENABLED(CONFIG_PM)
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	LOG_INF("Comm worker: SUSPEND_TO_IDLE unlocked after GPS/LoRa uplink");
#endif
}

static void comm_worker_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (1) {
		k_sem_take(&comm_sem, K_FOREVER);

		uint32_t pending = (uint32_t)atomic_set(&comm_pending_flags, 0);
		if (pending == 0U) {
			continue;
		}

		atomic_set(&comm_busy, 1);
		task_watchdog_set_comm_active(true);
		task_watchdog_mark_comm_alive();
		power_fsm_request_wakeup(POWER_WAKE_SRC_COMM);

		bool pm_locked = false;
		if (task_lorawan_drop_uplink_if_backoff()) {
			task_status_led_event(STATUS_LED_ERROR);
			goto done;
		}

		comm_pm_lock_suspend_to_idle();
		pm_locked = true;

		bool require_gps = (pending & COMM_PENDING_NEED_GPS) != 0U;
		bool wait_gps = (pending & COMM_PENDING_WAIT_GPS) != 0U;
		if (APP_MICROPHONE_DEBUG_SKIP_GPS_LORA_ENABLED && (require_gps || wait_gps)) {
			LOG_INF("Comm worker: microphone debug mode, skip GPS stage");
		} else if (require_gps) {
			LOG_INF("Comm worker: start GPS->LoRa event (typed=NEED_GPS)");
			task_ina3221_block_active(true);
			(void)task_gps_acquire_for_uplink();
			task_ina3221_block_active(false);
			task_watchdog_mark_comm_alive();
		} else if (wait_gps) {
			bool rtc_synced = false;
			LOG_INF("Comm worker: start timed GPS->LoRa event (typed=WAIT_GPS, timeout=%u ms)",
				(unsigned int)APP_GPS_UPLINK_WAIT_TIMEOUT_MS);
			task_status_led_event(STATUS_LED_GPS_WAIT);
			if (!task_gps_acquire_with_timeout(APP_GPS_UPLINK_WAIT_TIMEOUT_MS, &rtc_synced)) {
				LOG_WRN("Comm worker: timed GPS unavailable, fallback to LoRa only");
			} else if (!rtc_synced) {
				LOG_WRN("Comm worker: GPS fix acquired but RTC sync not confirmed");
			}
			task_watchdog_mark_comm_alive();
		} else {
			LOG_INF("Comm worker: start LoRa event (typed=NO_GPS)");
		}

		task_watchdog_mark_comm_alive();
		int lora_ret = task_lorawan_send_event_uplink();
		task_watchdog_mark_comm_alive();
		if (lora_ret == 0) {
			LOG_INF("Comm worker: event done");
		} else {
			LOG_WRN("Comm worker: LoRa send failed (err=%d), uplink cache will be overwritten on next window", lora_ret);
		}

done:
		if (pm_locked) {
			task_watchdog_mark_comm_alive();
			k_msleep(COMM_PM_POST_UPLINK_GRACE_MS);
			task_watchdog_mark_comm_alive();
			comm_pm_unlock_suspend_to_idle();
		}
		atomic_set(&comm_busy, 0);
		task_watchdog_set_comm_active(false);
	}
}

int task_comm_init(void)
{
	if (comm_started) {
		return 0;
	}

	atomic_set(&comm_pending_flags, 0);
	atomic_set(&comm_busy, 0);
	k_tid_t tid = k_thread_create(&comm_thread_data,
				      comm_thread_stack,
				      K_THREAD_STACK_SIZEOF(comm_thread_stack),
				      comm_worker_thread,
				      NULL, NULL, NULL,
				      K_LOWEST_APPLICATION_THREAD_PRIO,
				      0, K_NO_WAIT);
	k_thread_name_set(tid, "comm_worker");
	comm_started = true;
	LOG_INF("Comm worker thread started");
	return 0;
}

void task_comm_request_uplink(void)
{
	task_comm_request_uplink_typed(TASK_COMM_UPLINK_NO_GPS);
}

bool task_comm_is_pending_or_busy(void)
{
	return (atomic_get(&comm_pending_flags) != 0) || (atomic_get(&comm_busy) != 0);
}

void task_comm_request_uplink_typed(task_comm_uplink_type_t type)
{
	uint32_t flag;

	if (APP_MICROPHONE_DEBUG_SKIP_GPS_LORA_ENABLED && (type != TASK_COMM_UPLINK_NO_GPS)) {
		LOG_INF("Comm request coerced to NO_GPS by microphone debug mode");
		type = TASK_COMM_UPLINK_NO_GPS;
	}

	// DEBUG
	switch (type) {
	case TASK_COMM_UPLINK_NEED_GPS:
		flag = COMM_PENDING_NEED_GPS;
		break;
	case TASK_COMM_UPLINK_WAIT_GPS:
		flag = COMM_PENDING_WAIT_GPS;
		break;
	case TASK_COMM_UPLINK_NO_GPS:
	default:
		flag = COMM_PENDING_NO_GPS;
		break;
	}

	// flag = COMM_PENDING_NO_GPS;

	atomic_or(&comm_pending_flags, (atomic_val_t)flag);
	power_fsm_request_wakeup(POWER_WAKE_SRC_COMM);
	k_sem_give(&comm_sem);
}
