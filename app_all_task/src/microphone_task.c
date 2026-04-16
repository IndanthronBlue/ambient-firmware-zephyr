/* Microphone capture using Zephyr stm32_sai driver (SAI1_B RX) */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/pm/policy.h>
#include <string.h>
#include <stdio.h>

LOG_MODULE_REGISTER(microphone_task, LOG_LEVEL_INF);

#include "ADC3101.h"
#include "tasks.h"

int codec_adc3101_init(void);

/* DT user node for custom properties (optional) */
#define ZEPHYR_USER_NODE DT_NODELABEL(zephyr_user)
#if DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, adc3101_reset_delay_ms)
#define ADC3101_RESET_DELAY_MS DT_PROP(ZEPHYR_USER_NODE, adc3101_reset_delay_ms)
#else
#define ADC3101_RESET_DELAY_MS 500
#endif

/* I2C2 for ADC3101 */
static const struct device *const i2c2_dev = DEVICE_DT_GET(DT_NODELABEL(i2c2));

/* SAI1 Block B node (Zephyr official stm32_sai driver) */
#define SAI1B_NODE DT_NODELABEL(sai1_b)
static const struct device *const i2s_dev = DEVICE_DT_GET(SAI1B_NODE);

/* ===== Audio stream configuration (16 kHz, 16-bit, stereo) ===== */
#define MIC_SAMPLE_FREQUENCY    16000
#define MIC_SAMPLE_BIT_WIDTH    16
#define MIC_CAPTURE_CHANNELS    2
#define MIC_APP_CHANNELS        1
#define MIC_BYTES_PER_SAMPLE    sizeof(int16_t)
/* 100ms block to simplify timing and buffering */
#define MIC_CAPTURE_SAMPLES_PER_BLOCK ((MIC_SAMPLE_FREQUENCY / 10) * MIC_CAPTURE_CHANNELS)
#define MIC_CAPTURE_BLOCK_SIZE  (MIC_BYTES_PER_SAMPLE * MIC_CAPTURE_SAMPLES_PER_BLOCK)
#define MIC_APP_SAMPLES_PER_BLOCK ((MIC_SAMPLE_FREQUENCY / 10) * MIC_APP_CHANNELS)
#define MIC_APP_BLOCK_SIZE      (MIC_BYTES_PER_SAMPLE * MIC_APP_SAMPLES_PER_BLOCK)
#define MIC_BLOCK_COUNT         6
#define MIC_IO_TIMEOUT_MS       200
#define MIC_RING_BLOCK_COUNT    30
#define MIC_PIPELINE_WAIT_MS        5000
#define MIC_INFERENCE_WAIT_TIMEOUT_MS 10000U

/* RX memory slab for I2S driver (blocks owned by driver and passed to app) */
K_MEM_SLAB_DEFINE_STATIC(mic_rx_slab, MIC_CAPTURE_BLOCK_SIZE, MIC_BLOCK_COUNT, 4);


/* Internal state */
static bool mic_initialized = false;
static bool mic_sleep_prepared = false;


/* Ring-RAM pipeline state: one producer (capture) and two consumers (inference/sd). */
static int16_t mic_ring_pcm[MIC_RING_BLOCK_COUNT][MIC_APP_SAMPLES_PER_BLOCK];
static int16_t mic_infer_buf[MIC_APP_SAMPLES_PER_BLOCK];
static struct k_mutex mic_ring_lock;
K_SEM_DEFINE(mic_infer_sem, 0, 1024);
K_SEM_DEFINE(mic_sd_sem, 0, 1024);
static struct k_thread mic_infer_thread_data;
static struct k_thread mic_sd_thread_data;
K_THREAD_STACK_DEFINE(mic_infer_thread_stack, 1024);
K_THREAD_STACK_DEFINE(mic_sd_thread_stack, 4096);
static bool mic_pipeline_workers_started = false;
static atomic_t mic_pipeline_active = ATOMIC_INIT(0);
static atomic_t mic_pipeline_producer_done = ATOMIC_INIT(0);
/* SD write in-flight counter to prevent closing file too early. */
static atomic_t sd_write_inflight = ATOMIC_INIT(0);
static uint32_t mic_ring_write_seq;
static uint32_t mic_ring_infer_seq;
static uint32_t mic_ring_sd_seq;
static uint32_t mic_ring_overrun_count;
static uint32_t mic_ring_infer_drop_count;
static uint32_t mic_ring_sd_drop_count;
static uint32_t mic_ring_sd_fail_count;

