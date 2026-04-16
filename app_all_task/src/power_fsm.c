#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/state.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/poweroff.h>

#include "tasks.h"
#include "app_feature_flags.h"
#include "retained_state.h"

LOG_MODULE_REGISTER(power_fsm, LOG_LEVEL_INF);

#define ACTIVE_BURST_MS                  (30U * 1000U)
#define ACTIVE_CAPTURE_GAP_MS            100U
#define GPS_UPLINK_WAIT_TIMEOUT_MS       20000U
#define GPS_RESYNC_PERIOD_SEC            (3U * 60U * 60U)
#define SUSPEND_TO_ACTIVE_PERIOD_MS      (1U * 60U * 1000U)
/* #define DEEP_SLEEP_WAKE_PERIOD_MS   (30U * 60U * 1000U) */
#define DEEP_SLEEP_WAKE_PERIOD_MS        (30U * 1000U)
#define WDT_SAFE_FEED_SLICE_MS           8000U

struct power_fsm_ctx {
	power_state_t state;
	uint32_t entered_at_ms;
	uint32_t suspend_to_active_deadline_ms;
	uint32_t deep_sleep_wakeup_deadline_ms;
	uint32_t active_next_period_deadline_ms;
	uint32_t active_exit_deadline_ms;
	bool deep_sleep_prepared;
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

	fsm.state = next;
	fsm.entered_at_ms = now;

	switch (next) {
	case POWER_STATE_ACTIVE:
		task_ina3221_block_active(true);
		fsm.active_next_period_deadline_ms = now;
		fsm.active_exit_deadline_ms = now + ACTIVE_BURST_MS;
		/* 恢复电源 */
		(void)power_ctrl_vperiph_on();
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
		task_ina3221_block_active(false);
		fsm.suspend_to_active_deadline_ms = now + SUSPEND_TO_ACTIVE_PERIOD_MS;
		/* 卸载SD卡、关闭高耗电外设、进入常规低功耗准备（保留sound wakeup） */
		(void)power_ctrl_prepare_suspend();
		break;
	case POWER_STATE_DEEP_SLEEP:
		task_ina3221_block_active(false);
		fsm.deep_sleep_wakeup_deadline_ms = now + DEEP_SLEEP_WAKE_PERIOD_MS;
		fsm.deep_sleep_prepared = false;
		break;
	case POWER_STATE_INITIAL:
	default:
		break;
	}

	LOG_INF("[PWR] %s->%s reason=%s", state_to_str(prev), state_to_str(next), reason);
}


static bool power_fsm_should_resync_gps_time(void)
{
	uint32_t now_epoch = 0U;
	struct retained_state_v1 retained;
	uint32_t delta = 0U;

	if (task_rtc_get_epoch_utc(&now_epoch) < 0) {
		return true;
	}

	if (retained_state_load(&retained) < 0) {
		return true;
	}

	if (retained.last_gps_sync_epoch == 0U) {
		return true;
	}

	if (!safe_elapsed(now_epoch, retained.last_gps_sync_epoch, &delta)) {
		return true;
	}

	return delta >= GPS_RESYNC_PERIOD_SEC;
}

static void power_fsm_run_periodic_gps_resync_if_needed(void)
{
	bool rtc_synced = false;

	if (!power_fsm_should_resync_gps_time()) {
		return;
	}

	LOG_INF("[PWR] GPS resync due, start timed sync (%u ms)",
		(unsigned int)GPS_UPLINK_WAIT_TIMEOUT_MS);
	if (!task_gps_acquire_with_timeout(GPS_UPLINK_WAIT_TIMEOUT_MS, &rtc_synced)) {
		LOG_WRN("[PWR] GPS resync timed out, keep current RTC/uplink flow");
		return;
	}

	if (!rtc_synced) {
		LOG_WRN("[PWR] GPS fix acquired but RTC sync not confirmed during resync");
		return;
	}

	LOG_INF("[PWR] GPS periodic resync complete");
}

