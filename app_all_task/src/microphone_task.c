/* Microphone capture using Zephyr stm32_sai driver (SAI1_B RX) */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <string.h>
#include <stdio.h>

LOG_MODULE_REGISTER(microphone_task, LOG_LEVEL_INF);

#include "ADC3101.h"
#include "tasks.h"

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
#define MIC_RING_BLOCK_COUNT    5
#define MIC_PIPELINE_WAIT_MS    3000

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
K_THREAD_STACK_DEFINE(mic_sd_thread_stack, 1536);
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
				if (!active && producer_done && chunk_fill_blocks > 0U) {
					size_t bytes = (size_t)chunk_fill_blocks * MIC_APP_BLOCK_SIZE;
					atomic_inc(&sd_write_inflight);
					task_storage_status_t st = task_storage_pcm_write(mic_sd_chunk_buf, bytes);
					atomic_dec(&sd_write_inflight);
					if (st != TASK_STORAGE_STATUS_OK &&
					    st != TASK_STORAGE_STATUS_NOT_OPEN &&
					    st != TASK_STORAGE_STATUS_NOT_MOUNTED) {
						k_mutex_lock(&mic_ring_lock, K_FOREVER);
						mic_ring_sd_fail_count++;
						k_mutex_unlock(&mic_ring_lock);
					}
					chunk_fill_blocks = 0U;
				}
				if (!active && producer_done) {
					break;
				}
				if (!active) {
					break;
				}
				break;
			}

			/* Once we have a full chunk, write it in one call. */
			if (chunk_fill_blocks >= MIC_SD_BLOCKS_PER_CHUNK) {
				atomic_inc(&sd_write_inflight);
				task_storage_status_t st = task_storage_pcm_write(mic_sd_chunk_buf,
									 (size_t)MIC_SD_BLOCKS_PER_CHUNK * MIC_APP_BLOCK_SIZE);
				atomic_dec(&sd_write_inflight);
				chunk_fill_blocks = 0U;
				if (st == TASK_STORAGE_STATUS_OK ||
				    st == TASK_STORAGE_STATUS_NOT_OPEN ||
				    st == TASK_STORAGE_STATUS_NOT_MOUNTED) {
					/* No-SD mode: keep pipeline moving, just skip writes. */
					continue;
				}
				{
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

	/* Recover I2C bus and allow codec/power rail settle time. */
	int r = i2c_recover_bus(i2c2_dev);
	if (r) {
		LOG_WRN("i2c_recover_bus failed before ADC3101 init: %d", r);
	}
	k_msleep(ADC3101_RESET_DELAY_MS);

	/* Address is fixed by hardware strap on this board. */
	const uint8_t adc_addr = ADC3101_ADDR00;
	adc3101_init(i2c2_dev, adc_addr);
	k_msleep(30);

	int ack = -EIO;
	for (int attempt = 1; attempt <= 30; attempt++) {
		ack = adc3101_read(0x00);
		if (ack >= 0) {
			break;
		}
		/* Do sparse recover to avoid shaking the bus repeatedly. */
		if (attempt == 6 || attempt == 16) {
			(void)i2c_recover_bus(i2c2_dev);
			k_msleep(10);
		}
		k_msleep((attempt < 10) ? 20 : 40);
	}
	if (ack < 0) {
		LOG_ERR("ADC3101 no ACK at fixed address 0x%02x", (unsigned int)adc_addr);
		return -ENODEV;
	}
	LOG_INF("ADC3101 ACK at 0x%02x (reg0=0x%02x)",
		(unsigned int)adc_addr, (unsigned int)ack);

	/* Perform setup and verify via a simple readback with limited retries */
	int rd = -1;
	for (int attempt = 1; attempt <= 3; attempt++) {
		adc3101_setup();
		/* Verify a known register (word length cfg at 0x1B) */
		rd = adc3101_read(0x1B);
		if (rd >= 0) {
			LOG_INF("ADC3101 setup verified: reg 0x1B = 0x%02x", rd);
			break;
		}
		LOG_WRN("ADC3101 verify attempt %d failed: %d", attempt, rd);
		(void)i2c_recover_bus(i2c2_dev);
		k_msleep(50);
	}
	if (rd < 0) {
		LOG_ERR("ADC3101 setup verification failed after retries");
		return -EIO;
	}
	LOG_INF("ADC3101 configured at 0x%02x (I2S 16-bit, PRB_P1)", (unsigned int)adc_addr);
	return 0;
}

/* ===== Mic/I2S Init ===== */
int task_microphone_init(void)
{
	if (mic_initialized) {
		mic_sleep_prepared = false;
		return 0;
	}

	if (!device_is_ready(i2s_dev)) {
		LOG_ERR("SAI1_B (I2S) device not ready");
		return -ENODEV;
	}

	int ret = codec_adc3101_init();
	if (ret < 0) {
		return ret;
	}

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

	ret = i2s_configure(i2s_dev, I2S_DIR_RX, &rx_cfg);
	k_msleep(1000);
	if (ret < 0) {
		LOG_ERR("i2s_configure RX failed: %d", ret);
		return ret;
	}

	LOG_INF("I2S RX configured: %u Hz, %u-bit, %u-ch(capture), capture block %u bytes, app block %u bytes",
			MIC_SAMPLE_FREQUENCY, MIC_SAMPLE_BIT_WIDTH, MIC_CAPTURE_CHANNELS,
			(unsigned)MIC_CAPTURE_BLOCK_SIZE, (unsigned)MIC_APP_BLOCK_SIZE);
	mic_initialized = true;
	mic_sleep_prepared = false;
	/* Keep V3.0 capture flow, but initialize inference pipeline. */
	(void)task_inference_init();
	mic_pipeline_workers_start_once();

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
	int ret;

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

	ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_STOP);
	if (ret < 0 && ret != -EIO) {
		LOG_WRN("RX STOP before sleep failed: %d", ret);
	}

	ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
	if (ret < 0 && ret != -EIO) {
		LOG_WRN("RX PREPARE before sleep failed: %d", ret);
	}

	ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);
	if (ret < 0 && ret != -EIO) {
		LOG_WRN("RX DROP before sleep failed: %d", ret);
	}
}

