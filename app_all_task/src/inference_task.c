/* Zephyr-adapted inference pipeline (MFCC + CNN) based on ambient firmware */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <math.h>

LOG_MODULE_REGISTER(inference_task, LOG_LEVEL_INF);

#include "tasks.h"

/* DSP & model includes from Zephyr's CMSIS-DSP module */
#include <arm_math.h>
#include <dsp/transform_functions.h>
#include <dsp/basic_math_functions.h>
#include <dsp/fast_math_functions.h>
#include <dsp/complex_math_functions.h>
#include <dsp/matrix_functions.h>
#include <dsp/statistics_functions.h>

#include "tab_filt.h" /* dctMatrixFilters, filtPos, filtLen, packedFilters, window */
#include "app_inference_labels.h"
#include "qualia_model_wrappers.h"

/* ===== Pipeline configuration ===== */
#define NB_MEL_FILTERS         10
#define NB_DCT_MFCC            10
#define MFCC_MODEL_COEFF_OFFSET 0
#define N_FFT                  256
#define HOP_SIZE               256
#define MODEL_SAMPLE_COUNT     (BIRD_MODEL_INPUT_DIM_0 * HOP_SIZE)
#define INFERENCE_CHUNKS       6
#define BASE_ROWS_PER_CHUNK    (BIRD_MODEL_INPUT_DIM_0 / INFERENCE_CHUNKS)
#define EXTRA_ROWS_PER_SERIES  (BIRD_MODEL_INPUT_DIM_0 % INFERENCE_CHUNKS)
#define MAX_ROWS_PER_CHUNK     (BASE_ROWS_PER_CHUNK + ((EXTRA_ROWS_PER_SERIES > 0) ? 1 : 0))
#define CHUNK_MAX_SAMPLES      (MAX_ROWS_PER_CHUNK * HOP_SIZE)
#define NB_SAMPLES             (2 * CHUNK_MAX_SAMPLES)
#define TMP_LEN                (N_FFT + 2)

_Static_assert((BIRD_MODEL_INPUT_DIM_1 + MFCC_MODEL_COEFF_OFFSET) <= NB_DCT_MFCC,
               "MFCC output must include all model input coefficients");
BUILD_ASSERT(NB_SAMPLES >= (2 * CHUNK_MAX_SAMPLES),
	     "Inference PCM ring must hold at least two MFCC chunks");

#if defined(CONFIG_BOARD_STM32L496G_BIRD)
#define INFERENCE_AUDIO_DATA_ATTR Z_GENERIC_SECTION(SRAM1) __aligned(4)
#else
#define INFERENCE_AUDIO_DATA_ATTR
#endif

/* ===== Internal buffers and state ===== */
static int16_t audio_data[NB_SAMPLES] INFERENCE_AUDIO_DATA_ATTR;
static volatile size_t write_point = 0;
static volatile size_t read_point = 0;
static volatile bool ready_for_inference = false;
static volatile size_t cnt_mfcc = 0; /* Accumulate 186 frames for Qualia 1D CNN input */
static size_t samples_since_window = 0; /* Producer counter to decide window readiness */

/* MFCC working buffers */
static float32_t input_mfcc[N_FFT];
static float32_t output_mfcc[NB_DCT_MFCC];
static float32_t tmp[TMP_LEN];
static arm_mfcc_instance_f32 instMFCC;

/* CNN inputs and outputs */
static bird_model_input_t inputs[BIRD_MODEL_INPUT_DIM_0][BIRD_MODEL_INPUT_DIM_1];
static bird_model_output_t outputs_detects[BIRD_DETECT_OUTPUT_SAMPLES];
static bird_model_output_t outputs_class[EZBIRD_OUTPUT_SAMPLES];
static uint8_t selected_audio_channel = 0U; /* Prefer left channel (ADC3101 mono path) */

BUILD_ASSERT(APP_INFERENCE_LABEL_CNT == EZBIRD_OUTPUT_SAMPLES,
	     "APP_INFERENCE_LABEL_CNT must match classifier output samples");
BUILD_ASSERT(APP_INFERENCE_LABELS_GENERATED_COUNT == APP_INFERENCE_LABEL_CNT,
	     "generated label count must match APP_INFERENCE_LABEL_CNT");

