/*
 * SD Card Management and Status Persistence
 * Mount, list root directory, and write application state with self-initialization
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/kvss/nvs.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <errno.h>
#include <ff.h>
#include <string.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(storage_task, LOG_LEVEL_INF);

#include "tasks.h"
#include "retained_state.h"

/* SD card device and mount structure */
static const struct device *sdhc_dev = DEVICE_DT_GET(DT_NODELABEL(sdhc0));

static FATFS fat_fs_data;
static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.mnt_point = "/SD:",
	.fs_data = &fat_fs_data,
	.storage_dev = (void *)"SD"
};

/* Initialization flag */
static bool sd_initialized = false;
static struct fs_file_t pcm_file;
static bool pcm_file_opened = false;
static bool pcm_file_inited = false;
static atomic_t storage_sleep_preparing = ATOMIC_INIT(0);
static atomic_t storage_local_write_blocked = ATOMIC_INIT(0);
K_MUTEX_DEFINE(storage_fs_lock);
static const uint64_t storage_pcm_min_free_bytes = (256ULL * 1024ULL);
static uint32_t pcm_write_seq = 0U;
static uint64_t pcm_write_total_bytes = 0U;
static uint32_t pcm_write_total_ms = 0U;
static uint32_t pcm_write_max_ms = 0U;
static uint32_t storage_session_id = 0U;
#define STORAGE_FS_LOCK_SHORT_WAIT_MS 100U
#define STORAGE_PCM_CLOSE_WAIT_MS 1000U
#define STORAGE_SD_WRITE_OP_TIMEOUT_MS 5000U
#define STORAGE_SD_CLOSE_OP_TIMEOUT_MS 5000U
#define STORAGE_SD_OPEN_OP_TIMEOUT_MS 5000U
#define STORAGE_SD_PROBE_OP_TIMEOUT_MS 10000U
#define STORAGE_SD_MOUNT_OP_TIMEOUT_MS 15000U
#define STORAGE_SD_RECOVER_OP_TIMEOUT_MS 5000U
#define STORAGE_SD_UNMOUNT_LOCK_WAIT_MS 250U
#define STORAGE_SD_UNMOUNT_OP_TIMEOUT_MS 5000U
#define STORAGE_SD_FUSE_FAST_FAILURE_THRESHOLD 3U
#define STORAGE_SD_FUSE_LOG_PERIOD_MS 60000U
#define STORAGE_SD_RETAINED_OP_CODE 1U
#define STORAGE_WAV_SAMPLE_RATE 16000U
#define STORAGE_WAV_CHANNELS 1U
#define STORAGE_WAV_BITS_PER_SAMPLE 16U
#define STORAGE_WAV_HEADER_SIZE 44U
#define STORAGE_DEV_EUI_PATH "/SD:/dev.txt"
#define STORAGE_GPS_COORDS_PATH "/SD:/gps.txt"
#define STORAGE_NVS_PARTITION storage_partition
#define STORAGE_NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(STORAGE_NVS_PARTITION)
#define STORAGE_NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(STORAGE_NVS_PARTITION)
#define STORAGE_NVS_PARTITION_SIZE FIXED_PARTITION_SIZE(STORAGE_NVS_PARTITION)
#define STORAGE_NVS_LORAWAN_DEVNONCE_ID 1U

static struct nvs_fs storage_nvs;
static bool storage_nvs_initialized;
static bool storage_nvs_ready;
K_MUTEX_DEFINE(storage_nvs_lock);

static struct k_thread storage_pcm_close_thread_data;
K_THREAD_STACK_DEFINE(storage_pcm_close_stack, 2048);
K_SEM_DEFINE(storage_pcm_close_req_sem, 0, 1);
K_SEM_DEFINE(storage_pcm_close_done_sem, 0, 1);
static bool storage_pcm_close_thread_started;
static atomic_t storage_pcm_close_pending = ATOMIC_INIT(0);
K_MUTEX_DEFINE(storage_sd_fuse_lock);
static bool storage_sd_fuse_initialized;
static bool storage_sd_fuse_active_flag;
static uint32_t storage_sd_fuse_until_epoch;
static uint32_t storage_sd_fuse_until_uptime_ms;
static uint32_t storage_sd_fuse_last_log_ms;

static void storage_pcm_close_worker_thread(void *a, void *b, void *c);
static void storage_pcm_close_worker_start_once(void);
static void storage_pcm_end_locked(void);
static void storage_clear_sd_state(void);
static void storage_note_sd_no_space(uint64_t free_bytes, const char *reason);

void task_storage_set_session_id(uint32_t session_id)
{
	storage_session_id = session_id;
}

bool task_storage_local_write_blocked(void)
{
	return (atomic_get(&storage_local_write_blocked) != 0) ||
	       task_storage_sd_fuse_active();
}

static void storage_pcm_close_worker_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (1) {
		k_sem_take(&storage_pcm_close_req_sem, K_FOREVER);

		k_mutex_lock(&storage_fs_lock, K_FOREVER);
		storage_pcm_end_locked();
		k_mutex_unlock(&storage_fs_lock);

		atomic_set(&storage_pcm_close_pending, 0);
		k_sem_give(&storage_pcm_close_done_sem);
	}
}

static void storage_pcm_close_worker_start_once(void)
{
	if (storage_pcm_close_thread_started) {
		return;
	}

	k_tid_t tid = k_thread_create(&storage_pcm_close_thread_data,
				      storage_pcm_close_stack,
				      K_THREAD_STACK_SIZEOF(storage_pcm_close_stack),
				      storage_pcm_close_worker_thread,
				      NULL, NULL, NULL,
				      K_LOWEST_APPLICATION_THREAD_PRIO,
				      0, K_NO_WAIT);
	k_thread_name_set(tid, "storage_pcm_close");
	storage_pcm_close_thread_started = true;
}

static bool storage_time_reached32(uint32_t now, uint32_t deadline)
{
	return (int32_t)(now - deadline) >= 0;
}

static uint32_t storage_sd_fuse_backoff_sec(uint16_t strikes)
{
	if (strikes <= 1U) {
		return 30U * 60U;
	}
	if (strikes == 2U) {
		return 2U * 60U * 60U;
	}
	if (strikes == 3U) {
		return 6U * 60U * 60U;
	}
	return 24U * 60U * 60U;
}

static bool storage_sd_get_epoch(uint32_t *epoch)
{
	if (epoch == NULL) {
		return false;
	}

	return task_rtc_get_epoch_local(epoch) == 0;
}

static void storage_sd_fuse_set_app_state_locked(bool active)
{
	storage_sd_fuse_active_flag = active;
	if (active) {
		storage_clear_sd_state();
		app_state.sdon = APP_SDON_FUSED;
	} else if (app_state.sdon == APP_SDON_FUSED) {
		app_state.sdon = APP_SDON_NO_RECENT_WRITE;
	}
}

static void storage_sd_fuse_activate_locked(struct retained_state_v1 *state,
					    const char *reason)
{
	uint16_t strikes = state->sd_fuse_strikes;
	if (strikes < UINT16_MAX) {
		strikes++;
	}

	uint32_t backoff_sec = storage_sd_fuse_backoff_sec(strikes);
	uint32_t now_epoch = 0U;
	bool have_epoch = storage_sd_get_epoch(&now_epoch);

	state->sd_fuse_strikes = strikes;
	state->sd_fail_streak = 0U;
	state->sd_op_pending_magic = 0U;
	state->sd_last_op_code = 0U;
	if (have_epoch) {
		state->sd_last_fault_epoch = now_epoch;
		state->sd_fuse_until_epoch = now_epoch + backoff_sec;
		storage_sd_fuse_until_epoch = state->sd_fuse_until_epoch;
		storage_sd_fuse_until_uptime_ms = 0U;
	} else {
		state->sd_last_fault_epoch = 0U;
		state->sd_fuse_until_epoch = 0U;
		storage_sd_fuse_until_epoch = 0U;
		storage_sd_fuse_until_uptime_ms =
			k_uptime_get_32() + (backoff_sec * 1000U);
	}

	(void)retained_state_store(state);
	storage_sd_fuse_set_app_state_locked(true);
	LOG_WRN("SD fuse active: reason=%s strikes=%u backoff=%u sec%s%u",
		reason != NULL ? reason : "unknown",
		(unsigned int)strikes,
		(unsigned int)backoff_sec,
		have_epoch ? " until_epoch=" : " until_uptime_ms=",
		(unsigned int)(have_epoch ? storage_sd_fuse_until_epoch :
			       storage_sd_fuse_until_uptime_ms));
}

