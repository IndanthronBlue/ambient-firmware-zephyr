/*
 * SD Card Management and Status Persistence
 * Mount, list root directory, and write application state with self-initialization
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/drivers/rtc.h>
#include <errno.h>
#include <ff.h>
#include <string.h>

LOG_MODULE_REGISTER(storage_task, LOG_LEVEL_INF);

#include "tasks.h"

/* SD card device and mount structure */
static const struct device *sdhc_dev = DEVICE_DT_GET(DT_NODELABEL(sdhc0));

/* RTC device for timestamp generation */
static const struct device *rtc_dev = DEVICE_DT_GET(DT_NODELABEL(rtc));

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
static const uint64_t storage_pcm_min_free_bytes = (256ULL * 1024ULL);
static uint32_t pcm_write_seq = 0U;
static uint64_t pcm_write_total_bytes = 0U;
static uint32_t pcm_write_total_ms = 0U;
static uint32_t pcm_write_max_ms = 0U;
static uint32_t storage_session_id = 0U;
static const char *storage_unsynced_dir = "/SD:/unsynced";

void task_storage_set_session_id(uint32_t session_id)
{
	storage_session_id = session_id;
}

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
		(void)fs_close(&pcm_file);
		pcm_file_opened = false;
	}
	(void)fs_unmount(&fatfs_mnt);
	storage_clear_sd_state();
}