int power_fsm_init(void)
{
	uint32_t now = k_uptime_get_32();
	uint32_t wake_mask = power_fsm_enabled_wakeup_mask();

	fsm.state = POWER_STATE_INITIAL;
	fsm.entered_at_ms = now;
	fsm.suspend_to_active_deadline_ms = now + SUSPEND_TO_ACTIVE_PERIOD_MS;
	fsm.deep_sleep_wakeup_deadline_ms = now + DEEP_SLEEP_WAKE_PERIOD_MS;
	fsm.active_next_period_deadline_ms = now;
	fsm.active_exit_deadline_ms = now + ACTIVE_BURST_MS;
	fsm.deep_sleep_prepared = false;
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
		wait_ms = ms_until(now, fsm.active_next_period_deadline_ms);
		{
			uint32_t exit_wait_ms = ms_until(now, fsm.active_exit_deadline_ms);
			if (exit_wait_ms < wait_ms) {
				wait_ms = exit_wait_ms;
			}
		}
		break;
	case POWER_STATE_SUSPEND:
		wait_ms = ms_until(now, fsm.suspend_to_active_deadline_ms);
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

static void power_fsm_tick_active(uint32_t now)
{
	if (task_comm_is_pending_or_busy()) {
		LOG_WRN("[PWR] comm is pending or busy, extend active burst deadline");
		fsm.active_exit_deadline_ms = now + 20000U;
		if (time_reached(now, fsm.active_next_period_deadline_ms)) {
			fsm.active_next_period_deadline_ms = now + 20000U;
		}
		return;
	}

	if (time_reached(now, fsm.active_exit_deadline_ms)) {
		/* 准备退出 ACTIVE，检查是否需要同步 GPS */
		task_watchdog_feed();
		power_fsm_run_periodic_gps_resync_if_needed();
		task_watchdog_feed();
		power_fsm_enter(POWER_STATE_SUSPEND, "active_burst_complete");
		return;
	}

	if (time_reached(now, fsm.active_next_period_deadline_ms)) {
		task_watchdog_feed();
		task_microphone_capture_once();
		task_watchdog_feed();
		// task_imu_sample();

		if (app_state.infer_window_ready) {
			/* 推理十次（即 infer_window_ready 为 true）时触发 LoRa 发送 */
			task_comm_uplink_type_t uplink_type = TASK_COMM_UPLINK_WAIT_GPS;
			uint32_t epoch = 0U;
			if (task_rtc_get_epoch_utc(&epoch) == 0) {
				LOG_INF("[PWR] uplink timestamp(epoch)=%u", (unsigned int)epoch);
				if (!power_fsm_should_resync_gps_time()) {
					uplink_type = TASK_COMM_UPLINK_NO_GPS;
				}
			} else {
				LOG_WRN("[PWR] RTC invalid, uplink type fallback to WAIT_GPS");
			}

			task_sensor_sample();
			task_inference_snapshot_uplink_window();
			task_watchdog_feed();
			task_comm_request_uplink_typed(uplink_type);
			task_watchdog_feed();
		}
		// power_fsm_run_gps_drift_sync_if_needed();

		fsm.active_next_period_deadline_ms = k_uptime_get_32() + ACTIVE_CAPTURE_GAP_MS;

		if (task_comm_is_pending_or_busy()) {
			LOG_WRN("[PWR] comm is pending or busy, extend active burst deadline");
			fsm.active_exit_deadline_ms = k_uptime_get_32() + 20000U;
			fsm.active_next_period_deadline_ms = k_uptime_get_32() + 20000U;
			return;
		}
	}
}

static void power_fsm_tick_suspend(uint32_t now)
{
	uint32_t wake = (uint32_t)atomic_set(&wake_flags, 0);

	if ((wake & POWER_WAKE_SRC_LOW_BAT) != 0U) {
		LOG_WRN("[PWR] SUSPEND->DEEP_SLEEP reason=low_battery_event");
		power_fsm_enter(POWER_STATE_DEEP_SLEEP, "low_battery_event");
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

	if ((wake & POWER_WAKE_SRC_PERIODIC) != 0U) {
		power_fsm_wake_suspend_to_active("periodic_window");
	}
}

static void power_fsm_tick_deep_sleep(uint32_t now)
{
	ARG_UNUSED(now);

	if (!fsm.deep_sleep_prepared) {
		int alarm_ret;
		int prep_ret;

		fsm.deep_sleep_prepared = true;

		alarm_ret = task_rtc_set_alarm_in_seconds(DEEP_SLEEP_WAKE_PERIOD_MS / 1000U);
		if (alarm_ret < 0) {
			LOG_ERR("[PWR] deep_sleep abort: rtc alarm setup failed: %d", alarm_ret);
			LOG_ERR("[PWR] deep_sleep abort: keep system running, skip sys_poweroff");
			fsm.deep_sleep_prepared = false;
			power_fsm_enter(POWER_STATE_SUSPEND, "deep_sleep_alarm_failed");
			return;
		}
		LOG_INF("[PWR] deep_sleep rtc_alarm ret=%d", alarm_ret);

		prep_ret = power_ctrl_prepare_deep_sleep_all();
		LOG_INF("[PWR] deep_sleep_prepare ret=%d", prep_ret);
		if (prep_ret < 0) {
			LOG_ERR("[PWR] deep_sleep abort: hardware prepare failed: %d", prep_ret);
			LOG_ERR("[PWR] deep_sleep abort: keep system running, skip sys_poweroff");
			fsm.deep_sleep_prepared = false;
			power_fsm_enter(POWER_STATE_SUSPEND, "deep_sleep_prepare_failed");
			return;
		}

		LOG_WRN("[PWR] deep sleep path entering sys_poweroff in %u ms wake window",
			(unsigned int)DEEP_SLEEP_WAKE_PERIOD_MS);
		LOG_WRN("[PWR] deep sleep final state: alarm armed, rails prepared");
		k_msleep(5000);

		if (APP_DEEP_SLEEP_ENABLED && !APP_DEEP_SLEEP_SIMULATE_ENABLED) {
			task_rtc_prepare_shutdown_wakeup_route();
			k_msleep(200);
			sys_poweroff();
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
		power_fsm_enter(POWER_STATE_DEEP_SLEEP, "low_battery_event");
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