static void storage_sd_fuse_clear_locked(const char *reason)
{
	struct retained_state_v1 state;
	int ret = retained_state_load(&state);
	if (ret == 0) {
		state.sd_fuse_until_epoch = 0U;
		state.sd_last_fault_epoch = 0U;
		state.sd_fuse_strikes = 0U;
		state.sd_fail_streak = 0U;
		state.sd_op_pending_magic = 0U;
		state.sd_last_op_code = 0U;
		(void)retained_state_store(&state);
	}

	storage_sd_fuse_until_epoch = 0U;
	storage_sd_fuse_until_uptime_ms = 0U;
	storage_sd_fuse_set_app_state_locked(false);
	LOG_INF("SD fuse cleared: %s", reason != NULL ? reason : "success");
}

static void storage_sd_fuse_refresh_locked(void)
{
	struct retained_state_v1 state;
	uint32_t now_epoch = 0U;

	if (!storage_sd_fuse_initialized) {
		storage_sd_fuse_initialized = true;
		int ret = retained_state_load(&state);
		if (ret < 0) {
			LOG_WRN("SD fuse retained state unavailable: %d", ret);
			return;
		}

		if (state.sd_op_pending_magic != 0U) {
			storage_sd_fuse_activate_locked(&state, "previous_sd_op_reset");
			return;
		}

		storage_sd_fuse_until_epoch = state.sd_fuse_until_epoch;
		if (state.sd_fuse_until_epoch != 0U) {
			if (storage_sd_get_epoch(&now_epoch)) {
				if (!storage_time_reached32(now_epoch, state.sd_fuse_until_epoch)) {
					storage_sd_fuse_set_app_state_locked(true);
					LOG_WRN("SD fuse restored: until_epoch=%u strikes=%u",
						(unsigned int)state.sd_fuse_until_epoch,
						(unsigned int)state.sd_fuse_strikes);
					return;
				}
			} else {
				storage_sd_fuse_set_app_state_locked(true);
				LOG_WRN("SD fuse restored without valid RTC; keep disabled this boot");
				return;
			}

			state.sd_fuse_until_epoch = 0U;
			state.sd_fail_streak = 0U;
			(void)retained_state_store(&state);
			storage_sd_fuse_until_epoch = 0U;
		}
	}

	if (!storage_sd_fuse_active_flag) {
		return;
	}

	if (storage_sd_fuse_until_epoch != 0U) {
		if (storage_sd_get_epoch(&now_epoch) &&
		    storage_time_reached32(now_epoch, storage_sd_fuse_until_epoch)) {
			storage_sd_fuse_clear_locked("backoff expired");
		}
		return;
	}

	if (storage_sd_fuse_until_uptime_ms != 0U &&
	    storage_time_reached32(k_uptime_get_32(), storage_sd_fuse_until_uptime_ms)) {
		storage_sd_fuse_clear_locked("uptime backoff expired");
	}
}

void task_storage_sd_fuse_init(void)
{
	k_mutex_lock(&storage_sd_fuse_lock, K_FOREVER);
	storage_sd_fuse_refresh_locked();
	k_mutex_unlock(&storage_sd_fuse_lock);
}

bool task_storage_sd_fuse_active(void)
{
	bool active;

	k_mutex_lock(&storage_sd_fuse_lock, K_FOREVER);
	storage_sd_fuse_refresh_locked();
	active = storage_sd_fuse_active_flag;
	if (active &&
	    storage_time_reached32(k_uptime_get_32(),
				   storage_sd_fuse_last_log_ms + STORAGE_SD_FUSE_LOG_PERIOD_MS)) {
		storage_sd_fuse_last_log_ms = k_uptime_get_32();
		LOG_WRN("SD fuse suppressing non-mount SD access; mount retries every ACTIVE (sdon=%u)",
			(unsigned int)APP_SDON_FUSED);
	}
	k_mutex_unlock(&storage_sd_fuse_lock);

	return active;
}

static void storage_sd_fuse_note_failure(const char *reason)
{
	struct retained_state_v1 state;

	k_mutex_lock(&storage_sd_fuse_lock, K_FOREVER);
	storage_sd_fuse_refresh_locked();
	if (storage_sd_fuse_active_flag) {
		k_mutex_unlock(&storage_sd_fuse_lock);
		return;
	}

	int ret = retained_state_load(&state);
	if (ret < 0) {
		LOG_WRN("SD failure not retained (%s): %d",
			reason != NULL ? reason : "unknown", ret);
		k_mutex_unlock(&storage_sd_fuse_lock);
		return;
	}

	if (state.sd_fail_streak < UINT16_MAX) {
		state.sd_fail_streak++;
	}

	if (state.sd_fail_streak >= STORAGE_SD_FUSE_FAST_FAILURE_THRESHOLD) {
		storage_sd_fuse_activate_locked(&state, reason);
	} else {
		(void)retained_state_store(&state);
		LOG_WRN("SD failure streak %u/%u: %s",
			(unsigned int)state.sd_fail_streak,
			(unsigned int)STORAGE_SD_FUSE_FAST_FAILURE_THRESHOLD,
			reason != NULL ? reason : "unknown");
	}

	k_mutex_unlock(&storage_sd_fuse_lock);
}

static void storage_sd_fuse_note_success(const char *reason)
{
	struct retained_state_v1 state;

	k_mutex_lock(&storage_sd_fuse_lock, K_FOREVER);
	storage_sd_fuse_refresh_locked();
	if (storage_sd_fuse_active_flag) {
		storage_sd_fuse_clear_locked(reason);
		k_mutex_unlock(&storage_sd_fuse_lock);
		return;
	}

	int ret = retained_state_load(&state);
	if (ret == 0 && (state.sd_fail_streak != 0U || state.sd_fuse_strikes != 0U ||
			 state.sd_fuse_until_epoch != 0U)) {
		state.sd_fail_streak = 0U;
		state.sd_fuse_strikes = 0U;
		state.sd_fuse_until_epoch = 0U;
		state.sd_last_fault_epoch = 0U;
		(void)retained_state_store(&state);
		LOG_INF("SD failure streak cleared: %s",
			reason != NULL ? reason : "success");
	}

	k_mutex_unlock(&storage_sd_fuse_lock);
}

static void storage_sd_op_start(const char *op_name, uint32_t timeout_ms)
{
	(void)retained_state_sd_op_start(STORAGE_SD_RETAINED_OP_CODE);
	task_watchdog_note_sd_op_start(op_name, timeout_ms);
}

static void storage_sd_op_end(void)
{
	task_watchdog_note_sd_op_end();
	(void)retained_state_sd_op_end();
}

#define task_watchdog_note_sd_op_start storage_sd_op_start
#define task_watchdog_note_sd_op_end storage_sd_op_end

static void storage_clear_sd_state(void)
{
	app_state.sd_mounted = false;
	app_state.sd_listed_once = false;
	app_state.sd_capacity_valid = false;
	app_state.sd_total_mib = 0U;
	app_state.sd_free_mib = 0U;
}

static void storage_note_sd_io_lost(int io_ret)
{
	ARG_UNUSED(io_ret);
	if (!app_state.sd_mounted) {
		return;
	}

	/* If we get hard I/O errors, assume the card/mount is no longer usable. */
	LOG_WRN("SD I/O error suggests mount lost (ret=%d); marking unmounted", io_ret);
	/* Close any open PCM file handle before unmounting. */
	if (pcm_file_opened) {
		task_watchdog_note_sd_op_start("sd_recover_close", STORAGE_SD_RECOVER_OP_TIMEOUT_MS);
		(void)fs_close(&pcm_file);
		task_watchdog_note_sd_op_end();
		pcm_file_opened = false;
	}
	task_watchdog_note_sd_op_start("sd_recover_unmount", STORAGE_SD_RECOVER_OP_TIMEOUT_MS);
	(void)fs_unmount(&fatfs_mnt);
	task_watchdog_note_sd_op_end();
	storage_clear_sd_state();
	app_state.sdon = APP_SDON_NO_RECENT_WRITE;
	storage_sd_fuse_note_failure("sd_io_lost");
}

static void storage_note_sd_no_space(uint64_t free_bytes, const char *reason)
{
	bool first_report = atomic_cas(&storage_local_write_blocked, 0, 1);

	if (first_report) {
		LOG_ERR("SD local storage disabled for this ACTIVE: free=%lluB min=%lluB usage=%u%% reason=%s; unmounting",
			free_bytes,
			storage_pcm_min_free_bytes,
			(unsigned int)app_state.sd_usage_percent,
			reason != NULL ? reason : "no_space");
	}

	if (pcm_file_opened) {
		task_watchdog_note_sd_op_start("sd_no_space_close", STORAGE_SD_CLOSE_OP_TIMEOUT_MS);
		(void)fs_close(&pcm_file);
		task_watchdog_note_sd_op_end();
		pcm_file_opened = false;
	}

	task_watchdog_note_sd_op_start("sd_no_space_unmount", STORAGE_SD_UNMOUNT_OP_TIMEOUT_MS);
	int ret = fs_unmount(&fatfs_mnt);
	task_watchdog_note_sd_op_end();
	if (ret < 0 && first_report) {
		LOG_WRN("SD unmount after no-space failed: %d", ret);
	}

	app_state.sd_mounted = false;
	app_state.sd_listed_once = false;
}