/* ===== Public API ===== */
/* Background work: copy one MFCC chunk and process it into the full Qualia input. */
static int16_t window_buf[CHUNK_MAX_SAMPLES];
static struct k_work mfcc_work;
static volatile bool work_in_progress = false;
static volatile bool infer_sleep_block = false;
/* Dedicated workqueue with lower priority than capture thread */
static struct k_work_q infer_wq;
#define INFER_WQ_STACK_SIZE 2048
K_THREAD_STACK_DEFINE(infer_wq_stack, INFER_WQ_STACK_SIZE);
#define INFER_WQ_PRIORITY 7 /* Lower priority (numerically higher) to avoid starving I2S reading */
/* Timing metrics */
static uint32_t series_start_ms = 0;   /* Start time of the 6-window series */
static uint32_t chunk_start_ms  = 0;   /* Start time of current MFCC chunk */
static uint32_t infer_session_id = 0U;
static uint32_t infer_capture_start_ms = 0U;
static volatile uint32_t infer_done_session_id = 0U;

static void infer_reset_uplink_window_state(void)
{
	memset(app_state.infer_label_counts_window, 0,
	       sizeof(app_state.infer_label_counts_window));
	app_state.infer_window_count = 0U;
	app_state.infer_window_ready = false;
}

static void infer_reset_periodic_rms_stats(void)
{
	app_state.infer_avg_rms = 0.0f;
}

static void infer_record_uplink_bucket(int label)
{
	size_t bucket;

	if ((label >= 0) && (label < APP_INFERENCE_LABEL_CNT)) {
		bucket = (size_t)label;
	} else if (label == -2) {
		bucket = APP_INFERENCE_LOW_CONF_BIRD_BUCKET;
	} else {
		return;
	}

	if (app_state.infer_label_counts_window[bucket] < UINT16_MAX) {
		app_state.infer_label_counts_window[bucket]++;
	}
}

static size_t mfcc_chunk_rows(size_t chunk_idx)
{
	return BASE_ROWS_PER_CHUNK + ((chunk_idx < EXTRA_ROWS_PER_SERIES) ? 1U : 0U);
}

static size_t mfcc_chunk_row_start(size_t chunk_idx)
{
	size_t extra_before = (chunk_idx < EXTRA_ROWS_PER_SERIES) ? chunk_idx : EXTRA_ROWS_PER_SERIES;
	return (chunk_idx * BASE_ROWS_PER_CHUNK) + extra_before;
}

static size_t mfcc_chunk_samples(size_t chunk_idx)
{
	return mfcc_chunk_rows(chunk_idx) * HOP_SIZE;
}

static float model_output_at(const bird_model_output_t *output, size_t index)
{
    return bird_model_output_to_float(output[index]);
}

static float softmax_model_output_probability(const bird_model_output_t *logits,
                                              size_t count,
                                              size_t index)
{
    float max_logit = model_output_at(logits, 0);

    for (size_t i = 1; i < count; i++) {
        float logit = model_output_at(logits, i);
        if (logit > max_logit) {
            max_logit = logit;
        }
    }

    float denom = 0.0f;
    float selected = 0.0f;
    for (size_t i = 0; i < count; i++) {
        float value = expf(model_output_at(logits, i) - max_logit);
        denom += value;
        if (i == index) {
            selected = value;
        }
    }

    return selected / denom;
}

