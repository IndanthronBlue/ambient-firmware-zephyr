/* Zephyr-adapted inference pipeline (MFCC + CNN) based on ambient firmware */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

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
#include "bird_detect_mfcc_model_floating.h" /* cnn_detect(), input_t, dense_1_output_type */

int32_t scale_number_t_int16_t(int32_t number, int scale_factor, round_mode_t round_mode);
int16_t scale_and_clamp_to_number_t_int16_t(int32_t number, int scale_factor, round_mode_t round_mode);

/* Undef macros defined by bird_detect_mfcc_model_floating.h that conflict with
 * ezbirdetect.h. Both headers define NUMBER_T, OUTPUT_ROUND_MODE, etc. but may
 * use different values. Undef all of them before including ezbirdetect.h.
 */
#undef NUMBER_T
#undef LONG_NUMBER_T
#undef OUTPUT_ROUND_MODE
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
#undef INPUT_SAMPLES
#undef FC_UNITS
#undef ACTIVATION_LINEAR
/* Suppress warnings from generated model code in ezbirdetect.h:
 * - The cnn() signature uses input_t (float) but internally passes to stem_0()
 *   which expects NUMBER_T (int16_t) — a mismatch in the generated code.
 * - The -Wstringop-overflow is a false positive from GCC's analysis of the
 *   generated model's fixed buffer sizes.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#include "ezbirdetect.h" /* Multiclass classifier: cnn(), classifier_output_type */
#pragma GCC diagnostic pop

int32_t scale_number_t_int16_t(int32_t number, int scale_factor, round_mode_t round_mode)
{
    if (scale_factor <= 0) {
        return number << (-scale_factor);
    }

    if (round_mode == ROUND_MODE_NEAREST) {
        number += (1 << (scale_factor - 1));
    }

    return number >> scale_factor;
}

int16_t scale_and_clamp_to_number_t_int16_t(int32_t number, int scale_factor, round_mode_t round_mode)
{
    int32_t scaled = scale_number_t_int16_t(number, scale_factor, round_mode);

    if (scaled > INT16_MAX) {
        return INT16_MAX;
    }
    if (scaled < INT16_MIN) {
        return INT16_MIN;
    }

    return (int16_t)scaled;
}

/* ===== Pipeline configuration ===== */
#define NB_COEF_MFCC           (MODEL_INPUT_DIM_1 + 1) /* MFCC outputs include C0; model uses 9 coeffs skipping C0 */
#define N_FFT                  256
#define HOP_SIZE               256
#define NB_SAMPLES             15872
#define SAMPLE_SEC             7936
#define ROWS_PER_CHUNK         (SAMPLE_SEC / N_FFT)     /* 7936/256 = 31 */
#define TMP_LEN                (N_FFT + 2)

/* ===== Internal buffers and state ===== */
static int16_t audio_data[NB_SAMPLES];
static volatile size_t write_point = 0;
static volatile size_t read_point = 0;
static volatile bool ready_for_inference = false;
static volatile size_t cnt_mfcc = 0; /* Accumulate 6*31 = 186 frames */
static size_t samples_since_window = 0; /* Producer counter to decide window readiness */

/* MFCC working buffers */
static float32_t input_mfcc[N_FFT];
static float32_t output_mfcc[NB_COEF_MFCC];
static float32_t tmp[TMP_LEN];
static arm_mfcc_instance_f32 instMFCC;

/* CNN inputs and outputs */
static input_t inputs; /* 186x9 float, used by detector and classifier */
static dense_1_output_type outputs_detects; /* 2-class detector */
static classifier_output_type outputs_class; /* 11-class classifier (float or int16 per config) */
/* Running RMS statistics (reset after each successful uplink). */
static float rms_accum = 0.0f;
static uint32_t rms_count = 0U;
static uint8_t selected_audio_channel = 0U; /* Prefer left channel (ADC3101 mono path) */

/* Human-readable label names */
static const char *kBirdLabels[11] = {
    "Common Chiffchaff",    /* 0 */
    "Eurasian Blackbird",   /* 1 */
    "Black Redstart",       /* 2 */
    "Common Chaffinch",     /* 3 */
    "European Robin",       /* 4 */
    "Noise",                /* 5 */
    "Common Nightingale",   /* 6 */
    "Eurasian Wren",        /* 7 */
    "Short-toed Treecreeper",/* 8 */
    "Eurasian Blackcap",    /* 9 */
    "European Serin"        /* 10 */
};