static bool storage_probe_sd_capacity(uint32_t *total_mib, uint32_t *free_mib,
				     uint8_t *usage_percent, uint64_t *free_bytes,
				     FRESULT *out_fres)
{
	DWORD free_clusters = 0;
	FATFS *fs = NULL;
	task_watchdog_note_sd_op_start("sd_getfree", STORAGE_SD_PROBE_OP_TIMEOUT_MS);
	FRESULT fres = f_getfree("SD:", &free_clusters, &fs);
	task_watchdog_note_sd_op_end();
	if (out_fres != NULL) {
		*out_fres = fres;
	}
	if (fres != FR_OK || fs == NULL || fs->n_fatent <= 2U) {
		return false;
	}

	uint32_t total_clusters = (uint32_t)fs->n_fatent - 2U;
	uint32_t sectors_per_cluster = (uint32_t)fs->csize;
	uint64_t total_sectors = (uint64_t)total_clusters * sectors_per_cluster;
	uint64_t free_sectors = (uint64_t)free_clusters * sectors_per_cluster;
	uint64_t total_mib_u64 = total_sectors >> 11; /* 512-byte sector -> MiB */
	uint64_t free_mib_u64 = free_sectors >> 11;

	uint32_t percent = 0U;
	if (total_clusters > 0U) {
		uint32_t used_clusters = total_clusters - (uint32_t)free_clusters;
		percent = (used_clusters * 100U) / total_clusters;
		if (percent > 100U) {
			percent = 100U;
		}
	}

	if (total_mib != NULL) {
		*total_mib = (uint32_t)total_mib_u64;
	}
	if (free_mib != NULL) {
		*free_mib = (uint32_t)free_mib_u64;
	}
	if (usage_percent != NULL) {
		*usage_percent = (uint8_t)percent;
	}
	if (free_bytes != NULL) {
		*free_bytes = free_sectors * 512ULL;
	}
	return true;
}

static uint8_t storage_get_sd_usage_percent(void)
{
	uint32_t total_mib = 0U;
	uint32_t free_mib = 0U;
	uint8_t usage = 0U;
	FRESULT fres = FR_INVALID_OBJECT;
	if (!storage_probe_sd_capacity(&total_mib, &free_mib, &usage, NULL, &fres)) {
		return 0U;
	}
	return usage;
}

static int storage_nvs_init_once(void)
{
	if (storage_nvs_initialized) {
		return storage_nvs_ready ? 0 : -ENODEV;
	}

	storage_nvs_initialized = true;
	storage_nvs.flash_device = STORAGE_NVS_PARTITION_DEVICE;
	if (!device_is_ready(storage_nvs.flash_device)) {
		LOG_ERR("NVS flash device not ready");
		return -ENODEV;
	}

	struct flash_pages_info info;
	int ret = flash_get_page_info_by_offs(storage_nvs.flash_device,
					      STORAGE_NVS_PARTITION_OFFSET,
					      &info);
	if (ret < 0) {
		LOG_ERR("NVS flash page info failed: %d", ret);
		return ret;
	}

	if (info.size == 0U || STORAGE_NVS_PARTITION_SIZE < (info.size * 2U)) {
		LOG_ERR("NVS partition too small: partition=%u page=%u",
			(unsigned int)STORAGE_NVS_PARTITION_SIZE,
			(unsigned int)info.size);
		return -ENOSPC;
	}

	storage_nvs.offset = STORAGE_NVS_PARTITION_OFFSET;
	storage_nvs.sector_size = info.size;
	storage_nvs.sector_count = (uint16_t)(STORAGE_NVS_PARTITION_SIZE / info.size);

	ret = nvs_mount(&storage_nvs);
	if (ret < 0) {
		LOG_ERR("NVS mount failed: %d", ret);
		return ret;
	}

	storage_nvs_ready = true;
	LOG_INF("NVS ready for LoRaWAN DevNonce (offset=0x%lx sector=%u count=%u)",
		(unsigned long)storage_nvs.offset,
		(unsigned int)storage_nvs.sector_size,
		(unsigned int)storage_nvs.sector_count);
	return 0;
}

static bool storage_get_sd_capacity_with_free_bytes(uint32_t *total_mib,
						    uint32_t *free_mib,
						    uint8_t *usage_percent,
						    uint64_t *free_bytes)
{
	FRESULT fres = FR_INVALID_OBJECT;
	return storage_probe_sd_capacity(total_mib, free_mib, usage_percent, free_bytes, &fres);
}

/**
 * SD Card Controller Initialization (called on first task execution)
 * Checks controller readiness
 */
static int init_sd_controller(void)
{
	if (!device_is_ready(sdhc_dev)) {
		LOG_ERR("SDHC device not ready");
		return -1;
	}
	LOG_INF("SDHC device ready");
	return 0;
}

static struct k_work_delayable sd_mount_work;
static struct k_work_sync sd_mount_cancel_sync;
static int sd_mount_retry_count = 0;
static bool sd_mount_work_inited = false;
#define SD_MOUNT_MAX_RETRIES 5

static void sd_mount_worker(struct k_work *work);

static void task_sd_mount_async_init_once(void)
{
	if (sd_mount_work_inited) {
		return;
	}

	k_work_init_delayable(&sd_mount_work, sd_mount_worker);
	sd_mount_work_inited = true;
}

static void sd_mount_worker(struct k_work *work)
{
	if (atomic_get(&storage_sleep_preparing) || app_state.sd_mounted ||
	    atomic_get(&storage_local_write_blocked)) {
		return;
	}

	if (sd_mount_retry_count >= SD_MOUNT_MAX_RETRIES) {
		LOG_WRN("Background SD mount failed after %d attempts, giving up for this ACTIVE period", SD_MOUNT_MAX_RETRIES);
		return;
	}

	sd_mount_retry_count++;
	LOG_INF("Background SD mount attempt %d/%d...", sd_mount_retry_count, SD_MOUNT_MAX_RETRIES);
	task_sd_mount();

	if (!atomic_get(&storage_sleep_preparing) &&
	    !atomic_get(&storage_local_write_blocked) &&
	    !app_state.sd_mounted && (sd_mount_retry_count < SD_MOUNT_MAX_RETRIES)) {
		/* 挂载失败，1秒后重试 */
		k_work_reschedule(&sd_mount_work, K_MSEC(1000));
	}
}

void task_sd_mount_async(void)
{
	task_sd_mount_async_init_once();
	atomic_set(&storage_sleep_preparing, 0);
	atomic_set(&storage_local_write_blocked, 0);
	sd_mount_retry_count = 0; // 每个 ACTIVE 周期重置计数器
	k_work_reschedule(&sd_mount_work, K_NO_WAIT);
}

void task_sd_mount_async_cancel(void)
{
	task_sd_mount_async_init_once();
	atomic_set(&storage_sleep_preparing, 1);
	bool was_busy = k_work_cancel_delayable_sync(&sd_mount_work, &sd_mount_cancel_sync);
	if (was_busy) {
		LOG_INF("Background SD mount stopped before sleep");
	}
	sd_mount_retry_count = 0;
}

