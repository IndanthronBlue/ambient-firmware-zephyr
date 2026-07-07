#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/state.h>
#include <zephyr/sys/atomic.h>

#include "tasks.h"
#include "app_feature_flags.h"
#include "retained_state.h"

LOG_MODULE_REGISTER(power_fsm, LOG_LEVEL_INF);

#define ACTIVE_BURST_MS                  (10U * 1000U)
#define ACTIVE_CAPTURE_GAP_MS            100U
#define ACTIVE_UPLINK_WAIT_MS            (10U * 1000U)
#define ACTIVE_UPLINK_POLL_MS            500U
#define GPS_RESYNC_PERIOD_SEC            (3U * 60U * 60U)
#define SUSPEND_TO_ACTIVE_PERIOD_MS      ((uint32_t)CONFIG_APP_SUSPEND_TO_ACTIVE_PERIOD_MS)
#ifndef DEEP_SLEEP_WAKE_PERIOD_MS
#define DEEP_SLEEP_WAKE_PERIOD_MS   ((uint32_t)CONFIG_APP_DEEP_SLEEP_WAKE_SEC * 1000U)
#endif
#define CODEC_ZERO_FAULT_SHUTDOWN_STREAK 5U
#define CODEC_ZERO_FAULT_SHUTDOWN_WAKE_MS (20U * 1000U)
#define WDT_SAFE_FEED_SLICE_MS           15000U

struct power_fsm_ctx {
	power_state_t state;
	uint32_t entered_at_ms;
	uint32_t suspend_to_active_deadline_ms;
	uint32_t lorawan_uplink_deadline_ms;
	uint32_t deep_sleep_wakeup_period_ms;
	uint32_t deep_sleep_wakeup_deadline_ms;
	uint32_t active_next_period_deadline_ms;
	uint32_t active_exit_deadline_ms;
	bool active_uplink_pending;
	bool active_uplink_in_progress;
	task_comm_uplink_type_t active_uplink_type;
	const char *active_uplink_reason;
	bool deep_sleep_prepared;
	uint8_t codec_zero_fault_streak;
};

static struct power_fsm_ctx fsm;
static atomic_t wake_flags;
K_SEM_DEFINE(power_fsm_wake_sem, 0, K_SEM_MAX_LIMIT);

static atomic_t pm_enter_count;
static atomic_t pm_exit_count;
static enum pm_state pm_last_enter_state;
static enum pm_state pm_last_exit_state;
static uint8_t pm_last_enter_substate;
static uint8_t pm_last_exit_substate;
static uint32_t pm_enter_seen;
static uint32_t pm_exit_seen;

static void power_fsm_log_pm_activity_if_any(void)
{
	uint32_t enter_now = (uint32_t)atomic_get(&pm_enter_count);
	uint32_t exit_now = (uint32_t)atomic_get(&pm_exit_count);

	if ((enter_now == pm_enter_seen) && (exit_now == pm_exit_seen)) {
		return;
	}

	pm_enter_seen = enter_now;
	pm_exit_seen = exit_now;

	LOG_INF("[PWR] PM activity enter=%u exit=%u last_enter=%s/%u last_exit=%s/%u",
		(unsigned int)enter_now,
		(unsigned int)exit_now,
		pm_state_to_str(pm_last_enter_state),
		(unsigned int)pm_last_enter_substate,
		pm_state_to_str(pm_last_exit_state),
		(unsigned int)pm_last_exit_substate);
}

static void pm_substate_entry_cb(enum pm_state state, uint8_t substate_id)
{
	pm_last_enter_state = state;
	pm_last_enter_substate = substate_id;
	atomic_inc(&pm_enter_count);
}

static void pm_substate_exit_cb(enum pm_state state, uint8_t substate_id)
{
	pm_last_exit_state = state;
	pm_last_exit_substate = substate_id;
	atomic_inc(&pm_exit_count);
}

static struct pm_notifier app_pm_notifier = {
	.substate_entry = pm_substate_entry_cb,
	.substate_exit = pm_substate_exit_cb,
	.report_substate = true,
};