static void mfcc_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    if (infer_sleep_block) {
        work_in_progress = false;
        return;
    }
    LOG_DBG("mfcc_work: start (cnt_mfcc=%u)", (unsigned int)cnt_mfcc);
    const uint32_t chunk_idx = (uint32_t)cnt_mfcc;
    chunk_start_ms = k_uptime_get_32();
    if (cnt_mfcc == 0) {
        series_start_ms = chunk_start_ms;
    }
    LOG_DBG("PIPE_EVT,session=%u,evt=mfcc_chunk,phase=start,idx=%u",
            (unsigned int)infer_session_id, (unsigned int)(chunk_idx + 1U));
    /* Process the last copied window_buf into its slice of the full 186x10 input. */
    size_t row_start = mfcc_chunk_row_start(cnt_mfcc);
    size_t rows_this_chunk = mfcc_chunk_rows(cnt_mfcc);
    size_t samples_this_chunk = mfcc_chunk_samples(cnt_mfcc);
    size_t row_end = row_start + rows_this_chunk;
    LOG_DBG("mfcc_work: rows %u..%u (rows_this_chunk=%u)",
            (unsigned int)row_start,
            (unsigned int)(row_end - 1),
            (unsigned int)rows_this_chunk);

    /* Compute RMS of current MFCC window. */
    float rms = 0.0f;
    int16_t peak_abs = 0;
    for (size_t i = 0; i < samples_this_chunk; i++) {
        float sig = (float)window_buf[i];
        rms += (sig * sig) / 32768.0f;
        int16_t abs_v = (window_buf[i] < 0) ? (int16_t)(-window_buf[i]) : window_buf[i];
        if (abs_v > peak_abs) {
            peak_abs = abs_v;
        }
    }
	rms = rms / (float)samples_this_chunk;
	rms = sqrtf(rms);
	app_state.infer_last_rms = rms;
	uint32_t rms_index = (app_state.infer_window_count * (uint32_t)INFERENCE_CHUNKS) +
			     (uint32_t)cnt_mfcc;
	app_state.infer_avg_rms =
	    ((app_state.infer_avg_rms * (float)rms_index) + rms) /
	    (float)(rms_index + 1U);
	LOG_INF("Audio RMS: %.6f, avg RMS: %.6f, avg_idx=%u, peak: %d, ch: %u",
	        (double)rms,
	        (double)app_state.infer_avg_rms,
	        (unsigned int)rms_index,
	        (int)peak_abs,
	        (unsigned int)selected_audio_channel);

    for (size_t row = row_start; row < row_end; row++) {
        int start = (int)((row - row_start) * HOP_SIZE);
        for (size_t i = 0; i < N_FFT; i++) {
            size_t idx = start + i; /* within window_buf range */
            /* Full-MFCC models keep C0, so preserve the raw int16 scale used in training. */
            input_mfcc[i] = (float32_t)window_buf[idx];
        }

        arm_mfcc_f32(&instMFCC, input_mfcc, output_mfcc, tmp);

        for (size_t col = 0; col < BIRD_MODEL_INPUT_DIM_1; col++) {
            inputs[row][col] =
                bird_model_quantize_input(output_mfcc[col + MFCC_MODEL_COEFF_OFFSET]);
        }

        if ((row == row_start) || (row == row_end - 1)) {
            LOG_DBG("mfcc_work: row %u computed", (unsigned int)row);
        }
        /* Yield periodically to avoid starving higher-priority threads */
        if (((row - row_start) & 0x07) == 0) {
            k_yield();
        }
    }

    uint32_t chunk_ms = k_uptime_get_32() - chunk_start_ms;
    if (chunk_idx < 6U) {
        app_state.infer_mfcc_chunk_ms[chunk_idx] = (uint16_t)chunk_ms;
    }
    LOG_DBG("mfcc_work: chunk time = %u ms", (unsigned int)chunk_ms);
    LOG_DBG("PIPE_EVT,session=%u,evt=mfcc_chunk,phase=end,idx=%u,dur_ms=%u",
            (unsigned int)infer_session_id, (unsigned int)(chunk_idx + 1U), (unsigned int)chunk_ms);

    cnt_mfcc++;
    LOG_DBG("mfcc_work: chunk complete, cnt_mfcc now %u", (unsigned int)cnt_mfcc);

    if (cnt_mfcc == INFERENCE_CHUNKS) {
        LOG_DBG("mfcc_work: accumulated %u chunk(s), running detector", (unsigned int)INFERENCE_CHUNKS);
        uint32_t det_start_ms = k_uptime_get_32();
        LOG_DBG("PIPE_EVT,session=%u,evt=detector,phase=start",
                (unsigned int)infer_session_id);
        bird_detect_model_run(inputs, outputs_detects);
        uint32_t det_ms = k_uptime_get_32() - det_start_ms;
        LOG_DBG("PIPE_EVT,session=%u,evt=detector,phase=end,dur_ms=%u",
                (unsigned int)infer_session_id, (unsigned int)det_ms);
        float bird_score = model_output_at(outputs_detects, 0);    /* EzBirdSmall labels.json: 0=bird */
        float no_bird_score = model_output_at(outputs_detects, 1); /* EzBirdSmall labels.json: 1=no_bird */
        float bird_prob = softmax_model_output_probability(outputs_detects, BIRD_DETECT_OUTPUT_SAMPLES, 0);
        bool bird_detected = bird_score > no_bird_score;

        LOG_INF("Detector logits: bird=%.3f, no_bird=%.3f, p_bird=%.3f",
                (double)bird_score, (double)no_bird_score, (double)bird_prob);
        LOG_INF("Detector time: %u ms", (unsigned int)det_ms);

        uint32_t cls_ms_u32 = 0U;
        int label = -1;
        if (bird_detected) {
            if (bird_prob >= APP_INFERENCE_DETECT_CONF_THRESH) {
                LOG_DBG("mfcc_work: detector gated true, running classifier");
                uint32_t cls_start_ms = k_uptime_get_32();
                LOG_DBG("PIPE_EVT,session=%u,evt=classifier,phase=start",
                        (unsigned int)infer_session_id);
                ezbird_model_run(inputs, outputs_class);
                cls_ms_u32 = k_uptime_get_32() - cls_start_ms;
                LOG_DBG("PIPE_EVT,session=%u,evt=classifier,phase=end,dur_ms=%u",
                        (unsigned int)infer_session_id, (unsigned int)cls_ms_u32);
                int best = 0;
                float best_val = model_output_at(outputs_class, 0);
                for (int i = 1; i < EZBIRD_OUTPUT_SAMPLES; i++) {
                    float value = model_output_at(outputs_class, (size_t)i);
                    if (value > best_val) {
                        best_val = value;
                        best = i;
                    }
                }
                label = best;
                const char *label_name = kBirdLabels[best];
                float best_prob = softmax_model_output_probability(outputs_class,
                                                                  EZBIRD_OUTPUT_SAMPLES,
                                                                  (size_t)best);
                LOG_INF("Bird class: %s (logit=%.3f, prob=%.3f)",
                        label_name, (double)best_val, (double)best_prob);
                LOG_INF("Classifier time: %u ms", (unsigned int)cls_ms_u32);
                /* Allow other threads to run after heavy classifier */
                k_yield();
            } else {
                label = -2;
                LOG_INF("Detector positive but below confidence threshold %.2f",
                        (double)APP_INFERENCE_DETECT_CONF_THRESH);
            }
        } else {
            label = -1;
            LOG_INF("No bird detected");
        }
        app_state.infer_last_label = label;
	        app_state.infer_total_count++;
	        app_state.infer_window_count++;
	        infer_record_uplink_bucket(label);
			task_status_led_event(STATUS_LED_INFER_DONE);
        LOG_INF("Inference state updated: label=%d window_total=%u total=%u",
                label,
                (unsigned int)app_state.infer_window_count,
                (unsigned int)app_state.infer_total_count);

        uint32_t series_ms = k_uptime_get_32() - series_start_ms;
        uint32_t capture_to_done_ms = 0U;
        if (infer_capture_start_ms != 0U) {
            capture_to_done_ms = k_uptime_get_32() - infer_capture_start_ms;
        }
        app_state.infer_mfcc_series_ms = series_ms;
        app_state.infer_detector_ms = (uint16_t)det_ms;
        app_state.infer_classifier_ms = (uint16_t)cls_ms_u32;
        app_state.infer_capture_to_done_ms = capture_to_done_ms;
        app_state.infer_session_id_done = infer_session_id;
        infer_done_session_id = infer_session_id;

        LOG_INF("MFCC chunk ms: [%u %u %u %u %u %u]",
                (unsigned int)app_state.infer_mfcc_chunk_ms[0],
                (unsigned int)app_state.infer_mfcc_chunk_ms[1],
                (unsigned int)app_state.infer_mfcc_chunk_ms[2],
                (unsigned int)app_state.infer_mfcc_chunk_ms[3],
                (unsigned int)app_state.infer_mfcc_chunk_ms[4],
                (unsigned int)app_state.infer_mfcc_chunk_ms[5]);
        LOG_INF("Timing: det=%u ms cls=%u ms mfcc6+model=%u ms capture->done=%u ms (session=%u)",
                (unsigned int)det_ms,
                (unsigned int)cls_ms_u32,
                (unsigned int)series_ms,
                (unsigned int)capture_to_done_ms,
                (unsigned int)infer_session_id);
        LOG_DBG("PIPE_EVT,session=%u,evt=infer_series,phase=end,capture_to_done_ms=%u",
                (unsigned int)infer_session_id, (unsigned int)capture_to_done_ms);

        cnt_mfcc = 0;
    }

    work_in_progress = false;
    LOG_DBG("mfcc_work: finished, work_in_progress=false");
}