static inline uint32_t mic_min_u32(uint32_t a, uint32_t b)
{
	return (a < b) ? a : b;
}

static int mic_configure_i2s_rx(void)
{
	struct i2s_config rx_cfg = {0};
	rx_cfg.word_size = MIC_SAMPLE_BIT_WIDTH;
	rx_cfg.channels = MIC_CAPTURE_CHANNELS;
	rx_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	/* MCU provides BCLK/LRCK as master; bit clock continuous */
	rx_cfg.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_CONT;
	rx_cfg.frame_clk_freq = MIC_SAMPLE_FREQUENCY;
	rx_cfg.mem_slab = &mic_rx_slab;
	rx_cfg.block_size = MIC_CAPTURE_BLOCK_SIZE;
	rx_cfg.timeout = MIC_IO_TIMEOUT_MS;

	int ret = i2s_configure(i2s_dev, I2S_DIR_RX, &rx_cfg);
	if (ret < 0) {
		LOG_ERR("i2s_configure RX failed: %d", ret);
		return ret;
	}

	return 0;
}

static int mic_full_rx_recover(void)
{
	int ret;

	LOG_WRN("Mic RX full recover: stop/drop + codec/i2s reconfigure");

	/* Always attempt STOP then DROP regardless of current state;
	 * ignore errors since the stream may already be in error/ready state.
	 */
	(void)i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_STOP);
	(void)i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);

	/* Full codec re-init: re-programs all registers and waits for PLL lock.
	 * This is necessary because HAL_SAI_InitProtocol requires a clean HAL
	 * state, and the ADC3101 PLL needs ~50ms to stabilise before SAI DMA
	 * can be started successfully.
	 */
	ret = codec_adc3101_init();
	if (ret < 0) {
		LOG_ERR("Codec re-init failed in recover: %d", ret);
		return ret;
	}

	/* Re-configure the SAI/I2S peripheral after codec is ready. */
	ret = mic_configure_i2s_rx();
	if (ret < 0) {
		LOG_ERR("I2S reconfigure failed in recover: %d", ret);
		return ret;
	}

	/* Extra settling time: ADC3101 PLL needs time to lock to MCLK before
	 * the first DMA transfer can succeed. Without this delay
	 * HAL_SAI_Receive_DMA fails immediately (-EIO / -5).
	 */
	k_msleep(80);

	return 0;
}

static void mic_pipeline_reset_state(void)
{
	k_mutex_lock(&mic_ring_lock, K_FOREVER);
	mic_ring_write_seq = 0U;
	mic_ring_infer_seq = 0U;
	mic_ring_sd_seq = 0U;
	mic_ring_overrun_count = 0U;
	mic_ring_infer_drop_count = 0U;
	mic_ring_sd_drop_count = 0U;
	mic_ring_sd_fail_count = 0U;
	k_mutex_unlock(&mic_ring_lock);
	atomic_set(&mic_pipeline_producer_done, 0);
	atomic_set(&mic_pipeline_active, 1);

	while (k_sem_take(&mic_infer_sem, K_NO_WAIT) == 0) {
	}
	while (k_sem_take(&mic_sd_sem, K_NO_WAIT) == 0) {
	}
}

static void mic_pipeline_stop_and_wait(void)
{
	atomic_set(&mic_pipeline_active, 0);
	atomic_set(&mic_pipeline_producer_done, 1);
	k_sem_give(&mic_infer_sem);
	k_sem_give(&mic_sd_sem);

	uint32_t elapsed_ms = 0U;
	while (elapsed_ms < MIC_PIPELINE_WAIT_MS) {
		uint32_t pending_infer;
		uint32_t pending_sd;
		uint32_t inflight_sd;
		k_mutex_lock(&mic_ring_lock, K_FOREVER);
		pending_infer = mic_ring_write_seq - mic_ring_infer_seq;
		pending_sd = mic_ring_write_seq - mic_ring_sd_seq;
		k_mutex_unlock(&mic_ring_lock);
		inflight_sd = (uint32_t)atomic_get(&sd_write_inflight);
		if (pending_infer == 0U && pending_sd == 0U && inflight_sd == 0U) {
			break;
		}
		k_msleep(5);
		elapsed_ms += 5U;
	}
}