static bool pm_notifier_registered;

static inline bool time_reached(uint32_t now, uint32_t deadline)
{
	return (int32_t)(now - deadline) >= 0;
}

static inline uint32_t ms_until(uint32_t now, uint32_t deadline)
{
	if (time_reached(now, deadline)) {
		return 0U;
	}
	return (uint32_t)(deadline - now);
}

static void advance_deadline(uint32_t *deadline, uint32_t period_ms, uint32_t now)
{
	do {
		*deadline += period_ms;
	} while (time_reached(now, *deadline));
}

static const char *state_to_str(power_state_t s)
{
	switch (s) {
	case POWER_STATE_INITIAL:
		return "INITIAL";
	case POWER_STATE_ACTIVE:
		return "ACTIVE";
	case POWER_STATE_SUSPEND:
		return "SUSPEND";
	case POWER_STATE_DEEP_SLEEP:
		return "DEEP_SLEEP";
	default:
		return "UNKNOWN";
	}
}

static uint32_t power_fsm_enabled_wakeup_mask(void)
{
	uint32_t mask = 0U;

	if (APP_WAKEUP_SRC_BUTTON_ENABLED) {
		mask |= POWER_WAKE_SRC_BUTTON;
	}
	if (APP_WAKEUP_SRC_PERIODIC_ENABLED) {
		mask |= POWER_WAKE_SRC_PERIODIC;
	}
	if (APP_WAKEUP_SRC_COMM_ENABLED) {
		mask |= POWER_WAKE_SRC_COMM;
	}
	if (APP_WAKEUP_SRC_RTC_ALARM_ENABLED) {
		mask |= POWER_WAKE_SRC_RTC_ALARM;
	}
	if (APP_WAKEUP_SRC_SOUND_ENABLED) {
		mask |= POWER_WAKE_SRC_SOUND;
	}
	if (APP_WAKEUP_SRC_LOW_BAT_ENABLED) {
		mask |= POWER_WAKE_SRC_LOW_BAT;
	}

	return mask;
}

static void power_fsm_log_wakeup_mask(uint32_t mask)
{
	LOG_INF("[PWR] wake_src mask=0x%08x [button=%d periodic=%d comm=%d rtc=%d sound=%d low_bat=%d]",
		(unsigned int)mask,
		APP_WAKEUP_SRC_BUTTON_ENABLED,
		APP_WAKEUP_SRC_PERIODIC_ENABLED,
		APP_WAKEUP_SRC_COMM_ENABLED,
		APP_WAKEUP_SRC_RTC_ALARM_ENABLED,
		APP_WAKEUP_SRC_SOUND_ENABLED,
		APP_WAKEUP_SRC_LOW_BAT_ENABLED);
}

