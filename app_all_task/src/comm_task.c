#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include "tasks.h"

LOG_MODULE_REGISTER(comm_task, LOG_LEVEL_INF);

#define GPS_UPLINK_WAIT_TIMEOUT_MS 20000U

static struct k_thread comm_thread_data;
K_THREAD_STACK_DEFINE(comm_thread_stack, 1536);
static K_SEM_DEFINE(comm_sem, 0, 1);
static atomic_t comm_pending_flags;
static atomic_t comm_busy;
static bool comm_started;

#define COMM_PENDING_NO_GPS    BIT(0)
#define COMM_PENDING_NEED_GPS  BIT(1)
#define COMM_PENDING_WAIT_GPS  BIT(2)

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
		power_fsm_request_wakeup(POWER_WAKE_SRC_COMM);

		bool require_gps = (pending & COMM_PENDING_NEED_GPS) != 0U;
		bool wait_gps = (pending & COMM_PENDING_WAIT_GPS) != 0U;
		if (require_gps) {
			LOG_INF("Comm worker: start GPS->LoRa event (typed=NEED_GPS)");
			(void)task_gps_acquire_for_uplink();
		} else if (wait_gps) {
			bool rtc_synced = false;
			LOG_INF("Comm worker: start timed GPS->LoRa event (typed=WAIT_GPS, timeout=%u ms)",
				(unsigned int)GPS_UPLINK_WAIT_TIMEOUT_MS);
			if (!task_gps_acquire_with_timeout(GPS_UPLINK_WAIT_TIMEOUT_MS, &rtc_synced)) {
				LOG_WRN("Comm worker: timed GPS unavailable, fallback to LoRa only");
			} else if (!rtc_synced) {
				LOG_WRN("Comm worker: GPS fix acquired but RTC sync not confirmed");
			}
		} else {
			LOG_INF("Comm worker: start LoRa event (typed=NO_GPS)");
		}

		int lora_ret = task_lorawan_send_event_uplink();
		if (lora_ret == 0) {
			LOG_INF("Comm worker: event done");
		} else {
			LOG_WRN("Comm worker: LoRa send failed (err=%d), uplink cache will be overwritten on next window", lora_ret);
		}

		atomic_set(&comm_busy, 0);
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

	atomic_or(&comm_pending_flags, (atomic_val_t)flag);
	power_fsm_request_wakeup(POWER_WAKE_SRC_COMM);
	k_sem_give(&comm_sem);
}
