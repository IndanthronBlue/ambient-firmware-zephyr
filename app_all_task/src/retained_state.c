#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/retention/retention.h>
#include <zephyr/sys/crc.h>
#include <string.h>

#include "retained_state.h"

LOG_MODULE_REGISTER(retained_state, LOG_LEVEL_INF);

#define RETAINED_STATE_MAGIC   0x52535431U /* "RST1" */
#define RETAINED_STATE_VERSION 0x0002U
#define RETAINED_SD_OP_PENDING_MAGIC 0x53444F50U /* "SDOP" */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(app_retention), okay)
static const struct device *const retained_dev = DEVICE_DT_GET(DT_NODELABEL(app_retention));
#else
static const struct device *const retained_dev;
#endif

static uint32_t retained_crc_compute(const struct retained_state_v1 *state)
{
	struct retained_state_v1 temp;

	memcpy(&temp, state, sizeof(temp));
	temp.crc32 = 0U;
	return crc32_ieee((const uint8_t *)&temp, sizeof(temp));
}

static bool retained_crc_valid(const struct retained_state_v1 *state)
{
	return state->crc32 == retained_crc_compute(state);
}

static bool retained_state_is_current(const struct retained_state_v1 *state)
{
	return (state != NULL) &&
	       (state->magic == RETAINED_STATE_MAGIC) &&
	       (state->version == RETAINED_STATE_VERSION) &&
	       retained_crc_valid(state);
}

static void retained_fill_defaults(struct retained_state_v1 *state)
{
	memset(state, 0, sizeof(*state));
	state->magic = RETAINED_STATE_MAGIC;
	state->version = RETAINED_STATE_VERSION;
	state->last_sleep_reason = RETAINED_SLEEP_REASON_NORMAL;
	state->crc32 = retained_crc_compute(state);
}

int retained_state_load(struct retained_state_v1 *out)
{
	if (out == NULL) {
		return -EINVAL;
	}

	if (retained_dev == NULL || !device_is_ready(retained_dev)) {
		return -ENODEV;
	}

	int ret = retention_read(retained_dev, 0, (uint8_t *)out, sizeof(*out));
	if (ret < 0) {
		return ret;
	}

	return retained_state_is_current(out) ? 0 : -EINVAL;
}

static int retained_state_store_internal(const struct retained_state_v1 *state, bool log_store)
{
	if (state == NULL) {
		return -EINVAL;
	}

	if (retained_dev == NULL || !device_is_ready(retained_dev)) {
		return -ENODEV;
	}

	struct retained_state_v1 temp;
	memcpy(&temp, state, sizeof(temp));
	temp.crc32 = retained_crc_compute(&temp);

	int ret = retention_write(retained_dev, 0, (const uint8_t *)&temp, sizeof(temp));
	if (ret == 0 && log_store) {
		LOG_INF("[RET] store boot=%u wake30=%u recov=%u reason=%u consec=%u gps_local_epoch=%u sd_pending=0x%08x sd_until=%u sd_strikes=%u sd_fail=%u crc=0x%08x",
			(unsigned int)temp.boot_count,
			(unsigned int)temp.wake_count_30min,
			(unsigned int)temp.low_battery_recovery_count,
			(unsigned int)temp.last_sleep_reason,
			(unsigned int)temp.consecutive_recovery_ok,
			(unsigned int)temp.last_gps_sync_epoch,
			(unsigned int)temp.sd_op_pending_magic,
			(unsigned int)temp.sd_fuse_until_epoch,
			(unsigned int)temp.sd_fuse_strikes,
			(unsigned int)temp.sd_fail_streak,
			(unsigned int)temp.crc32);
	}

	return ret;
}

int retained_state_store(const struct retained_state_v1 *state)
{
	return retained_state_store_internal(state, true);
}