static void power_fsm_enter(power_state_t next, const char *reason)
{
	uint32_t now = k_uptime_get_32();
	power_state_t prev = fsm.state;
	bool release_ina_after_suspend_prepare = false;

	fsm.state = next;
	fsm.entered_at_ms = now;

	switch (next) {
	case POWER_STATE_ACTIVE:
		fsm.active_next_period_deadline_ms = now;
		fsm.active_exit_deadline_ms = now + ACTIVE_BURST_MS;
		fsm.active_uplink_pending = false;
		fsm.active_uplink_in_progress = false;
		fsm.active_uplink_type = TASK_COMM_UPLINK_NO_GPS;
		fsm.active_uplink_reason = "active_entry";
		/* 恢复电源 */
		int pwr_ret = power_ctrl_vperiph_on();
		if (pwr_ret < 0) {
			LOG_WRN("[PWR] v_periph enable failed on ACTIVE entry: %d", pwr_ret);
			task_sd_mount_async_cancel();
			power_fsm_enter(POWER_STATE_SUSPEND, "active_entry_vperiph_failed");
			return;
		}
		task_status_led_event(STATUS_LED_ACTIVE_ENTER);
		/* 配置Codec并开启录音流水线 */
		if (!task_microphone_is_initialized()) {
			int mic_ret = task_microphone_init();
			if (mic_ret < 0) {
				LOG_WRN("[PWR] microphone init failed on ACTIVE entry: %d", mic_ret);
				task_sd_mount_async_cancel();
				power_fsm_enter(POWER_STATE_SUSPEND, "active_entry_mic_init_failed");
				return;
			}
		}
		/* 后台挂载SD卡 (非阻塞) */
		task_sd_mount_async();
		break;
	case POWER_STATE_SUSPEND:
		task_ina3221_block_active(true);
		if (!task_ina3221_wait_idle(1000U)) {
			LOG_WRN("[PWR] INA read still busy while entering SUSPEND; continue suspend prep");
		}
		/* 卸载SD卡、关闭高耗电外设、进入常规低功耗准备（保留sound wakeup） */
		(void)power_ctrl_prepare_suspend();
		fsm.suspend_to_active_deadline_ms =
			k_uptime_get_32() + SUSPEND_TO_ACTIVE_PERIOD_MS;
		release_ina_after_suspend_prepare = true;
		break;
	case POWER_STATE_DEEP_SLEEP:
		task_ina3221_block_active(false);
		if (fsm.deep_sleep_wakeup_period_ms == 0U) {
			fsm.deep_sleep_wakeup_period_ms = DEEP_SLEEP_WAKE_PERIOD_MS;
		}
		fsm.deep_sleep_wakeup_deadline_ms = now + fsm.deep_sleep_wakeup_period_ms;
		fsm.deep_sleep_prepared = false;
		break;
	case POWER_STATE_INITIAL:
	default:
		break;
	}

	LOG_INF("[PWR] %s->%s reason=%s", state_to_str(prev), state_to_str(next), reason);

	if (release_ina_after_suspend_prepare) {
		task_ina3221_block_active(false);
	}
}

static void power_fsm_enter_deep_sleep_with_period(const char *reason, uint32_t wake_period_ms)
{
	fsm.deep_sleep_wakeup_period_ms = wake_period_ms;
	power_fsm_enter(POWER_STATE_DEEP_SLEEP, reason);
}

static void power_fsm_enter_deep_sleep_default(const char *reason)
{
	power_fsm_enter_deep_sleep_with_period(reason, DEEP_SLEEP_WAKE_PERIOD_MS);
}

static void power_fsm_reset_codec_zero_fault_streak(const char *reason)
{
	if (fsm.codec_zero_fault_streak == 0U) {
		return;
	}

	LOG_INF("[PWR] codec zero fault streak reset from %u reason=%s",
		(unsigned int)fsm.codec_zero_fault_streak,
		reason != NULL ? reason : "unknown");
	fsm.codec_zero_fault_streak = 0U;
}


static bool power_fsm_should_resync_gps_time(const char **reason)
{
	if (reason != NULL) {
		*reason = "not_checked";
	}

	if (APP_MICROPHONE_DEBUG_SKIP_GPS_LORA_ENABLED) {
		if (reason != NULL) {
			*reason = "mic_debug_skip_gps_lora";
		}
		return false;
	}

	uint32_t now_epoch = 0U;
	struct retained_state_v1 retained;
	uint32_t delta = 0U;

	if (task_rtc_get_epoch_local(&now_epoch) < 0) {
		LOG_WRN("[PWR] GPS resync check failed to get RTC local epoch");
		if (reason != NULL) {
			*reason = "rtc_epoch_invalid";
		}
		return true;
	}

	if (retained_state_load(&retained) < 0) {
		LOG_WRN("[PWR] GPS resync check failed to load retained state");
		if (reason != NULL) {
			*reason = "retained_load_failed";
		}
		return true;
	}

	if (retained.last_gps_sync_epoch == 0U) {
		LOG_INF("[PWR] GPS resync needed: no retained GPS sync epoch");
		if (reason != NULL) {
			*reason = "no_retained_gps_sync_epoch";
		}
		return true;
	}

	if (!safe_elapsed(now_epoch, retained.last_gps_sync_epoch, &delta)) {
		LOG_WRN("[PWR] GPS resync check failed to compute elapsed time");
		if (reason != NULL) {
			*reason = "rtc_before_last_gps_sync";
		}
		return true;
	}

	if (delta < GPS_RESYNC_PERIOD_SEC) {
		LOG_INF("[PWR] GPS resync not needed, elapsed since last sync=%u sec",
			(unsigned int)delta);
		if (reason != NULL) {
			*reason = "last_gps_sync_fresh";
		}
		return false;
	}

	LOG_INF("[PWR] GPS resync needed, elapsed since last sync=%u sec",
		(unsigned int)delta);
	if (reason != NULL) {
		*reason = "last_gps_sync_expired";
	}
	return true;
}

