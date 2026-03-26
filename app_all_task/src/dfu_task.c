/*
 * SD-card based DFU task (MCUboot)
 * Read /SD:/update.bin and write to image-1 slot using Zephyr official APIs.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log_ctrl.h>
#include <errno.h>
#include <string.h>

#include "tasks.h"

LOG_MODULE_REGISTER(dfu_task, LOG_LEVEL_INF);

#define DFU_UPDATE_FILE_PATH "/SD:/update.bin"
#define DFU_DONE_FILE_PATH   "/SD:/update.done"
#define DFU_READ_CHUNK_SIZE  4096U
#define DFU_SLOT1_AREA_ID    FIXED_PARTITION_ID(slot1_partition)

static bool dfu_checked_once;
static uint8_t dfu_read_buf[DFU_READ_CHUNK_SIZE];

/**
 * @brief 获取升级文件大小并做基础类型检查。
 *
 * 该函数通过 `fs_stat()` 查询路径对应的目录项信息：
 * - 若路径不存在，返回底层错误码（如 `-ENOENT`）。
 * - 若路径是目录而不是普通文件，返回 `-EISDIR`。
 * - 若成功，输出文件大小到 `out_size`。
 *
 * @param path     升级文件路径（例如 `/SD:/update.bin`）。
 * @param out_size 输出参数，返回文件字节数。
 * @return 0 表示成功；负值表示失败（标准 errno）。
 */
static int dfu_get_update_file_size(const char *path, size_t *out_size)
{
	struct fs_dirent ent;
	int ret;

	if (path == NULL || out_size == NULL) {
		return -EINVAL;
	}

	ret = fs_stat(path, &ent);
	if (ret != 0) {
		return ret;
	}

	if (ent.type == FS_DIR_ENTRY_DIR) {
		return -EISDIR;
	}

	*out_size = (size_t)ent.size;
	return 0;
}

/**
 * @brief 擦除 MCUboot 的 Slot1 分区（image-1）。
 *
 * 流程：
 * 1. `flash_area_open()` 打开 `slot1_partition` 对应分区；
 * 2. `flash_area_erase()` 按分区全长执行擦除；
 * 3. `flash_area_close()` 关闭句柄。
 *
 * 擦除成功后，Slot1 处于可写入新镜像状态。
 *
 * @return 0 表示成功；负值表示失败。
 */
static int dfu_erase_image_1_slot(void)
{
	const struct flash_area *slot1;
	int ret = flash_area_open(DFU_SLOT1_AREA_ID, &slot1);
	if (ret != 0) {
		LOG_ERR("flash_area_open(image_1) failed: %d", ret);
		return ret;
	}

	ret = flash_area_erase(slot1, 0U, slot1->fa_size);
	if (ret != 0) {
		LOG_ERR("flash_area_erase(image_1) failed: %d", ret);
	}

	flash_area_close(slot1);
	return ret;
}

/**
 * @brief 将 SD 卡上的升级文件分块写入 Slot1。
 *
 * 核心步骤：
 * - 以只读方式打开升级文件；
 * - 先擦除 Slot1，确保写入区域干净；
 * - 初始化 `flash_img_context` 绑定到 Slot1；
 * - 循环执行 `fs_read()` + `flash_img_buffered_write()`；
 * - 循环中喂狗，避免长时间写入导致看门狗复位；
 * - 最后调用 `flash_img_buffered_write(..., flush=true)` 刷新尾块。
 *
 * 完成后会校验累计写入字节数与 `expected_size` 一致，
 * 以避免出现短写/漏写未被发现的情况。
 *
 * @param path          升级文件路径。
 * @param expected_size 期望写入总字节数（来自 `fs_stat`）。
 * @return 0 表示成功；负值表示失败。
 */
static int dfu_copy_file_to_image_1(const char *path, size_t expected_size)
{
	struct fs_file_t file;
	struct flash_img_context ctx;
	size_t total_written = 0U;
	int ret;

	fs_file_t_init(&file);
	ret = fs_open(&file, path, FS_O_READ);
	if (ret != 0) {
		LOG_ERR("fs_open(%s) failed: %d", path, ret);
		return ret;
	}

	ret = dfu_erase_image_1_slot();
	if (ret != 0) {
		(void)fs_close(&file);
		return ret;
	}

	ret = flash_img_init_id(&ctx, DFU_SLOT1_AREA_ID);
	if (ret != 0) {
		LOG_ERR("flash_img_init_id(image_1) failed: %d", ret);
		(void)fs_close(&file);
		return ret;
	}

	while (1) {
		ssize_t rd = fs_read(&file, dfu_read_buf, sizeof(dfu_read_buf));
		if (rd < 0) {
			LOG_ERR("fs_read update.bin failed: %d", (int)rd);
			ret = (int)rd;
			break;
		}
		if (rd == 0) {
			ret = 0;
			break;
		}

		ret = flash_img_buffered_write(&ctx, dfu_read_buf, (size_t)rd, false);
		if (ret != 0) {
			LOG_ERR("flash_img_buffered_write failed: %d", ret);
			break;
		}

		total_written += (size_t)rd;
		task_watchdog_feed();
	}

	if (ret == 0) {
		ret = flash_img_buffered_write(&ctx, NULL, 0U, true);
		if (ret != 0) {
			LOG_ERR("flash_img flush failed: %d", ret);
		}
	}

	(void)fs_close(&file);

	if (ret != 0) {
		return ret;
	}

	if (total_written != expected_size) {
		LOG_ERR("Size mismatch after copy: expected=%u actual=%u",
			(unsigned int)expected_size,
			(unsigned int)total_written);
		return -EIO;
	}

	LOG_INF("DFU image copied to image-1: %u bytes", (unsigned int)total_written);
	return 0;
}

