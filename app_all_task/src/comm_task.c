#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include "tasks.h"

LOG_MODULE_REGISTER(comm_task, LOG_LEVEL_INF);

static struct k_thread comm_thread_data;
K_THREAD_STACK_DEFINE(comm_thread_stack, 1536);
static K_SEM_DEFINE(comm_sem, 0, 1);
static atomic_t comm_pending;
static bool comm_started;

static void comm_worker_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (1) {
		k_sem_take(&comm_sem, K_FOREVER);
		if (!atomic_cas(&comm_pending, 1, 0)) {
			continue;
		}

		LOG_INF("Comm worker: start GPS->LoRa event");
		(void)task_gps_acquire_for_uplink();
		int lora_ret = task_lorawan_send_event_uplink();
		if (lora_ret == 0) {
			LOG_INF("Comm worker: event done");
		} else {
			LOG_WRN("Comm worker: LoRa send failed (err=%d), uplink cache will be overwritten on next window", lora_ret);
		}
	}
}

int task_comm_init(void)
{
	if (comm_started) {
		return 0;
	}

	atomic_set(&comm_pending, 0);
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
	atomic_set(&comm_pending, 1);
	k_sem_give(&comm_sem);
}