int power_fsm_init(void)
{
	uint32_t now = k_uptime_get_32();
	uint32_t wake_mask = power_fsm_enabled_wakeup_mask();

	fsm.state = POWER_STATE_INITIAL;
	fsm.entered_at_ms = now;
	fsm.suspend_to_active_deadline_ms = now + SUSPEND_TO_ACTIVE_PERIOD_MS;
	fsm.lorawan_uplink_deadline_ms = now + APP_LORAWAN_UPLINK_INTERVAL_MS;
	fsm.deep_sleep_wakeup_period_ms = DEEP_SLEEP_WAKE_PERIOD_MS;
	fsm.deep_sleep_wakeup_deadline_ms = now + DEEP_SLEEP_WAKE_PERIOD_MS;
	fsm.active_next_period_deadline_ms = now;
	fsm.active_exit_deadline_ms = now + ACTIVE_BURST_MS;
	fsm.active_uplink_pending = false;
	fsm.active_uplink_in_progress = false;
	fsm.active_uplink_type = TASK_COMM_UPLINK_NO_GPS;
	fsm.active_uplink_reason = "init";
	fsm.deep_sleep_prepared = false;
	fsm.codec_zero_fault_streak = 0U;
	atomic_set(&wake_flags, 0);
	atomic_set(&pm_enter_count, 0);
	atomic_set(&pm_exit_count, 0);
	pm_last_enter_state = PM_STATE_ACTIVE;
	pm_last_exit_state = PM_STATE_ACTIVE;
	pm_last_enter_substate = 0U;
	pm_last_exit_substate = 0U;
	pm_enter_seen = 0U;
	pm_exit_seen = 0U;

	if (!pm_notifier_registered) {
		pm_notifier_register(&app_pm_notifier);
		pm_notifier_registered = true;
	}

	power_fsm_log_wakeup_mask(wake_mask);

	// 完成boot，进入初始状态
	power_fsm_enter(POWER_STATE_SUSPEND, "boot_done_enter_suspend");
	return 0;
}

void power_fsm_request_wakeup(uint32_t flags)
{
	uint32_t enabled_mask = power_fsm_enabled_wakeup_mask();
	uint32_t effective_flags = flags & enabled_mask;

	/* Low-battery shutdown is a protection path, not an optional wake source. */
	if ((flags & POWER_WAKE_SRC_LOW_BAT) != 0U) {
		effective_flags |= POWER_WAKE_SRC_LOW_BAT;
	}

	if (effective_flags == 0U) {
		return;
	}

	/* While already ACTIVE, external wake requests are redundant and can
	 * cause unnecessary scheduler churn when IRQs keep firing.
	 */
	if ((fsm.state == POWER_STATE_ACTIVE) &&
	    ((effective_flags & POWER_WAKE_SRC_LOW_BAT) == 0U)) {
		return;
	}

	atomic_val_t prev = atomic_or(&wake_flags, (atomic_val_t)effective_flags);
	if ((prev & (atomic_val_t)effective_flags) == (atomic_val_t)effective_flags) {
		return;
	}

	k_sem_give(&power_fsm_wake_sem);
}

power_state_t power_fsm_get_state(void)
{
	return fsm.state;
}