static void mic_inference_consumer_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (1) {
		k_sem_take(&mic_infer_sem, K_FOREVER);
		while (1) {
			bool active = atomic_get(&mic_pipeline_active) != 0;
			bool producer_done = atomic_get(&mic_pipeline_producer_done) != 0;
			bool has_data = false;
			uint32_t seq = 0U;
			k_mutex_lock(&mic_ring_lock, K_FOREVER);
			if (mic_ring_infer_seq < mic_ring_write_seq) {
				uint32_t backlog = mic_ring_write_seq - mic_ring_infer_seq;
				if (backlog > MIC_RING_BLOCK_COUNT) {
					uint32_t drop = backlog - MIC_RING_BLOCK_COUNT;
					mic_ring_infer_drop_count += drop;
					mic_ring_infer_seq += drop;
				}
				seq = mic_ring_infer_seq;
				uint32_t slot = seq % MIC_RING_BLOCK_COUNT;
				memcpy(mic_infer_buf, mic_ring_pcm[slot], MIC_APP_BLOCK_SIZE);
				mic_ring_infer_seq++;
				has_data = true;
			}
			k_mutex_unlock(&mic_ring_lock);

			if (!has_data) {
				if (!active && producer_done) {
					break;
				}
				if (!active) {
					break;
				}
				break;
			}

			ARG_UNUSED(seq);
			task_inference_process_block(mic_infer_buf, MIC_APP_BLOCK_SIZE);
		}
	}
}

static void mic_sd_consumer_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	/* Aggregate 100ms blocks into 0.2s chunks (2 blocks) to reduce RAM pressure while
	 * still avoiding per-block SD writes.
	 */
	enum { MIC_SD_BLOCKS_PER_CHUNK = 2 };
	static int16_t mic_sd_chunk_buf[MIC_SD_BLOCKS_PER_CHUNK * MIC_APP_SAMPLES_PER_BLOCK];
	uint32_t chunk_fill_blocks = 0U;
	bool pcm_file_initialized = false;
	char path[64] = {0};

	while (1) {
		k_sem_take(&mic_sd_sem, K_FOREVER);
		while (1) {
			bool active = atomic_get(&mic_pipeline_active) != 0;
			bool producer_done = atomic_get(&mic_pipeline_producer_done) != 0;
			bool has_data = false;
			k_mutex_lock(&mic_ring_lock, K_FOREVER);
			if (mic_ring_sd_seq < mic_ring_write_seq) {
				uint32_t backlog = mic_ring_write_seq - mic_ring_sd_seq;
				if (backlog > MIC_RING_BLOCK_COUNT) {
					uint32_t drop = backlog - MIC_RING_BLOCK_COUNT;
					mic_ring_sd_drop_count += drop;
					mic_ring_sd_seq += drop;
				}
				uint32_t slot = mic_ring_sd_seq % MIC_RING_BLOCK_COUNT;
				/* Copy into chunk buffer */
				uint32_t off = chunk_fill_blocks * MIC_APP_SAMPLES_PER_BLOCK;
				memcpy(&mic_sd_chunk_buf[off], mic_ring_pcm[slot], MIC_APP_BLOCK_SIZE);
				mic_ring_sd_seq++;
				chunk_fill_blocks++;
				has_data = true;
			}
			k_mutex_unlock(&mic_ring_lock);

			if (!has_data) {
				/* No more ring data: if producer finished, flush tail chunk. */
				if (!active && producer_done) {
					if (chunk_fill_blocks > 0U && pcm_file_initialized) {
						size_t bytes = (size_t)chunk_fill_blocks * MIC_APP_BLOCK_SIZE;
						atomic_inc(&sd_write_inflight);
						(void)task_storage_pcm_write(mic_sd_chunk_buf, bytes);
						atomic_dec(&sd_write_inflight);
					}
					pcm_file_initialized = false;
					break;
				}
				if (!active) {
					break;
				}
				break;
			}

			/* 一旦有全块，尝试写入。 */
			if (chunk_fill_blocks >= MIC_SD_BLOCKS_PER_CHUNK) {
				/* 如果文件还没打开，说明是该次录音的第一块，先等待挂载并 Begin */
				if (!pcm_file_initialized) {
					while (atomic_get(&mic_pipeline_active) && !app_state.sd_mounted) {
						k_msleep(100); // 等待后台挂载线程成功
					}

					if (app_state.sd_mounted) {
						task_storage_status_t storage_status = task_storage_pcm_begin(path, sizeof(path));
						if (storage_status == TASK_STORAGE_STATUS_OK) {
							pcm_file_initialized = true;
							LOG_INF("SD catch-up: background file opened at %s", path);
						} else {
							LOG_WRN("SD catch-up: pcm_begin failed (%d), dropping blocks", (int)storage_status);
							chunk_fill_blocks = 0U;
							continue;
						}
					} else {
						/* 录音已结束但SD仍未挂载，丢弃该块 */
						chunk_fill_blocks = 0U;
						continue;
					}
				}

				/* 执行写入 */
				atomic_inc(&sd_write_inflight);
				task_storage_status_t st = task_storage_pcm_write(mic_sd_chunk_buf,
									 (size_t)MIC_SD_BLOCKS_PER_CHUNK * MIC_APP_BLOCK_SIZE);
				atomic_dec(&sd_write_inflight);
				chunk_fill_blocks = 0U;
				if (st != TASK_STORAGE_STATUS_OK) {
					k_mutex_lock(&mic_ring_lock, K_FOREVER);
					mic_ring_sd_fail_count++;
					k_mutex_unlock(&mic_ring_lock);
				}
			}
		}
	}
}