int task_inference_init(void)
{
    /* Initialize MFCC instance with ambient tables */
    arm_status status = arm_mfcc_init_f32(
        &instMFCC,
        N_FFT,
        NB_MEL_FILTERS,
        NB_DCT_MFCC,
        dctMatrixFilters,
        filtPos,
        filtLen,
        packedFilters,
        window);
    if (status != ARM_MATH_SUCCESS) {
        LOG_ERR("MFCC init failed: %d", status);
        return -EINVAL;
    }
    write_point = 0;
    read_point = 0;
    cnt_mfcc = 0;
    ready_for_inference = false;
    memset(audio_data, 0, sizeof(audio_data));
    app_state.infer_last_rms = 0.0f;
    app_state.infer_avg_rms = 0.0f;
    app_state.infer_avg_rms_uplink = 0.0f;
    infer_reset_uplink_window_state();
    k_work_init(&mfcc_work, mfcc_work_handler);
    /* Start dedicated inference workqueue */
    k_work_queue_start(&infer_wq, infer_wq_stack,
                       K_THREAD_STACK_SIZEOF(infer_wq_stack),
                       INFER_WQ_PRIORITY, NULL);
    work_in_progress = false;
    infer_sleep_block = false;
    LOG_INF("Inference pipeline initialized: N_FFT=%d, MFCC frames=%d, coeffs=%d (keep c0), chunks=%d, samples=%d, ring=%d",
            N_FFT, BIRD_MODEL_INPUT_DIM_0, BIRD_MODEL_INPUT_DIM_1, INFERENCE_CHUNKS, MODEL_SAMPLE_COUNT, NB_SAMPLES);
    LOG_INF("Inference uplink buckets: labels=%u low_conf_bucket=%u total_buckets=%u",
            (unsigned int)APP_INFERENCE_LABEL_CNT,
            (unsigned int)APP_INFERENCE_LOW_CONF_BIRD_BUCKET,
            (unsigned int)APP_INFERENCE_UPLINK_BUCKET_COUNT);
    return 0;
}