k_timeout_t power_fsm_next_wait_timeout(void)
{
	if (!APP_LOW_POWER_FSM_ENABLED) {
		return K_FOREVER;
	}

	if (atomic_get(&wake_flags) != 0) {
		return K_NO_WAIT;
	}

	uint32_t now = k_uptime_get_32();
	uint32_t wait_ms = 50U;

	switch (fsm.state) {
	case POWER_STATE_ACTIVE:
		if (fsm.active_uplink_in_progress) {
			wait_ms = ms_until(now, fsm.active_exit_deadline_ms);
			if (wait_ms > ACTIVE_UPLINK_POLL_MS) {
				wait_ms = ACTIVE_UPLINK_POLL_MS;
			}
		} else {
			wait_ms = ms_until(now, fsm.active_next_period_deadline_ms);
			{
				uint32_t exit_wait_ms = ms_until(now, fsm.active_exit_deadline_ms);
				if (exit_wait_ms < wait_ms) {
					wait_ms = exit_wait_ms;
				}
			}
			{
				uint32_t uplink_wait_ms =
					ms_until(now, fsm.lorawan_uplink_deadline_ms);
				if (uplink_wait_ms < wait_ms) {
					wait_ms = uplink_wait_ms;
				}
			}
		}
		break;
	case POWER_STATE_SUSPEND:
		wait_ms = ms_until(now, fsm.suspend_to_active_deadline_ms);
		{
			uint32_t uplink_wait_ms = ms_until(now, fsm.lorawan_uplink_deadline_ms);
			if (uplink_wait_ms < wait_ms) {
				wait_ms = uplink_wait_ms;
			}
		}
		break;
	case POWER_STATE_DEEP_SLEEP:
		wait_ms = fsm.deep_sleep_prepared ?
			ms_until(now, fsm.deep_sleep_wakeup_deadline_ms) : 0U;
		break;
	case POWER_STATE_INITIAL:
	default:
		wait_ms = 0U;
		break;
	}

	return K_MSEC(wait_ms);
}

void power_fsm_wait_for_event(k_timeout_t timeout)
{
	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		return;
	}

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		while (1) {
			if (k_sem_take(&power_fsm_wake_sem, K_MSEC(WDT_SAFE_FEED_SLICE_MS)) == 0) {
				return;
			}
			task_watchdog_mark_main_alive();
			task_watchdog_feed();
		}
	}

	uint32_t deadline_ms = k_uptime_get_32() + k_ticks_to_ms_floor32(timeout.ticks);
	while (1) {
		uint32_t now = k_uptime_get_32();
		if (time_reached(now, deadline_ms)) {
			return;
		}

		uint32_t remain_ms = deadline_ms - now;
		uint32_t slice_ms = (remain_ms > WDT_SAFE_FEED_SLICE_MS) ?
			WDT_SAFE_FEED_SLICE_MS : remain_ms;

		if (k_sem_take(&power_fsm_wake_sem, K_MSEC(slice_ms)) == 0) {
			return;
		}

		task_watchdog_mark_main_alive();
		task_watchdog_feed();
	}
}

void power_fsm_notify_active_done(void)
{
	if (fsm.state == POWER_STATE_ACTIVE) {
		power_fsm_enter(POWER_STATE_SUSPEND, "active_done");
	}
}

static void power_fsm_tick_initial(uint32_t now)
{
	ARG_UNUSED(now);
	power_fsm_enter(POWER_STATE_SUSPEND, "initial_complete");
}

static void power_fsm_wake_suspend_to_active(const char *reason)
{
	task_inference_resume_after_wakeup();
	power_fsm_enter(POWER_STATE_ACTIVE, reason);
}