static bool storage_probe_sd_capacity(uint32_t *total_mib, uint32_t *free_mib,
				     uint8_t *usage_percent, uint64_t *free_bytes,
				     FRESULT *out_fres)
{
	DWORD free_clusters = 0;
	FATFS *fs = NULL;
	FRESULT fres = f_getfree("SD:", &free_clusters, &fs);
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

void task_sd_mount(void)
{
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


	/* Retry mounting up to 5 times */
	int ret = -EIO;
	for (int attempt = 1; attempt <= 5; attempt++) {
		LOG_INF("SD mount attempt %d/5...", attempt);
		task_watchdog_feed();

		/* Add delay before each attempt to let SD card stabilize */
		if (attempt > 1) {
			task_watchdog_feed();
			k_msleep(500);
		}

		ret = fs_mount(&fatfs_mnt);
		if (ret == 0) {
			app_state.sd_mounted = true;
			app_state.sd_listed_once = false;
			LOG_INF("✓ SD card mounted at %s (attempt %d)", 
			        fatfs_mnt.mnt_point, attempt);
			//打印可用容量，storage_get_sd_capacity_with_free_bytes
			uint32_t total_mib = 0U;
			uint32_t free_mib = 0U;
			uint8_t usage_percent = 0U;
			uint64_t free_bytes = 0U;
			if (storage_get_sd_capacity_with_free_bytes(&total_mib, &free_mib, &usage_percent, &free_bytes)) {
				LOG_INF("SD Capacity: Total=%u MiB, Free=%u MiB, Usage=%u%%, FreeBytes=%llu",
				        total_mib, free_mib, usage_percent, free_bytes);
			}
			else {
				LOG_WRN("Failed to get SD capacity after mount");
			}
			return;
		}

		LOG_WRN("SD mount attempt %d failed: %d", attempt, ret);
	}

	LOG_ERR("SD mount failed after 5 attempts (last error: %d)", ret);
}

void task_sd_ensure_mounted(void)
{
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
			(void)fs_unmount(&fatfs_mnt);
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

	if (!device_is_ready(rtc_dev)) {
		LOG_WRN("RTC device not ready");
		return false;
	}

	int ret = rtc_get_time(rtc_dev, tm);
	if (ret != 0) {
		if (ret == -ENODATA) {
			/* RTC has not been set yet (GPS not synced) — not a hardware error */
			LOG_INF("RTC not yet set (no GPS sync), using uptime fallback");
		} else {
			LOG_WRN("Failed to get RTC time: %d", ret);
		}
		return false;
	}

	LOG_INF("RTC time read: %04d-%02d-%02d %02d:%02d:%02d UTC",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	return true;
}

static bool storage_rtc_time_is_sane(const struct rtc_time *tm)
{
	if (tm == NULL) {
		return false;
	}

	if (tm->tm_sec < 0 || tm->tm_sec > 59) {
		return false;
	}
	if (tm->tm_min < 0 || tm->tm_min > 59) {
		return false;
	}
	if (tm->tm_hour < 0 || tm->tm_hour > 23) {
		return false;
	}
	if (tm->tm_mday < 1 || tm->tm_mday > 31) {
		return false;
	}
	if (tm->tm_mon < 0 || tm->tm_mon > 11) {
		return false;
	}

	int year = tm->tm_year + 1900;
	if (year < 700 || year > 2099) {
		return false;
	}

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

static int storage_ensure_pcm_parent_dirs(const struct rtc_time *tm)
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
 * Generate PCM filename with directory structure based on RTC timestamp
 * Path format: /SD:/YY-MM-DD/HH/MM/HH-MM-SS.pcm
 */
static void storage_make_pcm_filename_from_tm(const struct rtc_time *tm, char *out_path, size_t out_sz)
{
	if (tm == NULL) {
		/* Before RTC sync, store under a dedicated default folder. */
		unsigned long long ms = (unsigned long long)k_uptime_get();
		(void)snprintk(out_path, out_sz, "%s/pcm_%llu.pcm", storage_unsynced_dir, ms);
		return;
	}

	/* Format: YY-MM-DD/HH/MM/HH-MM-SS.pcm */
	int yy = (tm->tm_year + 1900) % 100;
	int mm = tm->tm_mon + 1;
	int dd = tm->tm_mday;
	int hh = tm->tm_hour;
	int min = tm->tm_min;
	int ss = tm->tm_sec;

	(void)snprintk(out_path, out_sz,
		       "/SD:/%02d-%02d-%02d/%02d/%02d/%02d-%02d-%02d.pcm",
		       yy, mm, dd, hh, min, hh, min, ss);
}

task_storage_status_t task_storage_pcm_begin(char *out_path, size_t out_sz)
{
	if (!app_state.sd_mounted) {
		return TASK_STORAGE_STATUS_NOT_MOUNTED;
	}
	if (out_path == NULL || out_sz == 0U) {
		return TASK_STORAGE_STATUS_INVALID_ARG;
	}

	uint8_t usage = 0U;
	uint64_t free_bytes = 0U;
	if (!storage_get_sd_capacity_with_free_bytes(&app_state.sd_total_mib,
						     &app_state.sd_free_mib,
						     &usage,
						     &free_bytes)) {
		app_state.sd_capacity_valid = false;
		return TASK_STORAGE_STATUS_CAPACITY_UNAVAILABLE;
	}
	app_state.sd_capacity_valid = true;
	app_state.sd_usage_percent = usage;

	if (free_bytes < storage_pcm_min_free_bytes) {
		return TASK_STORAGE_STATUS_NO_SPACE;
	}

	if (!pcm_file_inited) {
		fs_file_t_init(&pcm_file);
		pcm_file_inited = true;
	}
	if (pcm_file_opened) {
		fs_close(&pcm_file);
		pcm_file_opened = false;
	}

	struct rtc_time tm;
	struct rtc_time *tm_ptr = NULL;
	if (storage_get_rtc_time(&tm) && storage_rtc_time_is_sane(&tm)) {
		tm_ptr = &tm;
	} else {
		LOG_INF("RTC not ready or not synced yet, using default unsynced folder");
	}

	storage_make_pcm_filename_from_tm(tm_ptr, out_path, out_sz);
	LOG_INF("Generated PCM file path: %s", out_path);

	// build the parent directory tree based on timestamp, so that even if the file creation fails, we have the directories created for next attempts; if timestamp is not available, ensure the default unsynced directory exists
	if (tm_ptr != NULL) {
		int mkdir_ret = storage_ensure_pcm_parent_dirs(tm_ptr);
		if (mkdir_ret != 0) {
			LOG_WRN("Failed to prepare PCM directory tree for %s", out_path);
		}
	} else {
		(void)storage_mkdir_if_needed(storage_unsynced_dir);
	}

	int ret = fs_open(&pcm_file, out_path, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
	if (ret != 0) {
		if (ret == -ENODEV || ret == -EIO) {
			storage_note_sd_io_lost(ret);
		}
		return TASK_STORAGE_STATUS_IO_ERROR;
	}

	pcm_file_opened = true;
	pcm_write_seq = 0U;
	pcm_write_total_bytes = 0U;
	pcm_write_total_ms = 0U;
	pcm_write_max_ms = 0U;
	app_state.sd_pcm_write_enabled = true;
	app_state.sd_pcm_blocks = 0U;
	app_state.sd_pcm_total_bytes = 0U;
	app_state.sd_pcm_total_ms = 0U;
	app_state.sd_pcm_max_ms = 0U;
	app_state.sd_pcm_avg_ms = 0U;
	app_state.sd_pcm_last_block_ms = 0U;
	return TASK_STORAGE_STATUS_OK;
}

task_storage_status_t task_storage_pcm_write(const void *data, size_t len)
{
	if (!pcm_file_opened) {
		return TASK_STORAGE_STATUS_NOT_OPEN;
	}
	if (data == NULL || len == 0U) {
		return TASK_STORAGE_STATUS_INVALID_ARG;
	}

	const uint32_t seq = pcm_write_seq + 1U;
	LOG_DBG("PIPE_EVT,session=%u,evt=sd_write,phase=start,seq=%u,bytes=%u",
		(unsigned int)storage_session_id,
		(unsigned int)seq,
		(unsigned int)len);

	uint32_t t0 = k_uptime_get_32();
	int ret = fs_write(&pcm_file, data, len);
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
		if (ret == -ENODEV || ret == -EIO) {
			storage_note_sd_io_lost(ret);
		}
		return TASK_STORAGE_STATUS_IO_ERROR;
	}
	if ((size_t)ret != len) {
		LOG_INF("SD write short: seq=%u req=%uB wrote=%uB cost=%ums",
			(unsigned int)seq,
			(unsigned int)len,
			(unsigned int)ret,
			(unsigned int)cost_ms);
		return TASK_STORAGE_STATUS_NO_SPACE;
	}

	pcm_write_seq++;
	pcm_write_total_bytes += (uint64_t)len;
	pcm_write_total_ms += cost_ms;
	if (cost_ms > pcm_write_max_ms) {
		pcm_write_max_ms = cost_ms;
	}

	app_state.sdon = 1U;
	return TASK_STORAGE_STATUS_OK;
}

void task_storage_pcm_end(void)
{
	if (!pcm_file_opened) {
		return;
	}
	fs_close(&pcm_file);
	pcm_file_opened = false;
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

void task_sd_list_root(void)
{
	/* Execute only once */
	if (!app_state.sd_mounted || app_state.sd_listed_once) {
		return;
	}

	/* Avoid directory operations while recording to reduce FS contention */
	if (app_state.mic_recording) {
		return;
	}

	struct fs_dir_t dir;
	fs_dir_t_init(&dir);

	int ret = fs_opendir(&dir, "/SD:");
	if (ret != 0) {
		LOG_WRN("fs_opendir failed: %d", ret);
		return;
	}

	LOG_INF("SD root listing:");

	while (true) {
		struct fs_dirent ent;
		ret = fs_readdir(&dir, &ent);
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

	fs_closedir(&dir);
	app_state.sd_listed_once = true;
}

void task_status_persist(void)
{
	/* Only write if SD card is mounted */
	if (!app_state.sd_mounted) {
		return;
	}

	/* Skip while recording to avoid concurrent FATFS access */
	if (app_state.mic_recording) {
		return;
	}

	/* Status text logging is disabled; keep SD usage telemetry refreshed. */
	app_state.sd_usage_percent = storage_get_sd_usage_percent();
}

void task_storage_prepare_sleep(void)
{
	task_storage_pcm_end();

	if (!app_state.sd_mounted) {
		return;
	}

	int ret = fs_unmount(&fatfs_mnt);
	if (ret < 0) {
		LOG_WRN("SD unmount before sleep failed: %d", ret);
	} else {
		LOG_INF("SD unmounted before sleep");
	}

	app_state.sd_mounted = false;
	app_state.sd_listed_once = false;
	app_state.sd_capacity_valid = false;
	app_state.sd_total_mib = 0U;
	app_state.sd_free_mib = 0U;
}