static void mic_pipeline_workers_start_once(void)
{
	if (mic_pipeline_workers_started) {
		return;
	}

	k_mutex_init(&mic_ring_lock);
	k_tid_t infer_tid = k_thread_create(&mic_infer_thread_data,
					    mic_infer_thread_stack,
					    K_THREAD_STACK_SIZEOF(mic_infer_thread_stack),
					    mic_inference_consumer_thread,
					    NULL, NULL, NULL,
					    K_LOWEST_APPLICATION_THREAD_PRIO,
					    0, K_NO_WAIT);
	k_thread_name_set(infer_tid, "mic_infer_cons");

	k_tid_t sd_tid = k_thread_create(&mic_sd_thread_data,
					 mic_sd_thread_stack,
					 K_THREAD_STACK_SIZEOF(mic_sd_thread_stack),
					 mic_sd_consumer_thread,
					 NULL, NULL, NULL,
					 K_LOWEST_APPLICATION_THREAD_PRIO,
					 0, K_NO_WAIT);
	k_thread_name_set(sd_tid, "mic_sd_cons");

	mic_pipeline_workers_started = true;
}

/* ===== Codec Init (ADC3101) ===== */
int codec_adc3101_init(void)
{
	if (!device_is_ready(i2c2_dev)) {
		LOG_ERR("I2C2 not ready");
		return -ENODEV;
	}

	/* 移除重复的等待延迟，由电源使能端统一控制 */
	// k_msleep(ADC3101_RESET_DELAY_MS);

	/* Address is fixed by hardware strap on this board. */
	const uint8_t adc_addr = ADC3101_ADDR00;
	adc3101_init(i2c2_dev, adc_addr);
	k_msleep(80);

	// int ack = -EIO;
	// for (int attempt = 1; attempt <= 30; attempt++) {
	// 	ack = adc3101_read(0x00);
	// 	if (ack >= 0) {
	// 		break;
	// 	}
	// 	/* Do sparse recover to avoid shaking the bus repeatedly. */
	// 	if (attempt == 6 || attempt == 16) {
	// 		task_i2c2_lock();
	// 		(void)i2c_recover_bus(i2c2_dev);
	// 		task_i2c2_unlock();
	// 		k_msleep(10);
	// 	}
	// 	k_msleep((attempt < 10) ? 20 : 40);
	// }
	// if (ack < 0) {
	// 	LOG_ERR("ADC3101 no ACK at fixed address 0x%02x", (unsigned int)adc_addr);
	// 	return -ENODEV;
	// }
	// LOG_INF("ADC3101 ACK at 0x%02x (reg0=0x%02x)",
	// 	(unsigned int)adc_addr, (unsigned int)ack);

	// /* Perform setup and verify via a simple readback with limited retries */
	// int rd = -1;
	// for (int attempt = 1; attempt <= 3; attempt++) {
	// 	adc3101_setup();
	// 	/* Verify a known register (word length cfg at 0x1B) */
	// 	rd = adc3101_read(0x1B);
	// 	if (rd >= 0) {
	// 		LOG_INF("ADC3101 setup verified: reg 0x1B = 0x%02x", rd);
	// 		break;
	// 	}
	// 	LOG_WRN("ADC3101 verify attempt %d failed: %d", attempt, rd);
	// 	task_i2c2_lock();
	// 	(void)i2c_recover_bus(i2c2_dev);
	// 	task_i2c2_unlock();
	// 	k_msleep(50);
	// }
	// if (rd < 0) {
	// 	LOG_ERR("ADC3101 setup verification failed after retries");
	// 	return -EIO;
	// }
	task_i2c2_lock();
	adc3101_setup();
	task_i2c2_unlock();
	LOG_INF("ADC3101 configured at 0x%02x (I2S 16-bit, PRB_P1)", (unsigned int)adc_addr);
	return 0;
}