static bool power_fsm_prepare_timed_uplink(uint32_t now)
{
	if (!time_reached(now, fsm.lorawan_uplink_deadline_ms)) {
		return false;
	}

	if (fsm.active_uplink_pending || fsm.active_uplink_in_progress ||
	    task_comm_is_pending_or_busy()) {
		return false;
	}

	task_comm_uplink_type_t uplink_type =
		APP_MICROPHONE_DEBUG_SKIP_GPS_LORA_ENABLED ?
		TASK_COMM_UPLINK_NO_GPS : TASK_COMM_UPLINK_WAIT_GPS;
	const char *uplink_reason = APP_MICROPHONE_DEBUG_SKIP_GPS_LORA_ENABLED ?
		"mic_debug_skip_gps_lora" : "gps_resync_default";
	uint32_t epoch = 0U;

	if (APP_MICROPHONE_DEBUG_SKIP_GPS_LORA_ENABLED) {
		LOG_INF("[PWR] microphone debug mode: timed uplink prepared without GPS");
	} else if (task_rtc_get_epoch_local(&epoch) == 0) {
		LOG_INF("[PWR] timed uplink timestamp(local_epoch)=%u", (unsigned int)epoch);
		if (!power_fsm_should_resync_gps_time(&uplink_reason)) {
			uplink_type = TASK_COMM_UPLINK_NO_GPS;
		}
	} else {
		LOG_WRN("[PWR] RTC invalid, timed uplink type fallback to WAIT_GPS");
		uplink_reason = "rtc_epoch_invalid";
	}

	task_sensor_sample();
	task_inference_snapshot_uplink_window();
	advance_deadline(&fsm.lorawan_uplink_deadline_ms,
			 APP_LORAWAN_UPLINK_INTERVAL_MS, now);

	fsm.active_uplink_type = uplink_type;
	fsm.active_uplink_reason = uplink_reason;
	fsm.active_uplink_pending = true;
	fsm.active_exit_deadline_ms = now;
	task_status_led_event(STATUS_LED_UPLINK_READY);
	task_watchdog_feed();
	LOG_INF("[PWR] timed infer window ready: deferred comm uplink prepared (type=%d reason=%s next_in=%u ms)",
		(int)uplink_type,
		uplink_reason,
		(unsigned int)ms_until(k_uptime_get_32(), fsm.lorawan_uplink_deadline_ms));
	if (uplink_type == TASK_COMM_UPLINK_WAIT_GPS) {
		LOG_INF("[PWR] WAIT_GPS required: reason=%s timeout=%u ms",
			uplink_reason,
			(unsigned int)APP_GPS_UPLINK_WAIT_TIMEOUT_MS);
	}

	return true;
}