int retained_state_init_or_reset(void)
{
	if (retained_dev == NULL || !device_is_ready(retained_dev)) {
		LOG_ERR("Retention device not ready");
		return -ENODEV;
	}

	struct retained_state_v1 state;
	bool reset_required = false;

	int valid_ret = retention_is_valid(retained_dev);
	if (valid_ret < 0) {
		LOG_WRN("retention_is_valid failed: %d", valid_ret);
		reset_required = true;
	} else if (valid_ret == 0) {
		reset_required = true;
	}

	if (!reset_required) {
		int ret = retained_state_load(&state);
		if (ret < 0) {
			LOG_WRN("retained_state_load failed: %d", ret);
			reset_required = true;
		} else if (!retained_state_is_current(&state)) {
			reset_required = true;
		}
	}

	if (reset_required) {
		retained_fill_defaults(&state);
		int ret = retained_state_store(&state);
		if (ret < 0) {
			LOG_ERR("retained_state_store(default) failed: %d", ret);
			return ret;
		}
		LOG_INF("Retained state reset to defaults");
	}

	if (retained_state_load(&state) < 0) {
		return -EIO;
	}

	state.boot_count++;
	int ret = retained_state_store(&state);
	if (ret < 0) {
		LOG_ERR("retained_state_store(boot_count) failed: %d", ret);
		return ret;
	}

	LOG_INF("Retained state initialized (boot_count=%u)", (unsigned int)state.boot_count);
	return 0;
}

int retained_state_mark_low_battery(void)
{
	struct retained_state_v1 state;
	int ret = retained_state_load(&state);
	if (ret < 0) {
		return ret;
	}

	state.last_sleep_reason = RETAINED_SLEEP_REASON_LOW_BAT;
	state.consecutive_recovery_ok = 0U;
	return retained_state_store(&state);
}

int retained_state_mark_normal(void)
{
	struct retained_state_v1 state;
	int ret = retained_state_load(&state);
	if (ret < 0) {
		return ret;
	}

	state.last_sleep_reason = RETAINED_SLEEP_REASON_NORMAL;
	state.consecutive_recovery_ok = 0U;
	return retained_state_store(&state);
}

int retained_state_update_gps_sync(uint32_t local_epoch)
{
	struct retained_state_v1 state;
	int ret = retained_state_load(&state);
	if (ret < 0) {
		LOG_WRN("retained_state_load before GPS sync failed: %d, using defaults", ret);
		retained_fill_defaults(&state);
	}

	state.last_gps_sync_epoch = local_epoch;
	return retained_state_store(&state);
}

int retained_state_recovery_counter_inc_or_clear(bool condition_ok)
{
	struct retained_state_v1 state;
	int ret = retained_state_load(&state);
	if (ret < 0) {
		return ret;
	}

	if (condition_ok) {
		state.consecutive_recovery_ok++;
		state.wake_count_30min++;
		state.low_battery_recovery_count++;
		state.last_sleep_reason = RETAINED_SLEEP_REASON_RECOVERY;
	} else {
		state.consecutive_recovery_ok = 0U;
		state.last_sleep_reason = RETAINED_SLEEP_REASON_LOW_BAT;  /* keep in low-bat recovery loop */
	}

	return retained_state_store(&state);
}

int retained_state_sd_op_start(uint16_t op_code)
{
	struct retained_state_v1 state;
	int ret = retained_state_load(&state);
	if (ret < 0) {
		return ret;
	}

	state.sd_op_pending_magic = RETAINED_SD_OP_PENDING_MAGIC;
	state.sd_last_op_code = op_code;
	return retained_state_store_internal(&state, false);
}

int retained_state_sd_op_end(void)
{
	struct retained_state_v1 state;
	int ret = retained_state_load(&state);
	if (ret < 0) {
		return ret;
	}

	if (state.sd_op_pending_magic == 0U && state.sd_last_op_code == 0U) {
		return 0;
	}

	state.sd_op_pending_magic = 0U;
	state.sd_last_op_code = 0U;
	return retained_state_store_internal(&state, false);
}

bool safe_elapsed(uint32_t now, uint32_t last, uint32_t *delta)
{
	if (delta == NULL) {
		return false;
	}

	if (now < last) {
		*delta = 0U;
		return false;
	}

	*delta = now - last;
	return true;
}