/* ===== Mic/I2S Init ===== */
int task_microphone_init(void)
{
	int ret;

	if (mic_initialized) {
		if (mic_sleep_prepared) {
			/* 从 SUSPEND 恢复：外设电源刚恢复，必须重写 Codec 寄存器 */
			LOG_INF("Mic resuming: re-initializing codec only");
			ret = codec_adc3101_init();
			if (ret < 0) {
				mic_initialized = false;
				return ret;
			}
			mic_sleep_prepared = false;
			k_msleep(80);
		}
		return 0;
	}

	if (!device_is_ready(i2s_dev)) {
		LOG_ERR("SAI1_B (I2S) device not ready");
		return -ENODEV;
	}

	ret = codec_adc3101_init();
	if (ret < 0) {
		return ret;
	}

	/* 仅在全局第一次初始化时配置 I2S 硬件寄存器 */
	ret = mic_configure_i2s_rx();
	if (ret < 0) {
		return ret;
	}

	LOG_INF("I2S RX configured: %u Hz, %u-bit, %u-ch(capture)",
			MIC_SAMPLE_FREQUENCY, MIC_SAMPLE_BIT_WIDTH, MIC_CAPTURE_CHANNELS);
	mic_initialized = true;
	mic_sleep_prepared = false;
	/* Keep V3.0 capture flow, but initialize inference pipeline. */
	(void)task_inference_init();
	mic_pipeline_workers_start_once();

	/* ADC3101 PLL requires time to lock to MCLK after power-up/init.
	 * Without this delay the very first HAL_SAI_Receive_DMA call fails
	 * (-EIO) because the codec has not yet started outputting a valid
	 * I2S clock on its SD line.
	 */
	k_msleep(80);

	return 0;
}

bool task_microphone_is_initialized(void)
{
	return mic_initialized;
}

/* ===== External trigger (e.g., button ISR) ===== */
void task_microphone_trigger_recording(void)
{
	if (mic_sleep_prepared) {
		return;
	}
	app_state.mic_record_request = true;
}

void task_microphone_prepare_sleep(void)
{
	mic_sleep_prepared = true;
	atomic_set(&mic_pipeline_active, 0);
	atomic_set(&mic_pipeline_producer_done, 1);
	k_sem_give(&mic_infer_sem);
	k_sem_give(&mic_sd_sem);
	app_state.mic_record_request = false;
	app_state.mic_recording = false;

	if (!mic_initialized) {
		return;
	}

	/* 停止 DMA 传输，保持 I2S 配置寄存器 */
	(void)i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_STOP);
	(void)i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);
}