/* ===== Public API ===== */
/* Background work: copy window and process MFCC/classification */
static int16_t window_buf[SAMPLE_SEC];
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
	for (int i = 0; i < 10; i++) {
		app_state.infer_labels_window[i] = -1;
	}
	app_state.infer_window_count = 0U;
	app_state.infer_window_ready = false;
	app_state.infer_total_count = 0U;
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
    /* Process the last copied window_buf into MFCC rows */
    size_t row_start = ROWS_PER_CHUNK * cnt_mfcc;
    size_t row_end   = row_start + ROWS_PER_CHUNK;
    LOG_DBG("mfcc_work: rows %u..%u (ROWS_PER_CHUNK=%u)", (unsigned int)row_start, (unsigned int)(row_end - 1), (unsigned int)ROWS_PER_CHUNK);

    /* Compute RMS of current 7936-sample window (ambient-style). */
    float rms = 0.0f;
    int16_t peak_abs = 0;
    for (size_t i = 0; i < SAMPLE_SEC; i++) {
        float sig = (float)window_buf[i] / 32768.0f;
        rms += sig * sig;
        int16_t abs_v = (window_buf[i] < 0) ? (int16_t)(-window_buf[i]) : window_buf[i];
        if (abs_v > peak_abs) {
            peak_abs = abs_v;
        }
    }
    rms = rms / (float)SAMPLE_SEC;
    rms = sqrtf(rms);
    app_state.infer_last_rms = rms;
    rms_accum += rms;
    rms_count++;
    app_state.infer_avg_rms = (rms_count > 0U) ? (rms_accum / (float)rms_count) : 0.0f;
    LOG_INF("Audio RMS: %.6f, avg RMS: %.6f, peak: %d, ch: %u",
            (double)rms, (double)app_state.infer_avg_rms, (int)peak_abs, (unsigned int)selected_audio_channel);

    for (size_t row = row_start; row < row_end; row++) {
        int start = (int)((row - row_start) * HOP_SIZE);
        for (size_t i = 0; i < N_FFT; i++) {
            size_t idx = start + i; /* within window_buf range */
            input_mfcc[i] = (float32_t)window_buf[idx] / 32768.0f;
        }

        arm_mfcc_f32(&instMFCC, input_mfcc, output_mfcc, tmp);

        for (size_t col = 0; col < MODEL_INPUT_DIM_1; col++) {
            inputs[row][col] = output_mfcc[col + 1];
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

    if (cnt_mfcc == 6) {
        LOG_DBG("mfcc_work: accumulated 6 chunks, running detector");
        uint32_t det_start_ms = k_uptime_get_32();
        LOG_DBG("PIPE_EVT,session=%u,evt=detector,phase=start",
                (unsigned int)infer_session_id);
        cnn_detect(inputs, outputs_detects);
        uint32_t det_ms = k_uptime_get_32() - det_start_ms;
        LOG_DBG("PIPE_EVT,session=%u,evt=detector,phase=end,dur_ms=%u",
                (unsigned int)infer_session_id, (unsigned int)det_ms);
        float bird_score = outputs_detects[1];
        float no_bird_score = outputs_detects[0];
        bool bird_detected = bird_score > no_bird_score;

        LOG_INF("Detector logits: bird=%.3f, none=%.3f", (double)bird_score, (double)no_bird_score);
        LOG_INF("Detector time: %u ms", (unsigned int)det_ms);

        uint32_t cls_ms_u32 = 0U;
        int label = -1;
        if (bird_detected) {
            const float DETECT_CONF_THRESH = 1.5f;
            if (bird_score > DETECT_CONF_THRESH) {
                LOG_DBG("mfcc_work: detector gated true, running classifier");
                uint32_t cls_start_ms = k_uptime_get_32();
                LOG_DBG("PIPE_EVT,session=%u,evt=classifier,phase=start",
                        (unsigned int)infer_session_id);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
                cnn(inputs, outputs_class);
#pragma GCC diagnostic pop
                cls_ms_u32 = k_uptime_get_32() - cls_start_ms;
                LOG_DBG("PIPE_EVT,session=%u,evt=classifier,phase=end,dur_ms=%u",
                        (unsigned int)infer_session_id, (unsigned int)cls_ms_u32);
                int best = 0;
                float best_val = (float)outputs_class[0];
                for (int i = 1; i < 11; i++) {
                    if ((float)outputs_class[i] > best_val) {
                        best_val = (float)outputs_class[i];
                        best = i;
                    }
                }
                label = best;
                const char *label_name = kBirdLabels[best];
                LOG_INF("Bird class: %s (score=%.3f)", label_name, (double)best_val);
                LOG_INF("Classifier time: %u ms", (unsigned int)cls_ms_u32);
                /* Allow other threads to run after heavy classifier */
                k_yield();
            } else {
                label = -2;
                LOG_INF("Detector positive but below confidence threshold");
            }
        } else {
            label = -1;
            LOG_INF("No bird detected");
        }
        app_state.infer_last_label = label;
        app_state.infer_total_count++;
        if (!app_state.infer_window_ready) {
            if (app_state.infer_window_count < 10U) {
                app_state.infer_labels_window[app_state.infer_window_count] = (int8_t)label;
                app_state.infer_window_count++;
            }
            if (app_state.infer_window_count >= 10U) {
                app_state.infer_window_ready = true;
                LOG_INF("Inference window ready: 10 labels collected for uplink");
            }
        }
        LOG_INF("Inference state updated: label=%d total=%u",
                label, (unsigned int)app_state.infer_total_count);

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
        NB_COEF_MFCC,
        NB_COEF_MFCC,
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
    rms_accum = 0.0f;
    rms_count = 0U;
    app_state.infer_last_rms = 0.0f;
    app_state.infer_avg_rms = 0.0f;
    infer_reset_uplink_window_state();
    k_work_init(&mfcc_work, mfcc_work_handler);
    /* Start dedicated inference workqueue */
    k_work_queue_start(&infer_wq, infer_wq_stack,
                       K_THREAD_STACK_SIZEOF(infer_wq_stack),
                       INFER_WQ_PRIORITY, NULL);
    work_in_progress = false;
    infer_sleep_block = false;
    LOG_INF("Inference pipeline initialized: N_FFT=%d, ROWS_PER_CHUNK=%d", N_FFT, ROWS_PER_CHUNK);
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

    /* Schedule MFCC chunk whenever we've accumulated >= one window worth of samples */
    if ((samples_since_window >= SAMPLE_SEC) && !work_in_progress) {
        ready_for_inference = true;
        LOG_DBG("process_block: window ready (samples_since_window=%u)", (unsigned int)samples_since_window);

        LOG_DBG("process_block: preparing submit (read_point=%u)", (unsigned int)read_point);
        /* Copy the 7936-sample window into private buffer to avoid contention */
        for (size_t i = 0; i < SAMPLE_SEC; i++) {
            size_t idx = read_point + i;
            if (idx >= NB_SAMPLES) {
                idx -= NB_SAMPLES;
            }
            window_buf[i] = audio_data[idx];
        }
        /* Advance read_point for next window and submit background work */
        read_point += SAMPLE_SEC;
        if (read_point >= NB_SAMPLES) {
            read_point = 0;
        }
        work_in_progress = true;
        /* Submit to dedicated low-priority workqueue */
        k_work_submit_to_queue(&infer_wq, &mfcc_work);
        LOG_DBG("process_block: work submitted, work_in_progress=true, next read_point=%u", (unsigned int)read_point);
        ready_for_inference = false;
        samples_since_window -= SAMPLE_SEC;
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
    rms_accum = 0.0f;
    rms_count = 0U;
    app_state.infer_avg_rms = 0.0f;
    LOG_INF("Inference periodic RMS stats reset after uplink");
}

void task_inference_consume_uplink_window(void)
{
	infer_reset_uplink_window_state();
	LOG_INF("Inference uplink window consumed: counters reset");
}

void task_inference_snapshot_uplink_window(void)
{
	/* Copy live window into uplink cache, then immediately clear live window.
	 * comm_worker reads from the cache asynchronously; if LoRa fails the cache
	 * will simply be overwritten on the next window (discard semantics). */
	memcpy(app_state.infer_labels_uplink, app_state.infer_labels_window,
	       sizeof(app_state.infer_labels_uplink));
    app_state.infer_total_count_uplink = app_state.infer_total_count;
	task_inference_consume_uplink_window();
	LOG_INF("Inference window snapshotted to uplink cache");
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
