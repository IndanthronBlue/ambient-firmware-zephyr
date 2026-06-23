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

#define ADC3101_ACK_TIMEOUT_MS             50U
#define ADC3101_POST_SETUP_SETTLE_MS       100U
#define ADC3101_RECOVER_SETTLE_MS          200U
#define ADC3101_INA_IDLE_TIMEOUT_MS        1500U
#define MIC_LEADING_ZERO_ABORT_BLOCKS 5U

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
#define MIC_STARTUP_DISCARD_BLOCKS 8U
#define MIC_PIPELINE_WAIT_MS        3000
#define MIC_INFERENCE_WAIT_TIMEOUT_MS 10000U

/* RX memory slab for I2S driver (blocks owned by driver and passed to app) */
K_MEM_SLAB_DEFINE_STATIC(mic_rx_slab, MIC_CAPTURE_BLOCK_SIZE, MIC_BLOCK_COUNT, 4);


/* Internal state */
static bool mic_initialized = false;
static bool mic_sleep_prepared = false;
static bool mic_startup_discard_pending = false;


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
/* SD filesystem operation state to prevent closing or powering down too early. */
static atomic_t sd_write_inflight = ATOMIC_INIT(0);
static atomic_t sd_consumer_busy = ATOMIC_INIT(0);
static atomic_t sd_chunk_pending_blocks = ATOMIC_INIT(0);
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
	rx_cfg.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER |
			 I2S_OPT_BIT_CLK_CONT;
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

static void mic_stop_and_drain_rx(const char *reason)
{
	int stop_ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_STOP);

	if (stop_ret < 0) {
		LOG_WRN("STOP RX skipped during %s: %d",
			reason != NULL ? reason : "recover", stop_ret);
		return;
	}

	void *tail_block = NULL;
	size_t tail_sz = 0;
	int tail_ret = i2s_read(i2s_dev, &tail_block, &tail_sz);

	if ((tail_ret == 0) && (tail_block != NULL)) {
		k_mem_slab_free(&mic_rx_slab, tail_block);
		LOG_DBG("Drained RX tail block during %s: %u bytes",
			reason != NULL ? reason : "recover", (unsigned int)tail_sz);
	} else if (tail_ret == -EAGAIN) {
		LOG_DBG("No RX tail block during %s", reason != NULL ? reason : "recover");
	} else if (tail_ret < 0) {
		LOG_WRN("RX tail drain failed during %s: %d",
			reason != NULL ? reason : "recover", tail_ret);
	}
}

static int mic_full_rx_recover(bool expect_error_state)
{
	int ret;

	LOG_WRN("Mic RX recover: %s + drop + reconfigure + codec cold re-init",
		expect_error_state ? "prepare-from-error" : "stop");

	if (expect_error_state) {
		ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
		if (ret < 0) {
			LOG_WRN("I2S PREPARE from RX error failed: %d; continue with DROP", ret);
		}
	} else {
		mic_stop_and_drain_rx("full_recover");
	}
	(void)i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);

	ret = mic_configure_i2s_rx();
	if (ret < 0) {
		LOG_ERR("I2S RX reconfigure failed in recover: %d", ret);
		return ret;
	}

	ret = codec_adc3101_init();
	if (ret < 0) {
		LOG_ERR("Codec re-init failed in recover: %d", ret);
		return ret;
	}

	k_msleep(ADC3101_RECOVER_SETTLE_MS);

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