void task_sd_mount(void)
{
	if (atomic_get(&storage_sleep_preparing) ||
	    atomic_get(&storage_local_write_blocked)) {
		return;
	}

	/* First execution: Initialize SD controller */
	if (!sd_initialized) {
		if (init_sd_controller() == 0) {
			sd_initialized = true;
			LOG_INF("TASK-5/6/7 (Storage) initialized");
		} else {
			LOG_ERR("TASK-5/6/7 initialization failed");
			return;
		}
	}

	/* Already mounted - idempotent operation */
	if (app_state.sd_mounted) {
		return;
	}


	/* Retry mounting up to 2 times */
	int ret = -EIO;
	for (int attempt = 1; attempt <= 2; attempt++) {
		if (atomic_get(&storage_sleep_preparing)) {
			LOG_INF("SD mount aborted: sleep preparation started");
			return;
		}

		LOG_INF("SD mount attempt %d/2...", attempt);
		task_watchdog_feed();

		/* Add delay before each attempt to let SD card stabilize */
		if (attempt > 1) {
			task_watchdog_feed();
			k_msleep(500);
		}

		task_watchdog_note_sd_op_start("sd_mount", STORAGE_SD_MOUNT_OP_TIMEOUT_MS);
		ret = fs_mount(&fatfs_mnt);
		task_watchdog_note_sd_op_end();
		if (ret == 0) {
			uint32_t total_mib = 0U;
			uint32_t free_mib = 0U;
			uint8_t usage_percent = 0U;
			uint64_t free_bytes = 0U;
			if (storage_get_sd_capacity_with_free_bytes(&total_mib, &free_mib, &usage_percent, &free_bytes)) {
				app_state.sd_capacity_valid = true;
				app_state.sd_total_mib = total_mib;
				app_state.sd_free_mib = free_mib;
				app_state.sd_usage_percent = usage_percent;
				LOG_INF("SD Capacity: Total=%u MiB, Free=%u MiB, Usage=%u%%, FreeBytes=%llu",
				        total_mib, free_mib, usage_percent, free_bytes);
				if (free_bytes < storage_pcm_min_free_bytes) {
					storage_note_sd_no_space(free_bytes, "mount_capacity_check");
					return;
				}
			}
			else {
				LOG_WRN("Failed to get SD capacity after mount");
			}
			app_state.sd_mounted = true;
			app_state.sd_listed_once = false;
			storage_sd_fuse_note_success("sd_mount");
			LOG_INF("SD card mounted at %s (attempt %d)",
				fatfs_mnt.mnt_point, attempt);
			return;
		}

		if (atomic_get(&storage_sleep_preparing)) {
			LOG_INF("SD mount stopped before sleep after attempt %d (ret=%d)", attempt, ret);
			return;
		}

		LOG_WRN("SD mount attempt %d failed: %d", attempt, ret);
	}

	LOG_ERR("SD mount failed after 2 attempts (last error: %d)", ret);
	storage_sd_fuse_note_failure("sd_mount_failed");
}

void task_sd_ensure_mounted(void)
{
	if (atomic_get(&storage_local_write_blocked)) {
		return;
	}

	/* Avoid racing FATFS operations while recording. */
	if (app_state.mic_recording) {
		return;
	}

	/* If flag says mounted, do a lightweight probe to confirm the mount still works. */
	if (app_state.sd_mounted) {
		uint32_t total_mib = 0U;
		uint32_t free_mib = 0U;
		uint8_t usage = 0U;
		FRESULT fres = FR_INVALID_OBJECT;
		if (!storage_probe_sd_capacity(&total_mib, &free_mib, &usage, NULL, &fres)) {
			LOG_WRN("SD probe failed (fres=%u); will remount", (unsigned int)fres);
			storage_sd_fuse_note_failure("sd_probe_failed");
			task_watchdog_note_sd_op_start("sd_probe_unmount", STORAGE_SD_UNMOUNT_OP_TIMEOUT_MS);
			(void)fs_unmount(&fatfs_mnt);
			task_watchdog_note_sd_op_end();
			storage_clear_sd_state();
		} else {
			/* Keep usage telemetry reasonably fresh for uplink. */
			app_state.sd_capacity_valid = true;
			app_state.sd_total_mib = total_mib;
			app_state.sd_free_mib = free_mib;
			app_state.sd_usage_percent = usage;
			return;
		}
	}

	task_sd_mount();
}

void task_sd_check_capacity(void)
{
	if (atomic_get(&storage_local_write_blocked)) {
		return;
	}

	if (task_storage_sd_fuse_active()) {
		app_state.sd_capacity_valid = false;
		return;
	}

	if (!app_state.sd_mounted) {
		app_state.sd_capacity_valid = false;
		return;
	}

	/* Avoid long FATFS queries while recording audio. */
	if (app_state.mic_recording) {
		return;
	}

	uint32_t total_mib = 0U;
	uint32_t free_mib = 0U;
	uint8_t usage = 0U;
	FRESULT fres = FR_INVALID_OBJECT;
	if (!storage_probe_sd_capacity(&total_mib, &free_mib, &usage, NULL, &fres)) {
		LOG_WRN("Failed to read SD capacity (fres=%u)", (unsigned int)fres);
		app_state.sd_capacity_valid = false;
		storage_sd_fuse_note_failure("sd_capacity_failed");
		/* If mount is broken, drop the mounted flag so next window can remount. */
		storage_clear_sd_state();
		return;
	}

	app_state.sd_capacity_valid = true;
	app_state.sd_total_mib = total_mib;
	app_state.sd_free_mib = free_mib;
	app_state.sd_usage_percent = usage;
	LOG_INF("SD capacity: free=%u MiB total=%u MiB usage=%u%%",
		(unsigned int)free_mib,
		(unsigned int)total_mib,
		(unsigned int)usage);
}

static bool storage_get_rtc_time(struct rtc_time *tm)
{
	if (tm == NULL) {
		LOG_ERR("Invalid RTC time pointer");
		return false;
	}

	(void)memset(tm, 0, sizeof(*tm));

	bool default_seed = false;
	int ret = task_rtc_get_time_for_storage(tm, &default_seed);
	if (ret != 0) {
		LOG_WRN("Failed to get RTC time for PCM path: %d", ret);
		return false;
	}

	LOG_INF("RTC time read: %04d-%02d-%02d %02d:%02d:%02d France local%s",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec,
		default_seed ? " (default seed)" : "");

	return true;
}

static bool storage_dev_eui_is_valid(const uint8_t dev_eui[8])
{
	bool all_zero = true;
	bool all_ff = true;

	for (size_t i = 0; i < 8U; i++) {
		if (dev_eui[i] != 0U) {
			all_zero = false;
		}
		if (dev_eui[i] != 0xFFU) {
			all_ff = false;
		}
	}

	return !all_zero && !all_ff;
}

static int storage_hex_nibble(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
	return -1;
}

static bool storage_parse_dev_eui_text(const char *text, uint8_t out_dev_eui[8])
{
	if (text == NULL || out_dev_eui == NULL) {
		return false;
	}

	uint8_t parsed[8] = {0};
	int hi_nibble = -1;
	size_t out_i = 0U;

	for (const char *p = text; *p != '\0'; ++p) {
		int nib = storage_hex_nibble(*p);
		if (nib < 0) {
			continue;
		}

		if (hi_nibble < 0) {
			hi_nibble = nib;
		} else {
			if (out_i >= 8U) {
				return false;
			}
			parsed[out_i++] = (uint8_t)((hi_nibble << 4) | nib);
			hi_nibble = -1;
		}
	}

	if (hi_nibble >= 0 || out_i != 8U) {
		return false;
	}

	if (!storage_dev_eui_is_valid(parsed)) {
		return false;
	}

	memcpy(out_dev_eui, parsed, 8U);
	return true;
}

static void storage_format_dev_eui_text(const uint8_t dev_eui[8], char *out, size_t out_sz)
{
	(void)snprintk(out, out_sz,
		"%02X %02X %02X %02X %02X %02X %02X %02X\n",
		dev_eui[0], dev_eui[1], dev_eui[2], dev_eui[3],
		dev_eui[4], dev_eui[5], dev_eui[6], dev_eui[7]);
}