static void power_fsm_tick_active(uint32_t now)
{
	if (fsm.active_uplink_in_progress) {
		if (!task_comm_is_pending_or_busy()) {
			LOG_INF("[PWR] deferred comm uplink complete, entering suspend");
			fsm.active_uplink_in_progress = false;
			power_fsm_enter(POWER_STATE_SUSPEND, "uplink_complete");
			return;
		}

		if (time_reached(now, fsm.active_exit_deadline_ms)) {
			LOG_INF("[PWR] comm uplink still busy, keep ACTIVE without capture");
			fsm.active_exit_deadline_ms = now + ACTIVE_UPLINK_WAIT_MS;
		}

		return;
	}

	if (task_comm_is_pending_or_busy()) {
		LOG_INF("[PWR] comm is pending or busy, pause capture until uplink completes");
		fsm.active_uplink_in_progress = true;
		fsm.active_exit_deadline_ms = now + ACTIVE_UPLINK_WAIT_MS;
		return;
	}

	if (power_fsm_prepare_timed_uplink(now)) {
		return;
	}

	if (time_reached(now, fsm.active_exit_deadline_ms)) {
		if (fsm.active_uplink_pending) {
			LOG_INF("[PWR] active end: trigger deferred comm uplink (type=%d reason=%s)",
				(int)fsm.active_uplink_type,
				fsm.active_uplink_reason != NULL ? fsm.active_uplink_reason : "unknown");
			task_watchdog_feed();
			task_comm_request_uplink_typed(fsm.active_uplink_type);
			task_watchdog_feed();
			fsm.active_uplink_pending = false;
			fsm.active_uplink_in_progress = true;
			fsm.active_exit_deadline_ms = k_uptime_get_32() + ACTIVE_UPLINK_WAIT_MS;
			return;
		}

		if (task_comm_is_pending_or_busy()) {
			LOG_INF("[PWR] active end: comm still pending/busy, pause capture");
			fsm.active_uplink_in_progress = true;
			fsm.active_exit_deadline_ms = k_uptime_get_32() + ACTIVE_UPLINK_WAIT_MS;
			return;
		}

		power_fsm_enter(POWER_STATE_SUSPEND, "active_burst_complete");
		return;
	}

	if (time_reached(now, fsm.active_next_period_deadline_ms)) {
		task_watchdog_feed();
		bool capture_ok = task_microphone_capture_once();
		task_watchdog_feed();
		if (!capture_ok) {
			if (app_state.mic_codec_zero_fault) {
				if (fsm.codec_zero_fault_streak < UINT8_MAX) {
					fsm.codec_zero_fault_streak++;
				}
				LOG_WRN("[PWR] codec zero fault streak=%u/%u",
					(unsigned int)fsm.codec_zero_fault_streak,
					(unsigned int)CODEC_ZERO_FAULT_SHUTDOWN_STREAK);
				app_state.mic_codec_zero_fault = false;

				if (fsm.codec_zero_fault_streak >= CODEC_ZERO_FAULT_SHUTDOWN_STREAK) {
					LOG_ERR("[PWR] codec zero fault threshold reached; arm RTC wake in %u sec and shutdown",
						(unsigned int)(CODEC_ZERO_FAULT_SHUTDOWN_WAKE_MS / 1000U));
					fsm.codec_zero_fault_streak = 0U;
					power_fsm_enter_deep_sleep_with_period("codec_zero_fault_threshold",
						CODEC_ZERO_FAULT_SHUTDOWN_WAKE_MS);
					return;
				}

				LOG_WRN("[PWR] codec zero fault detected; send one RMS=0 LoRa fault uplink then suspend");
				task_inference_prepare_codec_zero_fault_uplink();
				fsm.active_uplink_pending = false;
				fsm.active_uplink_type = TASK_COMM_UPLINK_NO_GPS;
				fsm.active_uplink_reason = "codec_zero_fault";
				task_status_led_event(STATUS_LED_UPLINK_READY);
				task_comm_request_uplink_typed(TASK_COMM_UPLINK_NO_GPS);
				fsm.active_uplink_in_progress = true;
				fsm.active_exit_deadline_ms = k_uptime_get_32() + ACTIVE_UPLINK_WAIT_MS;
				return;
			}

			if (fsm.active_uplink_pending) {
				power_fsm_reset_codec_zero_fault_streak("capture_failed_pending_uplink");
				LOG_WRN("[PWR] capture failed/incomplete while uplink pending; send pending uplink first");
				task_comm_request_uplink_typed(fsm.active_uplink_type);
				fsm.active_uplink_pending = false;
				fsm.active_uplink_in_progress = true;
				fsm.active_exit_deadline_ms = k_uptime_get_32() + ACTIVE_UPLINK_WAIT_MS;
				return;
			}

			power_fsm_reset_codec_zero_fault_streak("capture_failed_non_zero_fault");
			LOG_WRN("[PWR] capture failed/incomplete; end ACTIVE cycle to reset codec next wake");
			power_fsm_enter(POWER_STATE_SUSPEND, "active_capture_failed");
			return;
		}
		power_fsm_reset_codec_zero_fault_streak("capture_ok");
		// task_imu_sample();
		(void)power_fsm_prepare_timed_uplink(k_uptime_get_32());

		fsm.active_next_period_deadline_ms = k_uptime_get_32() + ACTIVE_CAPTURE_GAP_MS;
	}
}