static bool mic_pipeline_stop_and_wait(void)
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
		uint32_t busy_sd;
		uint32_t chunk_pending;
		k_mutex_lock(&mic_ring_lock, K_FOREVER);
		pending_infer = mic_ring_write_seq - mic_ring_infer_seq;
		pending_sd = mic_ring_write_seq - mic_ring_sd_seq;
		k_mutex_unlock(&mic_ring_lock);
		inflight_sd = (uint32_t)atomic_get(&sd_write_inflight);
		busy_sd = (uint32_t)atomic_get(&sd_consumer_busy);
		chunk_pending = (uint32_t)atomic_get(&sd_chunk_pending_blocks);
		if (pending_infer == 0U && pending_sd == 0U && inflight_sd == 0U &&
		    busy_sd == 0U && chunk_pending == 0U) {
			return true;
		}
		task_watchdog_feed();
		k_msleep(5);
		elapsed_ms += 5U;
	}

	uint32_t pending_infer;
	uint32_t pending_sd;
	k_mutex_lock(&mic_ring_lock, K_FOREVER);
	pending_infer = mic_ring_write_seq - mic_ring_infer_seq;
	pending_sd = mic_ring_write_seq - mic_ring_sd_seq;
	k_mutex_unlock(&mic_ring_lock);
	LOG_WRN("Mic pipeline drain timeout: pending_infer=%u pending_sd=%u inflight_sd=%u busy_sd=%u chunk_pending=%u",
		(unsigned int)pending_infer,
		(unsigned int)pending_sd,
		(unsigned int)atomic_get(&sd_write_inflight),
		(unsigned int)atomic_get(&sd_consumer_busy),
		(unsigned int)atomic_get(&sd_chunk_pending_blocks));
	return false;
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
		atomic_set(&sd_consumer_busy, 1);
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
				atomic_set(&sd_chunk_pending_blocks, chunk_fill_blocks);
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
					chunk_fill_blocks = 0U;
					atomic_set(&sd_chunk_pending_blocks, 0);
					if (pcm_file_initialized) {
						task_storage_pcm_end_from_worker();
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
				if (task_storage_local_write_blocked()) {
					chunk_fill_blocks = 0U;
					atomic_set(&sd_chunk_pending_blocks, 0);
					continue;
				}

				/* 如果文件还没打开，说明是该次录音的第一块，先等待挂载并 Begin */
				if (!pcm_file_initialized) {
					while (atomic_get(&mic_pipeline_active) &&
					       !app_state.sd_mounted &&
					       !task_storage_local_write_blocked()) {
						k_msleep(100); // 等待后台挂载线程成功
					}

					if (task_storage_local_write_blocked()) {
						chunk_fill_blocks = 0U;
						atomic_set(&sd_chunk_pending_blocks, 0);
						continue;
					}

					if (app_state.sd_mounted) {
						atomic_inc(&sd_write_inflight);
						task_storage_status_t storage_status = task_storage_pcm_begin(path, sizeof(path));
						atomic_dec(&sd_write_inflight);
						if (storage_status == TASK_STORAGE_STATUS_OK) {
							pcm_file_initialized = true;
							LOG_INF("SD catch-up: background file opened at %s", path);
						} else {
							if (storage_status != TASK_STORAGE_STATUS_NO_SPACE) {
								LOG_WRN("SD catch-up: pcm_begin failed (%d), dropping blocks",
									(int)storage_status);
							}
							chunk_fill_blocks = 0U;
							atomic_set(&sd_chunk_pending_blocks, 0);
							continue;
						}
					} else {
						/* 录音已结束但SD仍未挂载，丢弃该块 */
						chunk_fill_blocks = 0U;
						atomic_set(&sd_chunk_pending_blocks, 0);
						continue;
					}
				}

				/* 执行写入 */
				atomic_inc(&sd_write_inflight);
				task_storage_status_t st = task_storage_pcm_write(mic_sd_chunk_buf,
									 (size_t)MIC_SD_BLOCKS_PER_CHUNK * MIC_APP_BLOCK_SIZE);
				atomic_dec(&sd_write_inflight);
				chunk_fill_blocks = 0U;
				atomic_set(&sd_chunk_pending_blocks, 0);
				if (st != TASK_STORAGE_STATUS_OK) {
					k_mutex_lock(&mic_ring_lock, K_FOREVER);
					mic_ring_sd_fail_count++;
					k_mutex_unlock(&mic_ring_lock);
				}
			}
		}
		atomic_set(&sd_consumer_busy, 0);
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
static int codec_i2c2_prepare(void)
{
	int ret;

	if (!device_is_ready(i2c2_dev)) {
		LOG_ERR("I2C2 not ready");
		return -ENODEV;
	}

	ret = i2c_configure(i2c2_dev, I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_STANDARD));
	if (ret < 0) {
		LOG_WRN("I2C2 reconfigure before codec init failed: %d", ret);
		return ret;
	}

	return 0;
}

int codec_adc3101_init(void)
{
	bool weak_verify = false;
	int ret;
	int setup_ret;

	/* Address is fixed by hardware strap on this board. */
	uint8_t adc_addr = ADC3101_ADDR00;

	task_ina3221_block_active(true);
	if (!task_ina3221_wait_idle(ADC3101_INA_IDLE_TIMEOUT_MS)) {
		LOG_WRN("INA3221 still busy before ADC3101 init; continuing with codec init");
	}

	ret = codec_i2c2_prepare();
	if (ret < 0) {
		goto out_unblock_ina;
	}
	adc3101_init(i2c2_dev, adc_addr);

	ret = adc3101_wait_ready(ADC3101_ACK_TIMEOUT_MS);
	if (ret < 0) {
		LOG_WRN("ADC3101 ACK check failed at 0x%02x: %d; audio blocks will decide",
			(unsigned int)adc_addr, ret);
	}

	setup_ret = adc3101_setup(&weak_verify);
	if (setup_ret < 0) {
		LOG_ERR("ADC3101 setup failed: %d", setup_ret);
		ret = setup_ret;
		goto out_unblock_ina;
	}

	if (weak_verify) {
		LOG_WRN("ADC3101 setup not fully verified; audio blocks will decide");
		ret = ADC3101_SETUP_WEAK_VERIFY;
		goto out_unblock_ina;
	}

	LOG_INF("ADC3101 configured at 0x%02x (I2S 16-bit, PRB_P1)",
		(unsigned int)adc_addr);
	ret = ADC3101_SETUP_OK;

out_unblock_ina:
	task_ina3221_block_active(false);
	return ret;
}