task_storage_status_t task_storage_persist_dev_eui(const uint8_t dev_eui[8])
{
	task_storage_status_t status = TASK_STORAGE_STATUS_OK;

	if (dev_eui == NULL || !storage_dev_eui_is_valid(dev_eui)) {
		return TASK_STORAGE_STATUS_INVALID_ARG;
	}

	if (task_storage_sd_fuse_active()) {
		return TASK_STORAGE_STATUS_NOT_MOUNTED;
	}
	if (atomic_get(&storage_local_write_blocked)) {
		return TASK_STORAGE_STATUS_NO_SPACE;
	}

	if (!app_state.sd_mounted) {
		return TASK_STORAGE_STATUS_NOT_MOUNTED;
	}

	if (k_mutex_lock(&storage_fs_lock, K_MSEC(STORAGE_FS_LOCK_SHORT_WAIT_MS)) != 0) {
		LOG_WRN("DevEUI persist skipped: SD FS busy");
		return TASK_STORAGE_STATUS_IO_ERROR;
	}

	struct fs_file_t file;
	fs_file_t_init(&file);

	task_watchdog_note_sd_op_start("sd_dev_eui_open", STORAGE_SD_OPEN_OP_TIMEOUT_MS);
	int ret = fs_open(&file, STORAGE_DEV_EUI_PATH, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
	task_watchdog_note_sd_op_end();
	if (ret != 0) {
		if (ret == -ENODEV || ret == -EIO) {
			storage_note_sd_io_lost(ret);
		}
		status = TASK_STORAGE_STATUS_IO_ERROR;
		goto out;
	}

	char line[32];
	storage_format_dev_eui_text(dev_eui, line, sizeof(line));
	size_t line_len = strnlen(line, sizeof(line));
	task_watchdog_note_sd_op_start("sd_dev_eui_write", STORAGE_SD_WRITE_OP_TIMEOUT_MS);
	ret = fs_write(&file, line, line_len);
	task_watchdog_note_sd_op_end();
	task_watchdog_note_sd_op_start("sd_dev_eui_close", STORAGE_SD_CLOSE_OP_TIMEOUT_MS);
	(void)fs_close(&file);
	task_watchdog_note_sd_op_end();

	if (ret < 0 || (size_t)ret != line_len) {
		if (ret == -ENODEV || ret == -EIO) {
			storage_note_sd_io_lost(ret);
		}
		status = TASK_STORAGE_STATUS_IO_ERROR;
		goto out;
	}

	LOG_INF("Persisted DevEUI to %s", STORAGE_DEV_EUI_PATH);

out:
	k_mutex_unlock(&storage_fs_lock);
	return status;
}

bool task_storage_load_dev_eui(uint8_t dev_eui[8])
{
	if (dev_eui == NULL) {
		return false;
	}

	if (task_storage_sd_fuse_active()) {
		return false;
	}

	if (!app_state.sd_mounted) {
		return false;
	}

	if (k_mutex_lock(&storage_fs_lock, K_MSEC(STORAGE_FS_LOCK_SHORT_WAIT_MS)) != 0) {
		LOG_WRN("DevEUI load skipped: SD FS busy");
		return false;
	}

	struct fs_file_t file;
	fs_file_t_init(&file);

	task_watchdog_note_sd_op_start("sd_dev_eui_open", STORAGE_SD_OPEN_OP_TIMEOUT_MS);
	int ret = fs_open(&file, STORAGE_DEV_EUI_PATH, FS_O_READ);
	task_watchdog_note_sd_op_end();
	if (ret != 0) {
		k_mutex_unlock(&storage_fs_lock);
		return false;
	}

	char buf[96];
	task_watchdog_note_sd_op_start("sd_dev_eui_read", STORAGE_SD_PROBE_OP_TIMEOUT_MS);
	ret = fs_read(&file, buf, sizeof(buf) - 1U);
	task_watchdog_note_sd_op_end();
	task_watchdog_note_sd_op_start("sd_dev_eui_close", STORAGE_SD_CLOSE_OP_TIMEOUT_MS);
	(void)fs_close(&file);
	task_watchdog_note_sd_op_end();
	if (ret <= 0) {
		k_mutex_unlock(&storage_fs_lock);
		return false;
	}
	buf[ret] = '\0';

	if (!storage_parse_dev_eui_text(buf, dev_eui)) {
		k_mutex_unlock(&storage_fs_lock);
		return false;
	}

	LOG_INF("Loaded DevEUI from %s", STORAGE_DEV_EUI_PATH);
	k_mutex_unlock(&storage_fs_lock);
	return true;
}

int task_storage_reserve_lorawan_dev_nonce(uint16_t seed, uint16_t *dev_nonce)
{
	if (dev_nonce == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&storage_nvs_lock, K_FOREVER);

	int ret = storage_nvs_init_once();
	if (ret < 0) {
		goto out;
	}

	uint32_t next = 0U;
	ssize_t read_len = nvs_read(&storage_nvs, STORAGE_NVS_LORAWAN_DEVNONCE_ID,
				    &next, sizeof(next));
	if (read_len == -ENOENT || read_len == 0) {
		next = seed;
		LOG_INF("LoRaWAN DevNonce counter initialized from seed=%u",
			(unsigned int)next);
	} else if (read_len < 0) {
		ret = (int)read_len;
		LOG_ERR("LoRaWAN DevNonce read failed: %d", ret);
		goto out;
	} else if (read_len != sizeof(next)) {
		next = seed;
		LOG_WRN("LoRaWAN DevNonce counter invalid (len=%d), reset to seed=%u",
			(int)read_len, (unsigned int)next);
	} else if (next > UINT16_MAX) {
		LOG_ERR("LoRaWAN DevNonce exhausted (next=%u); reset device on network before reusing",
			(unsigned int)next);
		ret = -EOVERFLOW;
		goto out;
	}

	uint32_t reserved = next;
	uint32_t following = reserved + 1U;
	ssize_t write_len = nvs_write(&storage_nvs, STORAGE_NVS_LORAWAN_DEVNONCE_ID,
				      &following, sizeof(following));
	if (write_len < 0) {
		ret = (int)write_len;
		LOG_ERR("LoRaWAN DevNonce persist failed: %d", ret);
		goto out;
	}

	*dev_nonce = (uint16_t)reserved;
	ret = 0;
	LOG_INF("LoRaWAN DevNonce reserved: %u (next=%u)",
		(unsigned int)*dev_nonce, (unsigned int)following);

out:
	k_mutex_unlock(&storage_nvs_lock);
	return ret;
}

task_storage_status_t task_storage_persist_gps_coords(float latitude, float longitude)
{
	task_storage_status_t status = TASK_STORAGE_STATUS_OK;

	if ((latitude < -90.0f) || (latitude > 90.0f) ||
	    (longitude < -180.0f) || (longitude > 180.0f)) {
		return TASK_STORAGE_STATUS_INVALID_ARG;
	}

	if (task_storage_sd_fuse_active()) {
		return TASK_STORAGE_STATUS_NOT_MOUNTED;
	}

	if (k_mutex_lock(&storage_fs_lock, K_MSEC(STORAGE_FS_LOCK_SHORT_WAIT_MS)) != 0) {
		LOG_WRN("GPS persist skipped: SD FS busy");
		return TASK_STORAGE_STATUS_IO_ERROR;
	}

	if (!app_state.sd_mounted) {
		status = TASK_STORAGE_STATUS_NOT_MOUNTED;
		goto out;
	}

	struct fs_file_t file;
	fs_file_t_init(&file);

	task_watchdog_note_sd_op_start("sd_gps_open", STORAGE_SD_OPEN_OP_TIMEOUT_MS);
	int ret = fs_open(&file, STORAGE_GPS_COORDS_PATH,
			  FS_O_CREATE | FS_O_WRITE | FS_O_APPEND);
	task_watchdog_note_sd_op_end();
	if (ret != 0) {
		if (ret == -ENODEV || ret == -EIO) {
			storage_note_sd_io_lost(ret);
		}
		status = TASK_STORAGE_STATUS_IO_ERROR;
		goto out;
	}

	char line[96];
	int len;

	if (app_state.time_valid) {
		len = snprintk(line, sizeof(line),
			       "%04u-%02u-%02u %02u:%02u:%02u,%.7f,%.7f\n",
			       (unsigned int)app_state.time_year,
			       (unsigned int)app_state.time_month,
			       (unsigned int)app_state.time_day,
			       (unsigned int)app_state.time_hour,
			       (unsigned int)app_state.time_minute,
			       (unsigned int)app_state.time_second,
			       (double)latitude, (double)longitude);
	} else {
		len = snprintk(line, sizeof(line), "%.7f,%.7f\n",
			       (double)latitude, (double)longitude);
	}

	if (len <= 0 || len >= (int)sizeof(line)) {
		task_watchdog_note_sd_op_start("sd_gps_close", STORAGE_SD_CLOSE_OP_TIMEOUT_MS);
		(void)fs_close(&file);
		task_watchdog_note_sd_op_end();
		status = TASK_STORAGE_STATUS_IO_ERROR;
		goto out;
	}

	task_watchdog_note_sd_op_start("sd_gps_write", STORAGE_SD_WRITE_OP_TIMEOUT_MS);
	ret = fs_write(&file, line, (size_t)len);
	task_watchdog_note_sd_op_end();
	task_watchdog_note_sd_op_start("sd_gps_close", STORAGE_SD_CLOSE_OP_TIMEOUT_MS);
	(void)fs_close(&file);
	task_watchdog_note_sd_op_end();
	if (ret < 0 || ret != len) {
		if (ret == -ENODEV || ret == -EIO) {
			storage_note_sd_io_lost(ret);
		}
		status = TASK_STORAGE_STATUS_IO_ERROR;
		goto out;
	}

	LOG_INF("GPS coords appended to %s: lat=%.7f lon=%.7f",
		STORAGE_GPS_COORDS_PATH, (double)latitude, (double)longitude);

out:
	k_mutex_unlock(&storage_fs_lock);
	return status;
}

static bool storage_parse_gps_coords_line(const char *line, float *latitude, float *longitude)
{
	if (line == NULL || latitude == NULL || longitude == NULL) {
		return false;
	}

	const char *last_comma = strrchr(line, ',');
	if (last_comma == NULL) {
		return false;
	}

	const char *prev_comma = NULL;
	for (const char *p = last_comma; p > line;) {
		p--;
		if (*p == ',') {
			prev_comma = p;
			break;
		}
	}

	const char *lat_text = (prev_comma != NULL) ? (prev_comma + 1) : line;
	const char *lon_text = last_comma + 1;
	char *end = NULL;

	float lat = strtof(lat_text, &end);
	if (end == lat_text) {
		return false;
	}

	float lon = strtof(lon_text, &end);
	if (end == lon_text) {
		return false;
	}

	if ((lat < -90.0f) || (lat > 90.0f) ||
	    (lon < -180.0f) || (lon > 180.0f)) {
		return false;
	}

	if ((lat == 0.0f) && (lon == 0.0f)) {
		return false;
	}

	*latitude = lat;
	*longitude = lon;
	return true;
}

bool task_storage_load_gps_coords(float *latitude, float *longitude)
{
	if (latitude == NULL || longitude == NULL) {
		return false;
	}

	if (task_storage_sd_fuse_active()) {
		return false;
	}

	if (!app_state.sd_mounted) {
		return false;
	}

	if (k_mutex_lock(&storage_fs_lock, K_MSEC(STORAGE_FS_LOCK_SHORT_WAIT_MS)) != 0) {
		LOG_WRN("GPS load skipped: SD FS busy");
		return false;
	}

	struct fs_file_t file;
	fs_file_t_init(&file);

	struct fs_dirent ent;
	task_watchdog_note_sd_op_start("sd_gps_stat", STORAGE_SD_PROBE_OP_TIMEOUT_MS);
	int ret = fs_stat(STORAGE_GPS_COORDS_PATH, &ent);
	task_watchdog_note_sd_op_end();
	if (ret != 0 || ent.type != FS_DIR_ENTRY_FILE || ent.size == 0U) {
		if (ret == -ENODEV || ret == -EIO) {
			storage_note_sd_io_lost(ret);
		}
		k_mutex_unlock(&storage_fs_lock);
		return false;
	}

	task_watchdog_note_sd_op_start("sd_gps_open", STORAGE_SD_OPEN_OP_TIMEOUT_MS);
	ret = fs_open(&file, STORAGE_GPS_COORDS_PATH, FS_O_READ);
	task_watchdog_note_sd_op_end();
	if (ret != 0) {
		if (ret == -ENODEV || ret == -EIO) {
			storage_note_sd_io_lost(ret);
		}
		k_mutex_unlock(&storage_fs_lock);
		return false;
	}

	char buf[512];
	size_t read_size = ent.size < sizeof(buf) - 1U ? ent.size : sizeof(buf) - 1U;
	off_t read_offset = (off_t)(ent.size - read_size);
	if (read_offset > 0) {
		task_watchdog_note_sd_op_start("sd_gps_seek", STORAGE_SD_PROBE_OP_TIMEOUT_MS);
		ret = fs_seek(&file, read_offset, FS_SEEK_SET);
		task_watchdog_note_sd_op_end();
		if (ret != 0) {
			task_watchdog_note_sd_op_start("sd_gps_close", STORAGE_SD_CLOSE_OP_TIMEOUT_MS);
			(void)fs_close(&file);
			task_watchdog_note_sd_op_end();
			if (ret == -ENODEV || ret == -EIO) {
				storage_note_sd_io_lost(ret);
			}
			k_mutex_unlock(&storage_fs_lock);
			return false;
		}
	}

	task_watchdog_note_sd_op_start("sd_gps_read", STORAGE_SD_PROBE_OP_TIMEOUT_MS);
	ret = fs_read(&file, buf, read_size);
	task_watchdog_note_sd_op_end();
	task_watchdog_note_sd_op_start("sd_gps_close", STORAGE_SD_CLOSE_OP_TIMEOUT_MS);
	(void)fs_close(&file);
	task_watchdog_note_sd_op_end();
	if (ret <= 0) {
		if (ret == -ENODEV || ret == -EIO) {
			storage_note_sd_io_lost(ret);
		}
		k_mutex_unlock(&storage_fs_lock);
		return false;
	}
	buf[ret] = '\0';

	float found_lat = 0.0f;
	float found_lon = 0.0f;
	bool found = false;
	char *line = buf;

	if (read_offset > 0) {
		char *first_newline = strchr(buf, '\n');
		if (first_newline == NULL) {
			k_mutex_unlock(&storage_fs_lock);
			return false;
		}
		line = first_newline + 1;
	}

	while (line != NULL && *line != '\0') {
		char *next = strchr(line, '\n');
		if (next != NULL) {
			*next = '\0';
		}

		size_t line_len = strlen(line);
		if (line_len > 0U && line[line_len - 1U] == '\r') {
			line[line_len - 1U] = '\0';
		}

		float lat = 0.0f;
		float lon = 0.0f;
		if (storage_parse_gps_coords_line(line, &lat, &lon)) {
			found_lat = lat;
			found_lon = lon;
			found = true;
		}

		if (next == NULL) {
			break;
		}
		line = next + 1;
	}

	if (!found) {
		k_mutex_unlock(&storage_fs_lock);
		return false;
	}

	*latitude = found_lat;
	*longitude = found_lon;
	k_mutex_unlock(&storage_fs_lock);
	return true;
}

static int storage_mkdir_if_needed(const char *path)
{
	if (path == NULL || path[0] == '\0') {
		return -EINVAL;
	}

	struct fs_dirent ent;
	int ret = fs_stat(path, &ent);
	if (ret == 0) {
		if (ent.type == FS_DIR_ENTRY_DIR) {
			return 0;
		}
		LOG_WRN("Path exists but is not a directory: %s", path);
		return -ENOTDIR;
	}

	if (ret != -ENOENT) {
		LOG_WRN("Failed to stat path %s: %d", path, ret);
		return ret;
	}

	ret = fs_mkdir(path);
	if (ret == 0 || ret == -EEXIST) {
		return 0;
	}

	LOG_WRN("Failed to create directory %s: %d", path, ret);
	return ret;
}

static int storage_ensure_audio_parent_dirs(const struct rtc_time *tm)
{
	if (tm == NULL) {
		return -EINVAL;
	}

	int year = tm->tm_year + 1900;
	char day_dir[24];
	char hour_dir[32];
	char minute_dir[40];

	(void)snprintk(day_dir, sizeof(day_dir), "/SD:/%02d-%02d-%02d",
		       year % 100,
		       tm->tm_mon + 1,
		       tm->tm_mday);
	(void)snprintk(hour_dir, sizeof(hour_dir), "%s/%02d",
		       day_dir,
		       tm->tm_hour);
	(void)snprintk(minute_dir, sizeof(minute_dir), "%s/%02d",
		       hour_dir,
		       tm->tm_min);

	int ret = storage_mkdir_if_needed(day_dir);
	if (ret != 0) {
		return ret;
	}

	ret = storage_mkdir_if_needed(hour_dir);
	if (ret != 0) {
		return ret;
	}

	return storage_mkdir_if_needed(minute_dir);
}

/**
 * Generate WAV filename with directory structure based on RTC timestamp
 * Path format: /SD:/YY-MM-DD/HH/MM/HH-MM-SS-session.wav
 */
static void storage_make_wav_filename_from_tm(const struct rtc_time *tm, char *out_path, size_t out_sz)
{
	int yy = (tm->tm_year + 1900) % 100;
	int mm = tm->tm_mon + 1;
	int dd = tm->tm_mday;
	int hh = tm->tm_hour;
	int min = tm->tm_min;
	int ss = tm->tm_sec;

	(void)snprintk(out_path, out_sz,
		       "/SD:/%02d-%02d-%02d/%02d/%02d/%02d-%02d-%02d-%04u.wav",
		       yy, mm, dd, hh, min, hh, min, ss,
		       (unsigned int)(storage_session_id % 10000U));
}

static void storage_make_wav_header(uint8_t header[STORAGE_WAV_HEADER_SIZE], uint32_t data_bytes)
{
	const uint32_t byte_rate = STORAGE_WAV_SAMPLE_RATE *
				   STORAGE_WAV_CHANNELS *
				   STORAGE_WAV_BITS_PER_SAMPLE / 8U;
	const uint16_t block_align = (uint16_t)(STORAGE_WAV_CHANNELS *
						STORAGE_WAV_BITS_PER_SAMPLE / 8U);

	(void)memset(header, 0, STORAGE_WAV_HEADER_SIZE);
	memcpy(&header[0], "RIFF", 4);
	sys_put_le32(36U + data_bytes, &header[4]);
	memcpy(&header[8], "WAVE", 4);
	memcpy(&header[12], "fmt ", 4);
	sys_put_le32(16U, &header[16]);
	sys_put_le16(1U, &header[20]);
	sys_put_le16(STORAGE_WAV_CHANNELS, &header[22]);
	sys_put_le32(STORAGE_WAV_SAMPLE_RATE, &header[24]);
	sys_put_le32(byte_rate, &header[28]);
	sys_put_le16(block_align, &header[32]);
	sys_put_le16(STORAGE_WAV_BITS_PER_SAMPLE, &header[34]);
	memcpy(&header[36], "data", 4);
	sys_put_le32(data_bytes, &header[40]);
}

static int storage_write_wav_header(uint32_t data_bytes)
{
	uint8_t header[STORAGE_WAV_HEADER_SIZE];

	storage_make_wav_header(header, data_bytes);
	int ret = fs_write(&pcm_file, header, sizeof(header));
	if (ret < 0) {
		return ret;
	}
	if ((size_t)ret != sizeof(header)) {
		return -ENOSPC;
	}

	return 0;
}

static int storage_finalize_wav_header(void)
{
	if (pcm_write_total_bytes > UINT32_MAX) {
		LOG_WRN("WAV data too large for RIFF header: %lluB",
			pcm_write_total_bytes);
		return -EFBIG;
	}

	int ret = fs_seek(&pcm_file, 0, FS_SEEK_SET);
	if (ret != 0) {
		return ret;
	}

	ret = storage_write_wav_header((uint32_t)pcm_write_total_bytes);
	if (ret != 0) {
		return ret;
	}

	return fs_seek(&pcm_file, 0, FS_SEEK_END);
}

task_storage_status_t task_storage_pcm_begin(char *out_path, size_t out_sz)
{
	task_storage_status_t status = TASK_STORAGE_STATUS_OK;

	if (out_path == NULL || out_sz == 0U) {
		return TASK_STORAGE_STATUS_INVALID_ARG;
	}

	if (task_storage_sd_fuse_active()) {
		return TASK_STORAGE_STATUS_NOT_MOUNTED;
	}

	k_mutex_lock(&storage_fs_lock, K_FOREVER);

	if (atomic_get(&storage_sleep_preparing) || !app_state.sd_mounted) {
		status = TASK_STORAGE_STATUS_NOT_MOUNTED;
		goto out;
	}

	uint8_t usage = 0U;
	uint64_t free_bytes = 0U;
	if (!storage_get_sd_capacity_with_free_bytes(&app_state.sd_total_mib,
						     &app_state.sd_free_mib,
						     &usage,
						     &free_bytes)) {
		app_state.sd_capacity_valid = false;
		storage_sd_fuse_note_failure("sd_pcm_capacity_failed");
		status = TASK_STORAGE_STATUS_CAPACITY_UNAVAILABLE;
		goto out;
	}
	app_state.sd_capacity_valid = true;
	app_state.sd_usage_percent = usage;

	if (free_bytes < storage_pcm_min_free_bytes) {
		storage_note_sd_no_space(free_bytes, "pcm_begin_capacity_check");
		status = TASK_STORAGE_STATUS_NO_SPACE;
		goto out;
	}

	if (!pcm_file_inited) {
		fs_file_t_init(&pcm_file);
		pcm_file_inited = true;
	}
	if (pcm_file_opened) {
		task_watchdog_note_sd_op_start("sd_close_stale_pcm", STORAGE_SD_CLOSE_OP_TIMEOUT_MS);
		fs_close(&pcm_file);
		task_watchdog_note_sd_op_end();
		pcm_file_opened = false;
	}

	struct rtc_time tm;
	if (!storage_get_rtc_time(&tm)) {
		status = TASK_STORAGE_STATUS_IO_ERROR;
		goto out;
	}

	storage_make_wav_filename_from_tm(&tm, out_path, out_sz);
	LOG_INF("Generated WAV file path: %s", out_path);

	task_watchdog_note_sd_op_start("sd_mkdirs", STORAGE_SD_WRITE_OP_TIMEOUT_MS);
	int mkdir_ret = storage_ensure_audio_parent_dirs(&tm);
	task_watchdog_note_sd_op_end();
	if (mkdir_ret != 0) {
		LOG_WRN("Failed to prepare audio directory tree for %s", out_path);
	}

	task_watchdog_note_sd_op_start("sd_open_pcm", STORAGE_SD_OPEN_OP_TIMEOUT_MS);
	int ret = fs_open(&pcm_file, out_path, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
	task_watchdog_note_sd_op_end();
	if (ret != 0) {
		if (ret == -ENODEV || ret == -EIO) {
			storage_note_sd_io_lost(ret);
		}
		status = TASK_STORAGE_STATUS_IO_ERROR;
		goto out;
	}

	pcm_file_opened = true;
	pcm_write_seq = 0U;
	pcm_write_total_bytes = 0U;
	pcm_write_total_ms = 0U;
	pcm_write_max_ms = 0U;

	task_watchdog_note_sd_op_start("sd_header_pcm", STORAGE_SD_WRITE_OP_TIMEOUT_MS);
	ret = storage_write_wav_header(0U);
	task_watchdog_note_sd_op_end();
	if (ret != 0) {
		LOG_WRN("Failed to write WAV header for %s: %d", out_path, ret);
		if (ret == -ENODEV || ret == -EIO) {
			storage_note_sd_io_lost(ret);
		} else {
			task_watchdog_note_sd_op_start("sd_close_pcm_error", STORAGE_SD_CLOSE_OP_TIMEOUT_MS);
			(void)fs_close(&pcm_file);
			task_watchdog_note_sd_op_end();
			pcm_file_opened = false;
		}
		status = TASK_STORAGE_STATUS_IO_ERROR;
		goto out;
	}

	app_state.sd_pcm_write_enabled = true;
	app_state.sd_pcm_blocks = 0U;
	app_state.sd_pcm_total_bytes = 0U;
	app_state.sd_pcm_total_ms = 0U;
	app_state.sd_pcm_max_ms = 0U;
	app_state.sd_pcm_avg_ms = 0U;
	app_state.sd_pcm_last_block_ms = 0U;

out:
	k_mutex_unlock(&storage_fs_lock);
	return status;
}

task_storage_status_t task_storage_pcm_write(const void *data, size_t len)
{
	task_storage_status_t status = TASK_STORAGE_STATUS_OK;

	if (task_storage_sd_fuse_active()) {
		return TASK_STORAGE_STATUS_NOT_MOUNTED;
	}
	if (atomic_get(&storage_local_write_blocked)) {
		return TASK_STORAGE_STATUS_NO_SPACE;
	}

	k_mutex_lock(&storage_fs_lock, K_FOREVER);

	if (atomic_get(&storage_sleep_preparing)) {
		status = TASK_STORAGE_STATUS_NOT_MOUNTED;
		goto out;
	}

	if (!pcm_file_opened) {
		status = TASK_STORAGE_STATUS_NOT_OPEN;
		goto out;
	}
	if (data == NULL || len == 0U) {
		status = TASK_STORAGE_STATUS_INVALID_ARG;
		goto out;
	}

	const uint32_t seq = pcm_write_seq + 1U;
	LOG_DBG("PIPE_EVT,session=%u,evt=sd_write,phase=start,seq=%u,bytes=%u",
		(unsigned int)storage_session_id,
		(unsigned int)seq,
		(unsigned int)len);

	uint32_t t0 = k_uptime_get_32();
	task_watchdog_note_sd_op_start("sd_write_pcm", STORAGE_SD_WRITE_OP_TIMEOUT_MS);
	int ret = fs_write(&pcm_file, data, len);
	task_watchdog_note_sd_op_end();
	uint32_t cost_ms = k_uptime_get_32() - t0;
	app_state.sd_pcm_last_block_ms = cost_ms;

	LOG_DBG("PIPE_EVT,session=%u,evt=sd_write,phase=end,seq=%u,bytes=%u,dur_ms=%u,ret=%d",
		(unsigned int)storage_session_id,
		(unsigned int)seq,
		(unsigned int)len,
		(unsigned int)cost_ms,
		ret);

	if (ret < 0) {
		LOG_INF("SD write fail: seq=%u req=%uB cost=%ums ret=%d",
			(unsigned int)seq,
			(unsigned int)len,
			(unsigned int)cost_ms,
			ret);
		if (ret == -ENOSPC) {
			storage_note_sd_no_space(0U, "pcm_write_enospc");
			status = TASK_STORAGE_STATUS_NO_SPACE;
			goto out;
		}
		if (ret == -ENODEV || ret == -EIO) {
			storage_note_sd_io_lost(ret);
		}
		status = TASK_STORAGE_STATUS_IO_ERROR;
		goto out;
	}
	if ((size_t)ret != len) {
		LOG_INF("SD write short: seq=%u req=%uB wrote=%uB cost=%ums",
			(unsigned int)seq,
			(unsigned int)len,
			(unsigned int)ret,
			(unsigned int)cost_ms);
		storage_note_sd_no_space(0U, "pcm_write_short");
		status = TASK_STORAGE_STATUS_NO_SPACE;
		goto out;
	}

	pcm_write_seq++;
	pcm_write_total_bytes += (uint64_t)len;
	pcm_write_total_ms += cost_ms;
	if (cost_ms > pcm_write_max_ms) {
		pcm_write_max_ms = cost_ms;
	}

	app_state.sdon = APP_SDON_WRITE_OK;
	storage_sd_fuse_note_success("sd_pcm_write");

out:
	k_mutex_unlock(&storage_fs_lock);
	return status;
}

static void storage_pcm_end_locked(void)
{
	if (!pcm_file_opened) {
		return;
	}
	task_watchdog_note_sd_op_start("sd_finalize_pcm", STORAGE_SD_WRITE_OP_TIMEOUT_MS);
	int ret = storage_finalize_wav_header();
	task_watchdog_note_sd_op_end();
	if (ret != 0) {
		LOG_WRN("Failed to finalize WAV header: %d", ret);
		if (ret == -ENODEV || ret == -EIO) {
			storage_note_sd_io_lost(ret);
			return;
		}
	}
	task_watchdog_note_sd_op_start("sd_close_pcm", STORAGE_SD_CLOSE_OP_TIMEOUT_MS);
	ret = fs_close(&pcm_file);
	task_watchdog_note_sd_op_end();
	pcm_file_opened = false;
	if (ret < 0) {
		LOG_WRN("Failed to close WAV file: %d", ret);
		if (ret == -ENODEV || ret == -EIO) {
			storage_note_sd_io_lost(ret);
			return;
		}
	}
	uint32_t avg_ms = (pcm_write_seq > 0U) ? (pcm_write_total_ms / pcm_write_seq) : 0U;
	uint32_t kb_per_s = (pcm_write_total_ms > 0U) ?
		(uint32_t)((pcm_write_total_bytes * 1000ULL) / (uint64_t)pcm_write_total_ms / 1024ULL) : 0U;
	LOG_INF("SD write summary: blocks=%u total=%uB total_cost=%ums max=%ums avg=%ums speed=%uKB/s",
		(unsigned int)pcm_write_seq,
		(unsigned int)pcm_write_total_bytes,
		(unsigned int)pcm_write_total_ms,
		(unsigned int)pcm_write_max_ms,
		(unsigned int)avg_ms,
		(unsigned int)kb_per_s);

	app_state.sd_pcm_blocks = pcm_write_seq;
	app_state.sd_pcm_total_bytes = (uint32_t)pcm_write_total_bytes;
	app_state.sd_pcm_total_ms = pcm_write_total_ms;
	app_state.sd_pcm_max_ms = pcm_write_max_ms;
	app_state.sd_pcm_avg_ms = avg_ms;
	app_state.sd_pcm_write_enabled = false;
}

void task_storage_pcm_end(void)
{
	storage_pcm_close_worker_start_once();

	while (k_sem_take(&storage_pcm_close_done_sem, K_NO_WAIT) == 0) {
	}

	if (atomic_cas(&storage_pcm_close_pending, 0, 1)) {
		k_sem_give(&storage_pcm_close_req_sem);
	}

	if (k_sem_take(&storage_pcm_close_done_sem,
		       K_MSEC(STORAGE_PCM_CLOSE_WAIT_MS)) != 0) {
		LOG_WRN("PCM close still pending after %u ms",
			(unsigned int)STORAGE_PCM_CLOSE_WAIT_MS);
	}
}

void task_storage_pcm_end_from_worker(void)
{
	k_mutex_lock(&storage_fs_lock, K_FOREVER);
	storage_pcm_end_locked();
	k_mutex_unlock(&storage_fs_lock);
}

void task_sd_list_root(void)
{
	if (task_storage_sd_fuse_active()) {
		return;
	}

	/* Execute only once */
	if (!app_state.sd_mounted || app_state.sd_listed_once) {
		return;
	}

	/* Avoid directory operations while recording to reduce FS contention */
	if (app_state.mic_recording) {
		return;
	}

	if (k_mutex_lock(&storage_fs_lock, K_MSEC(STORAGE_FS_LOCK_SHORT_WAIT_MS)) != 0) {
		LOG_WRN("SD root listing skipped: SD FS busy");
		return;
	}

	struct fs_dir_t dir;
	fs_dir_t_init(&dir);

	task_watchdog_note_sd_op_start("sd_opendir", STORAGE_SD_PROBE_OP_TIMEOUT_MS);
	int ret = fs_opendir(&dir, "/SD:");
	task_watchdog_note_sd_op_end();
	if (ret != 0) {
		LOG_WRN("fs_opendir failed: %d", ret);
		k_mutex_unlock(&storage_fs_lock);
		return;
	}

	LOG_INF("SD root listing:");

	while (true) {
		struct fs_dirent ent;
		task_watchdog_note_sd_op_start("sd_readdir", STORAGE_SD_PROBE_OP_TIMEOUT_MS);
		ret = fs_readdir(&dir, &ent);
		task_watchdog_note_sd_op_end();
		if (ret != 0) {
			LOG_WRN("fs_readdir failed: %d", ret);
			break;
		}

		/* End of directory */
		if (ent.name[0] == 0) {
			break;
		}

		/* Format output with "/" suffix for directories */
		LOG_INF(" - %s%s", ent.name, 
		        ent.type == FS_DIR_ENTRY_DIR ? "/" : "");
	}

	task_watchdog_note_sd_op_start("sd_closedir", STORAGE_SD_CLOSE_OP_TIMEOUT_MS);
	fs_closedir(&dir);
	task_watchdog_note_sd_op_end();
	app_state.sd_listed_once = true;
	k_mutex_unlock(&storage_fs_lock);
}

void task_status_persist(void)
{
	if (task_storage_sd_fuse_active()) {
		return;
	}

	/* Only write if SD card is mounted */
	if (!app_state.sd_mounted) {
		return;
	}

	/* Skip while recording to avoid concurrent FATFS access */
	if (app_state.mic_recording) {
		return;
	}

	if (k_mutex_lock(&storage_fs_lock, K_MSEC(STORAGE_FS_LOCK_SHORT_WAIT_MS)) != 0) {
		LOG_WRN("Status persist skipped: SD FS busy");
		return;
	}

	/* Status text logging is disabled; keep SD usage telemetry refreshed. */
	app_state.sd_usage_percent = storage_get_sd_usage_percent();
	k_mutex_unlock(&storage_fs_lock);
}

int task_storage_prepare_sleep(void)
{
	atomic_set(&storage_sleep_preparing, 1);
	task_sd_mount_async_cancel();
	if (task_storage_sd_fuse_active()) {
		return 0;
	}
	task_storage_pcm_end();
	if (atomic_get(&storage_pcm_close_pending)) {
		LOG_WRN("SD prepare sleep: PCM close still pending, keep v_periph on");
		return -EBUSY;
	}

	if (k_mutex_lock(&storage_fs_lock,
			 K_MSEC(STORAGE_SD_UNMOUNT_LOCK_WAIT_MS)) != 0) {
		LOG_WRN("SD prepare sleep: FS busy after %u ms, keep v_periph on",
			(unsigned int)STORAGE_SD_UNMOUNT_LOCK_WAIT_MS);
		return -EBUSY;
	}

	if (!app_state.sd_mounted) {
		k_mutex_unlock(&storage_fs_lock);
		return 0;
	}

	task_watchdog_note_sd_op_start("sd_unmount", STORAGE_SD_UNMOUNT_OP_TIMEOUT_MS);
	int ret = fs_unmount(&fatfs_mnt);
	task_watchdog_note_sd_op_end();
	if (ret < 0) {
		LOG_WRN("SD unmount before sleep failed: %d", ret);
		k_mutex_unlock(&storage_fs_lock);
		return ret;
	} else {
		LOG_INF("SD unmounted before sleep");
	}

	storage_clear_sd_state();
	k_mutex_unlock(&storage_fs_lock);
	return 0;
}