/**
 * @brief 升级文件消费后的清理动作。
 *
 * 优先将 `/SD:/update.bin` 重命名为 `/SD:/update.done`，
 * 便于保留“已使用”的痕迹，避免下次启动重复升级。
 * 若重命名失败，则退化为删除 `update.bin`。
 */
static void dfu_mark_update_file_consumed(void)
{
	int ret = fs_rename(DFU_UPDATE_FILE_PATH, DFU_DONE_FILE_PATH);
	if (ret == 0) {
		LOG_INF("Renamed update image to %s", DFU_DONE_FILE_PATH);
		return;
	}

	ret = fs_unlink(DFU_UPDATE_FILE_PATH);
	if (ret == 0) {
		LOG_INF("Removed consumed update image");
	} else {
		LOG_WRN("Failed to rename/unlink consumed image: %d", ret);
	}
}

/**
 * @brief 启动阶段执行一次 SD 卡离线升级检查与触发。
 *
 * 设计目标：
 * - 仅在单次启动周期内执行一次（`dfu_checked_once`）；
 * - 只依赖 Zephyr 官方 FS/Flash/MCUboot API；
 * - 在检测到有效升级文件后，自动写入 Slot1 并请求 MCUboot 升级。
 *
 * 执行流程：
 * 1. 确保 SD 已挂载；未挂载则直接跳过。
 * 2. 检查 `/SD:/update.bin` 是否存在且大小有效。
 * 3. 打开 Slot1 并校验文件大小不超过分区容量。
 * 4. 调用写入函数将镜像分块写入 Slot1。
 * 5. 调用 `boot_request_upgrade(BOOT_UPGRADE_TEST)` 请求测试升级。
 * 6. 清理升级文件（重命名/删除），随后冷重启。
 *
 * 说明：
 * - 该函数不做镜像签名验证，最终验签由 MCUboot 在启动阶段完成。
 * - 若任一步失败，仅记录日志并返回，不触发重启。
 */
void task_dfu_check_and_apply(void)
{
	const struct flash_area *slot1;
	size_t update_size = 0U;
	int ret;

	if (dfu_checked_once) {
		return;
	}
	dfu_checked_once = true;

	task_sd_ensure_mounted();
	if (!app_state.sd_mounted) {
		LOG_INF("DFU skipped: SD card not mounted");
		return;
	}

	ret = dfu_get_update_file_size(DFU_UPDATE_FILE_PATH, &update_size);
	if (ret == -ENOENT) {
		LOG_INF("DFU: no %s in SD root", DFU_UPDATE_FILE_PATH);
		return;
	}
	if (ret != 0) {
		LOG_WRN("DFU: cannot stat update image (%d)", ret);
		return;
	}
	if (update_size == 0U) {
		LOG_WRN("DFU: update image is empty");
		return;
	}

	ret = flash_area_open(DFU_SLOT1_AREA_ID, &slot1);
	if (ret != 0) {
		LOG_ERR("DFU: flash_area_open(image_1) failed: %d", ret);
		return;
	}

	if (update_size > slot1->fa_size) {
		LOG_ERR("DFU: update too large (%u > slot1 %u)",
			(unsigned int)update_size,
			(unsigned int)slot1->fa_size);
		flash_area_close(slot1);
		return;
	}

	flash_area_close(slot1);

	LOG_INF("DFU: found update image %u bytes, writing to image-1",
		(unsigned int)update_size);

	ret = dfu_copy_file_to_image_1(DFU_UPDATE_FILE_PATH, update_size);
	if (ret != 0) {
		LOG_ERR("DFU: image write failed (%d)", ret);
		return;
	}

	// ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
    ret = boot_request_upgrade(BOOT_UPGRADE_PERMANENT);
	if (ret != 0) {
		LOG_ERR("DFU: boot_request_upgrade failed: %d", ret);
		return;
	}

	dfu_mark_update_file_consumed();
	LOG_INF("DFU: upgrade requested (PERMANENT), rebooting now");
	LOG_PANIC();
	sys_reboot(SYS_REBOOT_COLD);
}