void task_inference_session_begin(uint32_t session_id, uint32_t capture_start_ms)
{
    /*
     * A new microphone capture session must start from a clean inference state.
     * Otherwise any leftover cnt_mfcc/read_point/write_point/samples_since_window
     * from a previous session can cause early detector execution and mixed timing
     * arrays like [0 0 19 18 19 19].
     */
    if (work_in_progress) {
        LOG_WRN("Starting new inference session while previous work is still marked in progress; resetting pipeline state");
    }

    infer_session_id = session_id;
    infer_capture_start_ms = capture_start_ms;
    infer_done_session_id = 0U;

    write_point = 0U;
    read_point = 0U;
    cnt_mfcc = 0U;
    samples_since_window = 0U;
    ready_for_inference = false;
    work_in_progress = false;

    memset(audio_data, 0, sizeof(audio_data));
    memset(window_buf, 0, sizeof(window_buf));
    memset(inputs, 0, sizeof(inputs));

    app_state.infer_session_id_done = 0U;
    app_state.infer_mfcc_series_ms = 0U;
    app_state.infer_detector_ms = 0U;
    app_state.infer_classifier_ms = 0U;
    app_state.infer_capture_to_done_ms = 0U;
    for (int i = 0; i < 6; i++) {
        app_state.infer_mfcc_chunk_ms[i] = 0U;
    }
}

bool task_inference_wait_series_done(uint32_t session_id, uint32_t timeout_ms)
{
    uint32_t start = k_uptime_get_32();
    while ((k_uptime_get_32() - start) < timeout_ms) {
        if (infer_done_session_id == session_id) {
            return true;
        }
        k_msleep(5);
    }
    return false;
}