/* ===== Perform single 3s capture to SD card ===== */
void task_microphone_capture_once(void)
{
    LOG_INF("task_microphone_capture_once called");
	/* Preconditions */
	if (mic_sleep_prepared) {
		LOG_WRN("Mic capture blocked: sleep preparation active");
		return;
	}
	if (!mic_initialized) {
        LOG_WRN("Mic not initialized; cannot capture");
		return;
	}
	// if (!app_state.mic_record_request || app_state.mic_recording) {
    //     LOG_WRN("Recording already in progress or not requested");
	// 	return;
	// }
	static uint32_t capture_session_id = 0U;
	capture_session_id++;
	task_storage_set_session_id(capture_session_id);

	/* SD can be transiently lost during an active window; re-check before each capture. */
	task_sd_ensure_mounted();

	bool sd_write_enabled = false;
	char path[64] = {0};

	/* SD card is optional: capture + inference continues even if SD is absent. */
	if (app_state.sd_mounted) {
		/* Open timestamped PCM file via storage task wrapper. */
		task_storage_status_t storage_status = task_storage_pcm_begin(path, sizeof(path));
		if (storage_status == TASK_STORAGE_STATUS_OK) {
			sd_write_enabled = true;
			LOG_INF("Start recording to %s (3s)", path);
		} else {
			if (storage_status == TASK_STORAGE_STATUS_NO_SPACE) {
				LOG_WRN("SD write disabled: free space not enough");
			} else if (storage_status == TASK_STORAGE_STATUS_CAPACITY_UNAVAILABLE) {
				LOG_WRN("SD write disabled: cannot read SD capacity");
			} else if (storage_status == TASK_STORAGE_STATUS_NOT_MOUNTED) {
				LOG_WRN("SD write disabled: SD not mounted");
			} else {
				LOG_WRN("SD write disabled: storage begin failed (status=%d)", (int)storage_status);
			}
		}
	} else {
		LOG_WRN("SD not mounted; capture+inference will continue without SD write");
	}

	if (!sd_write_enabled) {
		LOG_INF("Start capture+inference (3s), SD write skipped");
	}
	app_state.mic_recording = true;
	bool rx_started = false;
	bool rx_error = false;
	mic_pipeline_reset_state();
	uint32_t capture_start_ms = k_uptime_get_32();
	task_inference_session_begin(capture_session_id, capture_start_ms);

	/* Start RX stream */
	int ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_START);
	if (ret < 0) {
		LOG_ERR("i2s_trigger START RX failed: %d", ret);
		goto done_close;
	}
	rx_started = true;
	LOG_INF("I2S RX stream started");
	LOG_INF("PIPE_EVT,session=%u,evt=capture,phase=start",
		(unsigned int)capture_session_id);

	/* 3 seconds at 100ms per block -> 30 blocks */
	const int blocks_target = 30;
	int blocks_written = 0;

	int rx_timeouts = 0;
	while (blocks_written < blocks_target) {
		void *rx_mem_block = NULL;
		size_t sz = 0;
		ret = i2s_read(i2s_dev, &rx_mem_block, &sz);
		if (ret == -EAGAIN) {
			rx_timeouts++;
			LOG_WRN("i2s RX timeout (no data), count=%d", rx_timeouts);
			if (rx_timeouts >= 10) {
				LOG_ERR("No RX data after repeated timeouts, aborting");
				break;
			}
			continue;
		} else if (ret < 0) {
			LOG_ERR("i2s_buf_read failed: %d", ret);
			rx_error = true;
			break;
		}
		LOG_DBG("i2s_buf_read got block of size %u bytes", (unsigned)sz);

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

		if (rx_error || stop_ret < 0) {
			int prep_ret = i2s_trigger(i2s_dev, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
			if (prep_ret < 0) {
				LOG_WRN("PREPARE RX failed: %d", prep_ret);
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

	/* Showcase timing: wait briefly for final MFCC+model series to finish. */
	bool infer_done = task_inference_wait_series_done(capture_session_id, 20000U);
	uint32_t capture_total_ms = k_uptime_get_32() - capture_start_ms;
	if (!infer_done) {
		LOG_WRN("Timing summary: inference series not finished within timeout (session=%u)",
			(unsigned int)capture_session_id);
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
	LOG_INF("SD write time: blocks=%u total=%uB total_cost=%ums max=%ums avg=%ums last_block=%ums (enabled_now=%u)",
		(unsigned int)app_state.sd_pcm_blocks,
		(unsigned int)app_state.sd_pcm_total_bytes,
		(unsigned int)app_state.sd_pcm_total_ms,
		(unsigned int)app_state.sd_pcm_max_ms,
		(unsigned int)app_state.sd_pcm_avg_ms,
		(unsigned int)app_state.sd_pcm_last_block_ms,
		(unsigned int)(app_state.sd_pcm_write_enabled ? 1U : 0U));
}