/* ===== Perform single 3s capture to SD card ===== */
void task_microphone_capture_once(void)
{
	int ret;
	bool resumed_from_sleep = mic_sleep_prepared;
    LOG_INF("task_microphone_capture_once called");
	/* Preconditions */
	if (mic_sleep_prepared) {
		/* 调用 init 逻辑来恢复 Codec 而不重复配置 I2S */
		(void)task_microphone_init();
	}
	if (!mic_initialized) {
        LOG_WRN("Mic not initialized; cannot capture");
		return;
	}

#if IS_ENABLED(CONFIG_PM)
	/* Prevent CPU from entering STOP (suspend-to-idle) during the entire
	 * capture session. STM32 STOP mode gates PLL2 which feeds SAI1; if
	 * the CPU enters STOP while SAI DMA is running the DMA completion
	 * interrupt never fires and every i2s_read() times out.
	 */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
#endif
	// if (!app_state.mic_record_request || app_state.mic_recording) {
    //     LOG_WRN("Recording already in progress or not requested");
	// 	return;
	// }
	static uint32_t capture_session_id = 0U;
	capture_session_id++;
	task_storage_set_session_id(capture_session_id);

	/* SD card is optional: capture + inference continues even if SD is absent.
	 * The background consumer thread will wait for SD mount and call pcm_begin asynchronously.
	 */
	app_state.mic_recording = true;
	bool rx_started = false;
	bool rx_error = false;
	bool capture_complete = false;
	int blocks_written = 0;
	mic_pipeline_reset_state();
	uint32_t capture_start_ms = k_uptime_get_32();
	task_inference_session_begin(capture_session_id, capture_start_ms);

	/* Reset RX queue/state before every fresh session.
	 * Do not unconditionally PREPARE here: STM32 SAI reports invalid state
	 * in the normal READY path after suspend/resume, and that correlates with
	 * the first-session all-zero blocks. DROP is enough to bring the queue
	 * back to READY for a normal fresh START.
	 */
	ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);
	if (ret < 0 && ret != -EIO) {
		LOG_WRN("i2s_trigger DROP RX before START failed: %d", ret);
	}

	if (resumed_from_sleep) {
		/* Give codec/SAI data path a little extra settling time right after
		 * suspend resume to reduce first-session all-zero capture blocks.
		 */
		LOG_INF("resumed_from_sleep, sleeping for 100ms");
		k_msleep(600);
	}

	/* Start RX stream */
	ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START);
	if (ret < 0) {
		LOG_WRN("i2s_trigger START RX failed: %d, trying full recover", ret);
		ret = mic_full_rx_recover();
		if (ret < 0) {
			LOG_ERR("Full recover before START failed: %d", ret);
			goto done_close;
		}
		/* After full recover the SAI HAL state is clean; PREPARE+DROP
		 * are not needed again since mic_full_rx_recover() already
		 * called STOP+DROP and re-ran HAL_SAI_InitProtocol.
		 */
		ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START);
		if (ret < 0) {
			LOG_ERR("i2s_trigger START RX failed after full recover: %d", ret);
			goto done_close;
		}
	}
	rx_started = true;
	LOG_INF("I2S RX stream started");
	// adc3101_resume();
	LOG_INF("PIPE_EVT,session=%u,evt=capture,phase=start",
		(unsigned int)capture_session_id);

	/* 3 seconds at 100ms per block -> 30 blocks */
	const int blocks_target = 30;
	bool rx_restarted_once = false;
	bool rx_full_recovered_once = false;

	int rx_timeouts = 0;
	while (blocks_written < blocks_target) {
		void *rx_mem_block = NULL;
		size_t sz = 0;
		ret = i2s_read(i2s_dev, &rx_mem_block, &sz);
		if (ret == -EAGAIN) {
			rx_timeouts++;
			LOG_WRN("i2s RX timeout (no data), count=%d", rx_timeouts);
			if (rx_timeouts >= 10) {
				if (!rx_restarted_once) {
					LOG_WRN("RX timeout burst, trying one-time RX restart");
					(void)i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_STOP);
					(void)i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);
					k_msleep(10);
					ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START);
					if (ret == 0) {
						rx_restarted_once = true;
						rx_timeouts = 0;
						continue;
					}
					LOG_ERR("RX restart failed after timeout burst: %d", ret);
				}

				if (!rx_full_recovered_once) {
					LOG_WRN("RX timeout burst, trying one-time full recover");
					ret = mic_full_rx_recover();
					if (ret == 0) {
						ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START);
						if (ret == 0) {
							rx_full_recovered_once = true;
							rx_restarted_once = true;
							rx_timeouts = 0;
							continue;
						}
						LOG_ERR("START failed after full recover: %d", ret);
					} else {
						LOG_ERR("Full recover failed after timeout burst: %d", ret);
					}
				}

				LOG_ERR("No RX data after repeated timeouts, aborting");
				rx_error = true;
				break;
			}
			continue;
		} else if (ret < 0) {
			LOG_ERR("i2s_buf_read failed: %d", ret);
			rx_error = true;
			break;
		}
		LOG_DBG("i2s_buf_read got block of size %u bytes", (unsigned)sz);
		rx_timeouts = 0;

		if (sz == 0 || rx_mem_block == NULL) {
			if (rx_mem_block != NULL) {
				k_mem_slab_free(&mic_rx_slab, rx_mem_block);
			}
			k_msleep(5);
			continue;
		}

		/* Quick sanity check: report block peak to detect all-zero capture early. */
		const int16_t *s16 = (const int16_t *)rx_mem_block;
		size_t s16_count = sz / sizeof(int16_t);
		uint32_t peak = 0U;
		for (size_t i = 0; i < s16_count; i++) {
			int32_t v = s16[i];
			uint32_t a = (uint32_t)((v < 0) ? -v : v);
			if (a > peak) {
				peak = a;
			}
		}
		if (peak == 0U) {
			LOG_WRN("RX block peak is zero (size=%u bytes)", (unsigned)sz);
		} else {
			LOG_DBG("RX block peak=%u", (unsigned)peak);
		}

		k_mutex_lock(&mic_ring_lock, K_FOREVER);
		uint32_t slowest_seq = mic_min_u32(mic_ring_infer_seq, mic_ring_sd_seq);
		uint32_t used_blocks = mic_ring_write_seq - slowest_seq;
		if (used_blocks >= MIC_RING_BLOCK_COUNT) {
			mic_ring_overrun_count++;
		}
		uint32_t slot = mic_ring_write_seq % MIC_RING_BLOCK_COUNT;
		memset(mic_ring_pcm[slot], 0, MIC_APP_BLOCK_SIZE);
		const int16_t *stereo = (const int16_t *)rx_mem_block;
		size_t capture_samples = sz / sizeof(int16_t);
		size_t stereo_frames = capture_samples / MIC_CAPTURE_CHANNELS;
		size_t mono_samples = (stereo_frames > MIC_APP_SAMPLES_PER_BLOCK) ?
			MIC_APP_SAMPLES_PER_BLOCK : stereo_frames;
		for (size_t i = 0; i < mono_samples; i++) {
			mic_ring_pcm[slot][i] = stereo[i * MIC_CAPTURE_CHANNELS];
		}
		mic_ring_write_seq++;
		k_mutex_unlock(&mic_ring_lock);
		k_sem_give(&mic_infer_sem);
		k_sem_give(&mic_sd_sem);

		k_mem_slab_free(&mic_rx_slab, rx_mem_block);
		blocks_written++;
		LOG_DBG("Block %d captured (%u bytes)", blocks_written, (unsigned int)sz);

	}

	/* Graceful RX shutdown to avoid stale state across recording sessions. */
	if (rx_started) {
		int stop_ret = -EIO;

		if (!rx_error) {
			stop_ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_STOP);
			if (stop_ret == 0) {
				void *tail_block = NULL;
				size_t tail_sz = 0;
				int tail_ret = i2s_read(i2s_dev, &tail_block, &tail_sz);
				if (tail_ret == 0 && tail_block != NULL) {
					k_mem_slab_free(&mic_rx_slab, tail_block);
					LOG_DBG("Drained tail block: %u bytes", (unsigned)tail_sz);
				} else if (tail_ret == -EAGAIN) {
					LOG_DBG("No tail block after STOP");
				} else if (tail_ret < 0) {
					LOG_WRN("Tail drain failed after STOP: %d", tail_ret);
				}
			} else {
				LOG_WRN("STOP RX failed: %d", stop_ret);
			}
		}

		(void)i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);
	}

	LOG_INF("Recording done: %d blocks (~%d ms)",
			blocks_written, blocks_written * 100);
	LOG_INF("PIPE_EVT,session=%u,evt=capture,phase=end,blocks=%d,dur_ms=%u",
		(unsigned int)capture_session_id,
		blocks_written,
		(unsigned int)(k_uptime_get_32() - capture_start_ms));
	capture_complete = (!rx_error) && (blocks_written >= blocks_target);