/* Feed one RX PCM block (mono int16) into pipeline */
void task_inference_process_block(const void *rx_mem_block, size_t sz_bytes)
{
    if (infer_sleep_block) {
        return;
    }
    if (!rx_mem_block || sz_bytes < sizeof(int16_t)) {
        LOG_ERR("Invalid RX block for inference: %p size=%u", rx_mem_block, (unsigned)sz_bytes);
        return;
    }

    const int16_t *data16 = (const int16_t *)rx_mem_block;
    size_t mono_samples = sz_bytes / sizeof(int16_t);
    selected_audio_channel = 0U;
    LOG_DBG("process_block: rx=%p bytes=%u mono_samples=%u write_point=%u",
            rx_mem_block,
            (unsigned int)sz_bytes,
            (unsigned int)mono_samples,
            (unsigned int)write_point);

    /* Write mono samples into circular buffer */
    for (size_t i = 0; i < mono_samples; i++) {
        size_t widx = write_point % NB_SAMPLES;
        audio_data[widx] = data16[i];
        write_point++;
    }
    samples_since_window += mono_samples;
    LOG_DBG("process_block: appended=%u write_point=%u samples_since_window=%u",
            (unsigned int)mono_samples,
            (unsigned int)write_point,
            (unsigned int)samples_since_window);

    /* Schedule the next MFCC chunk as soon as enough samples for that slice are ready. */
    size_t next_chunk_samples = mfcc_chunk_samples(cnt_mfcc);
    if ((samples_since_window >= next_chunk_samples) && !work_in_progress) {
        ready_for_inference = true;
        LOG_DBG("process_block: chunk ready (idx=%u samples_since_window=%u chunk_samples=%u)",
                (unsigned int)cnt_mfcc,
                (unsigned int)samples_since_window,
                (unsigned int)next_chunk_samples);

        LOG_DBG("process_block: preparing submit (read_point=%u)", (unsigned int)read_point);
        /* Copy one Qualia MFCC chunk into a private buffer to avoid contention. */
        for (size_t i = 0; i < next_chunk_samples; i++) {
            size_t idx = read_point + i;
            if (idx >= NB_SAMPLES) {
                idx -= NB_SAMPLES;
            }
            window_buf[i] = audio_data[idx];
        }
        /* Advance read_point for next chunk and submit background work. */
        read_point = (read_point + next_chunk_samples) % NB_SAMPLES;
        work_in_progress = true;
        /* Submit to dedicated low-priority workqueue */
        k_work_submit_to_queue(&infer_wq, &mfcc_work);
        LOG_DBG("process_block: work submitted, work_in_progress=true, next read_point=%u", (unsigned int)read_point);
        ready_for_inference = false;
        samples_since_window -= next_chunk_samples;
    } else if (ready_for_inference && work_in_progress) {
        LOG_DBG("process_block: skip submit, work still in progress");
    } else {
        LOG_DBG("process_block: not ready (write_point=%u, ready=%d, busy=%d)",
                (unsigned int)write_point,
                ready_for_inference,
                work_in_progress);
    }
}

void task_inference_reset_periodic_stats(void)
{
	infer_reset_periodic_rms_stats();
	LOG_INF("Inference periodic RMS stats reset");
}

void task_inference_consume_uplink_window(void)
{
	infer_reset_uplink_window_state();
	infer_reset_periodic_rms_stats();
	LOG_INF("Inference uplink window consumed: window and RMS stats reset");
}

void task_inference_snapshot_uplink_window(void)
{
	/* Copy live bucket counts into uplink cache, then immediately clear them.
	 * comm_worker reads from the cache asynchronously; if LoRa fails the cache
	 * will simply be overwritten on the next window (discard semantics). */
	memcpy(app_state.infer_label_counts_uplink, app_state.infer_label_counts_window,
	       sizeof(app_state.infer_label_counts_uplink));
    app_state.infer_total_count_uplink = app_state.infer_total_count;
	app_state.infer_avg_rms_uplink = app_state.infer_avg_rms;
	task_inference_consume_uplink_window();
	LOG_INF("Inference window snapshotted to uplink cache (buckets=%u avg_rms=%.6f)",
		(unsigned int)APP_INFERENCE_UPLINK_BUCKET_COUNT,
		(double)app_state.infer_avg_rms_uplink);
}

void task_inference_prepare_codec_zero_fault_uplink(void)
{
	/* Keep the current uplink label snapshot intact; RMS=0 is the fault marker. */
	app_state.infer_total_count_uplink = app_state.infer_total_count;
	app_state.infer_avg_rms_uplink = 0.0f;
	LOG_WRN("Codec zero fault uplink prepared (labels unchanged, total=%u, avg_rms=0)",
		(unsigned int)app_state.infer_total_count_uplink);
}

void task_inference_prepare_sleep(void)
{
    infer_sleep_block = true;
    ready_for_inference = false;

    for (int i = 0; i < 20 && work_in_progress; i++) {
        k_msleep(5);
    }
    if (work_in_progress) {
        LOG_WRN("Inference work still running while preparing sleep");
    }
}

void task_inference_resume_after_wakeup(void)
{
    infer_sleep_block = false;
}
