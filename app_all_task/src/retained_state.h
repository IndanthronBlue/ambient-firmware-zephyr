#ifndef APP_RETAINED_STATE_H
#define APP_RETAINED_STATE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	RETAINED_SLEEP_REASON_NORMAL = 0,
	RETAINED_SLEEP_REASON_LOW_BAT = 1,
	RETAINED_SLEEP_REASON_RECOVERY = 2,
} retained_sleep_reason_t;

struct retained_state_v1 {
	uint32_t magic;
	uint16_t version;
	uint16_t reserved;
	uint32_t crc32;
	uint32_t boot_count;
	uint32_t wake_count_30min;
	uint32_t low_battery_recovery_count;
	uint8_t last_sleep_reason;
	uint8_t consecutive_recovery_ok;
	uint16_t reserved2;
	uint32_t last_gps_sync_epoch;
};

int retained_state_init_or_reset(void);
int retained_state_load(struct retained_state_v1 *out);
int retained_state_store(const struct retained_state_v1 *state);
int retained_state_mark_low_battery(void);
int retained_state_mark_normal(void);
int retained_state_update_gps_sync(uint32_t epoch);
int retained_state_recovery_counter_inc_or_clear(bool condition_ok);
bool safe_elapsed(uint32_t now, uint32_t last, uint32_t *delta);

#endif /* APP_RETAINED_STATE_H */