done_close:
	if (atomic_get(&mic_pipeline_active) || !atomic_get(&mic_pipeline_producer_done)) {
		mic_pipeline_stop_and_wait();
	}
	k_mutex_lock(&mic_ring_lock, K_FOREVER);
	LOG_INF("Mic pipeline stats: overrun=%u infer_drop=%u sd_drop=%u sd_fail=%u",
		(unsigned int)mic_ring_overrun_count,
		(unsigned int)mic_ring_infer_drop_count,
		(unsigned int)mic_ring_sd_drop_count,
		(unsigned int)mic_ring_sd_fail_count);
	k_mutex_unlock(&mic_ring_lock);
	task_storage_pcm_end();
	app_state.mic_recording = false;
	app_state.mic_record_request = false;

	uint32_t capture_total_ms = k_uptime_get_32() - capture_start_ms;
	if (!capture_complete) {
		LOG_WRN("Capture incomplete (%d/%d blocks), skip inference wait for session=%u",
			blocks_written,
			blocks_target,
			(unsigned int)capture_session_id);
	} else {
		bool infer_done = false;
		uint32_t infer_wait_start_ms = k_uptime_get_32();

		while ((k_uptime_get_32() - infer_wait_start_ms) < MIC_INFERENCE_WAIT_TIMEOUT_MS) {
			if (task_inference_wait_series_done(capture_session_id, 1000U)) {
				infer_done = true;
				break;
			}
			task_watchdog_feed();
		}

		if (!infer_done) {
			LOG_WRN("Inference wait timeout for session=%u after %u ms",
				(unsigned int)capture_session_id,
				(unsigned int)MIC_INFERENCE_WAIT_TIMEOUT_MS);
		} else {
			LOG_INF("Pipeline summary: capture_total=%ums session=%u",
				(unsigned int)capture_total_ms, (unsigned int)capture_session_id);
			LOG_INF("MFCC chunk ms: [%u %u %u %u %u %u] det=%u cls=%u mfcc6+model=%u capture->done=%u",
				(unsigned int)app_state.infer_mfcc_chunk_ms[0],
				(unsigned int)app_state.infer_mfcc_chunk_ms[1],
				(unsigned int)app_state.infer_mfcc_chunk_ms[2],
				(unsigned int)app_state.infer_mfcc_chunk_ms[3],
				(unsigned int)app_state.infer_mfcc_chunk_ms[4],
				(unsigned int)app_state.infer_mfcc_chunk_ms[5],
				(unsigned int)app_state.infer_detector_ms,
				(unsigned int)app_state.infer_classifier_ms,
				(unsigned int)app_state.infer_mfcc_series_ms,
				(unsigned int)app_state.infer_capture_to_done_ms);
		}
	}
	LOG_INF("SD write time: blocks=%u total=%uB total_cost=%ums max=%ums avg=%ums last_block=%ums (enabled_now=%u)",
		(unsigned int)app_state.sd_pcm_blocks,
		(unsigned int)app_state.sd_pcm_total_bytes,
		(unsigned int)app_state.sd_pcm_total_ms,
		(unsigned int)app_state.sd_pcm_max_ms,
		(unsigned int)app_state.sd_pcm_avg_ms,
		(unsigned int)app_state.sd_pcm_last_block_ms,
		(unsigned int)(app_state.sd_pcm_write_enabled ? 1U : 0U));

#if IS_ENABLED(CONFIG_PM)
	/* Release the STOP-mode lock acquired at the start of capture.
	 * From this point the PM subsystem is free to enter STOP again.
	 */
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
#endif
}