static void power_fsm_tick_suspend(uint32_t now)
{
	uint32_t wake = (uint32_t)atomic_set(&wake_flags, 0);

	if ((wake & POWER_WAKE_SRC_LOW_BAT) != 0U) {
		LOG_WRN("[PWR] SUSPEND->DEEP_SLEEP reason=low_battery_event");
		power_fsm_enter_deep_sleep_default("low_battery_event");
		return;
	}

	if ((wake & POWER_WAKE_SRC_BUTTON) != 0U) {
		app_state.button_press_count++;
		power_fsm_wake_suspend_to_active("button_irq_wakeup");
		return;
	}

	if ((wake & POWER_WAKE_SRC_COMM) != 0U) {
		power_fsm_wake_suspend_to_active("comm_wakeup");
		return;
	}

	/* RTC alarm wake flags are only meaningful for runtime alarm callbacks.
	 * Deep-sleep shutdown wake returns via cold boot in main(), not here.
	 */

	if ((wake & POWER_WAKE_SRC_SOUND) != 0U) {
		power_fsm_wake_suspend_to_active("sound_irq_wakeup");
		return;
	}

	if (time_reached(now, fsm.suspend_to_active_deadline_ms)) {
		power_fsm_request_wakeup(POWER_WAKE_SRC_PERIODIC);
		advance_deadline(&fsm.suspend_to_active_deadline_ms,
				SUSPEND_TO_ACTIVE_PERIOD_MS, now);
	}

	if (time_reached(now, fsm.lorawan_uplink_deadline_ms)) {
		power_fsm_wake_suspend_to_active("lorawan_uplink_interval");
		return;
	}

	if ((wake & POWER_WAKE_SRC_PERIODIC) != 0U) {
		power_fsm_wake_suspend_to_active("periodic_window");
	}
}

static void power_fsm_tick_deep_sleep(uint32_t now)
{
	ARG_UNUSED(now);

	if (!fsm.deep_sleep_prepared) {
		fsm.deep_sleep_prepared = true;

		uint32_t wake_period_ms = (fsm.deep_sleep_wakeup_period_ms != 0U) ?
			fsm.deep_sleep_wakeup_period_ms : DEEP_SLEEP_WAKE_PERIOD_MS;
		uint32_t wake_sec = (wake_period_ms + 999U) / 1000U;

		LOG_WRN("[PWR] deep sleep path entering unified shutdown in %u ms wake window",
			(unsigned int)wake_period_ms);

		int deep_sleep_ret = app_deep_sleep_enter(wake_sec, "power_fsm");
		if (deep_sleep_ret < 0) {
			LOG_ERR("[PWR] deep_sleep abort: entry failed: %d", deep_sleep_ret);
			LOG_ERR("[PWR] deep_sleep abort: keep system running, skip sys_poweroff");
			fsm.deep_sleep_prepared = false;
			power_fsm_enter(POWER_STATE_SUSPEND, "deep_sleep_entry_failed");
			return;
		}
	}

	if (time_reached(k_uptime_get_32(), fsm.deep_sleep_wakeup_deadline_ms)) {
		power_fsm_enter(POWER_STATE_INITIAL, "deep_sleep_timeout_or_simulated");
	}
}

void power_fsm_tick(void)
{
	if (!APP_LOW_POWER_FSM_ENABLED) {
		return;
	}

	if ((fsm.state != POWER_STATE_DEEP_SLEEP) &&
	    (((uint32_t)atomic_get(&wake_flags) & POWER_WAKE_SRC_LOW_BAT) != 0U)) {
		(void)atomic_and(&wake_flags, (atomic_val_t)(~POWER_WAKE_SRC_LOW_BAT));
		LOG_WRN("[PWR] %s->DEEP_SLEEP reason=low_battery_event",
			state_to_str(fsm.state));
		power_fsm_enter_deep_sleep_default("low_battery_event");
		return;
	}

	power_fsm_log_pm_activity_if_any();

	uint32_t now = k_uptime_get_32();

	switch (fsm.state) {
	case POWER_STATE_INITIAL:
		power_fsm_tick_initial(now);
		break;
	case POWER_STATE_ACTIVE:
		power_fsm_tick_active(now);
		break;
	case POWER_STATE_SUSPEND:
		power_fsm_tick_suspend(now);
		break;
	case POWER_STATE_DEEP_SLEEP:
		power_fsm_tick_deep_sleep(now);
		break;
	default:
		power_fsm_enter(POWER_STATE_INITIAL, "invalid_state_recover");
		break;
	}
}
