#ifndef APP_POWER_FSM_H
#define APP_POWER_FSM_H

#include <zephyr/kernel.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
	POWER_STATE_INITIAL = 0,
	POWER_STATE_ACTIVE,
	POWER_STATE_SUSPEND,
	POWER_STATE_DEEP_SLEEP,
} power_state_t;

typedef enum {
	POWER_WAKE_SRC_NONE = 0,
	POWER_WAKE_SRC_BUTTON = BIT(0),
	POWER_WAKE_SRC_PERIODIC = BIT(1),
	POWER_WAKE_SRC_COMM = BIT(2),
	POWER_WAKE_SRC_RTC_ALARM = BIT(3),
	POWER_WAKE_SRC_SOUND = BIT(4),
	POWER_WAKE_SRC_LOW_BAT = BIT(5),
} power_wake_src_t;

int power_fsm_init(void);
void power_fsm_tick(void);
void power_fsm_notify_active_done(void);
void power_fsm_request_wakeup(uint32_t wake_flags);
k_timeout_t power_fsm_next_wait_timeout(void);
void power_fsm_wait_for_event(k_timeout_t timeout);

#endif /* APP_POWER_FSM_H */