/* ===== Mic/I2S Init ===== */
static int task_microphone_init_locked(void)
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
				mic_startup_discard_pending = true;
				mic_sleep_prepared = false;
				k_msleep(ADC3101_POST_SETUP_SETTLE_MS);
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
	mic_startup_discard_pending = true;

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

	k_msleep(ADC3101_POST_SETUP_SETTLE_MS);

	return 0;
}

int task_microphone_init(void)
{
	int ret;

#if IS_ENABLED(CONFIG_PM)
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
#endif
	ret = task_microphone_init_locked();
#if IS_ENABLED(CONFIG_PM)
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
#endif

	return ret;
}

bool task_microphone_is_initialized(void)
{
	return mic_initialized && !mic_sleep_prepared;
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
	bool was_recording = app_state.mic_recording;

	mic_sleep_prepared = true;
	(void)mic_pipeline_stop_and_wait();
	app_state.mic_record_request = false;
	app_state.mic_recording = false;

	if (!mic_initialized) {
		return;
	}

	/* 若当前未在录音，不触发 STOP/DROP，避免无效状态扰动。 */
	if (was_recording) {
		mic_stop_and_drain_rx("prepare_sleep");
		(void)i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);
	}

	(void)adc3101_soft_powerdown();
}

/* ===== Perform single 3s capture to SD card ===== */
bool task_microphone_capture_once(void)
{
	int ret;
	bool resumed_from_sleep = mic_sleep_prepared;
	bool pm_locked = false;
	bool capture_complete = false;
	bool codec_zero_fault = false;
	const int blocks_target = 30;

	LOG_INF("task_microphone_capture_once called");
	app_state.mic_codec_zero_fault = false;
#if IS_ENABLED(CONFIG_PM)
	/* Prevent CPU from entering STOP (suspend-to-idle) during the entire
	 * codec/I2S bring-up and capture session. STOP mode gates the clocks
	 * used by SAI and can also interrupt the cold codec restart.
	 */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_locked = true;
#endif
	/* Preconditions */
	if (mic_sleep_prepared) {
		/* Restore the codec without reconfiguring I2S while the PM lock is held. */
		ret = task_microphone_init_locked();
		if (ret < 0) {
			LOG_ERR("Mic resume/init failed: %d", ret);
			goto out_pm_unlock;
		}
	}
	if (!mic_initialized) {
		LOG_WRN("Mic not initialized; cannot capture");
		goto out_pm_unlock;
	}

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
	task_status_led_event(STATUS_LED_CAPTURE_START);
	bool rx_started = false;
	bool rx_error = false;
	bool rx_driver_error = false;
	bool rx_restarted_once = false;
	bool rx_full_recovered_once = false;
	bool audio_seen = false;
	uint8_t startup_discard_blocks =
		mic_startup_discard_pending ? MIC_STARTUP_DISCARD_BLOCKS : 0U;
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
		LOG_INF("resumed_from_sleep, waiting %u ms for codec data path",
			(unsigned int)ADC3101_POST_SETUP_SETTLE_MS);
		k_msleep(ADC3101_POST_SETUP_SETTLE_MS);
	}

	/* Start RX stream */
	ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START);
	if (ret < 0) {
		LOG_WRN("i2s_trigger START RX failed: %d, trying full recover", ret);
		ret = mic_full_rx_recover(false);
		if (ret < 0) {
			LOG_ERR("Full recover before START failed: %d", ret);
			goto done_close;
		}
		/* mic_full_rx_recover() already reset the SAI queue/configuration
		 * and refreshed the codec.
		 */
		ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START);
		if (ret < 0) {
			LOG_ERR("i2s_trigger START RX failed after full recover: %d", ret);
			goto done_close;
		}
		rx_full_recovered_once = true;
	}
	rx_started = true;
	LOG_INF("I2S RX stream started");
	LOG_INF("PIPE_EVT,session=%u,evt=capture,phase=start",
		(unsigned int)capture_session_id);

	/* 3 seconds at 100ms per block -> 30 blocks */
	uint8_t leading_zero_blocks = 0U;

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
					mic_stop_and_drain_rx("timeout_restart");
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
					ret = mic_full_rx_recover(false);
					if (ret == 0) {
						ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START);
						if (ret == 0) {
							rx_full_recovered_once = true;
							rx_restarted_once = true;
							rx_timeouts = 0;
							leading_zero_blocks = 0U;
							continue;
						}
						LOG_ERR("START failed after full recover: %d", ret);
					} else {
						LOG_ERR("Full recover failed after timeout burst: %d", ret);
					}
				}

				LOG_ERR("No RX data after repeated timeouts, aborting");
				if (!audio_seen && (leading_zero_blocks > 0U)) {
					LOG_WRN("RX timed out after %u leading zero block(s); treat as codec zero fault",
						(unsigned int)leading_zero_blocks);
					codec_zero_fault = true;
				}
				rx_error = true;
				break;
			}
			continue;
		} else if (ret < 0) {
			LOG_ERR("i2s_buf_read failed: %d", ret);
			if (!rx_full_recovered_once) {
				LOG_WRN("RX read error, trying one-time full recover before declaring capture failed");
				ret = mic_full_rx_recover(true);
				if (ret == 0) {
					ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START);
					if (ret == 0) {
						rx_full_recovered_once = true;
						rx_restarted_once = true;
						rx_timeouts = 0;
						leading_zero_blocks = 0U;
						LOG_INF("I2S RX stream restarted after read error recovery");
						continue;
					}
					LOG_ERR("START failed after read-error recover: %d", ret);
				} else {
					LOG_ERR("Full recover failed after RX read error: %d", ret);
				}
			}
			if (!audio_seen && (leading_zero_blocks > 0U)) {
				LOG_WRN("RX failed after %u leading zero block(s); treat as codec zero fault",
					(unsigned int)leading_zero_blocks);
				codec_zero_fault = true;
			}
			rx_driver_error = true;
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
			if (!audio_seen) {
				leading_zero_blocks++;
				k_mem_slab_free(&mic_rx_slab, rx_mem_block);
				if (leading_zero_blocks >= MIC_LEADING_ZERO_ABORT_BLOCKS) {
					if (!rx_full_recovered_once) {
						LOG_WRN("RX leading zero blocks reached %u; trying one-time full recover",
							(unsigned int)leading_zero_blocks);
						ret = mic_full_rx_recover(false);
						if (ret == 0) {
							ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START);
							if (ret == 0) {
								rx_full_recovered_once = true;
								rx_restarted_once = true;
								rx_timeouts = 0;
								leading_zero_blocks = 0U;
								LOG_INF("I2S RX stream restarted after leading-zero recovery");
								continue;
							}
							LOG_ERR("START failed after leading-zero recover: %d", ret);
						} else {
							LOG_ERR("Full recover failed after leading-zero blocks: %d", ret);
						}
					}
					LOG_ERR("RX leading zero blocks reached %u; codec data path failed",
						(unsigned int)leading_zero_blocks);
					codec_zero_fault = true;
					rx_error = true;
					break;
				}
				continue;
			}
			} else {
				if (!audio_seen) {
					LOG_INF("RX audio probe OK: first non-zero block peak=%u after %u leading zero block(s)",
						(unsigned int)peak,
					(unsigned int)leading_zero_blocks);
				task_status_led_event(STATUS_LED_CODEC_RECORDING_OK);
			}
			audio_seen = true;
				LOG_DBG("RX block peak=%u", (unsigned)peak);
			}

			if (startup_discard_blocks > 0U) {
				startup_discard_blocks--;
				if (startup_discard_blocks == 0U) {
					mic_startup_discard_pending = false;
					LOG_INF("RX startup discard complete (%u block(s), ~%u ms)",
						(unsigned int)MIC_STARTUP_DISCARD_BLOCKS,
						(unsigned int)(MIC_STARTUP_DISCARD_BLOCKS * 100U));
				}
				k_mem_slab_free(&mic_rx_slab, rx_mem_block);
				continue;
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

	/* Graceful RX shutdown to avoid stale state across recording sessions.
	 * A codec zero fault is still a running RX stream and must be stopped
	 * and drained; otherwise STM32 SAI DROP forgets the active DMA block
	 * without returning it to the slab, and later sessions can fail with
	 * I2S_STATE_ERROR.
	 */
	if (rx_started) {
		if (rx_driver_error) {
			ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
			if (ret < 0) {
				LOG_WRN("I2S PREPARE during capture shutdown failed: %d", ret);
			}
		} else {
			mic_stop_and_drain_rx("capture_shutdown");
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
	app_state.mic_codec_zero_fault = codec_zero_fault;
	if (!capture_complete) {
		task_status_led_event(STATUS_LED_ERROR);
		LOG_WRN("Capture incomplete (%d/%d blocks), codec_zero_fault=%u, skip inference wait for session=%u",
			blocks_written,
			blocks_target,
			(unsigned int)(codec_zero_fault ? 1U : 0U),
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
out_pm_unlock:
	if (pm_locked) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
#else
out_pm_unlock:
	;
#endif
	return capture_complete;
}
