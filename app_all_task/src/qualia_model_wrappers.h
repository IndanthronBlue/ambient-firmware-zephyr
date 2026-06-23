#ifndef QUALIA_MODEL_WRAPPERS_H_
#define QUALIA_MODEL_WRAPPERS_H_

#include <math.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BIRD_MODEL_INPUT_DIM_0 186
#define BIRD_MODEL_INPUT_DIM_1 10
#define BIRD_DETECT_OUTPUT_SAMPLES 2
#define EZBIRD_OUTPUT_SAMPLES 11

#if defined(CONFIG_APP_INFERENCE_USE_INT16_QUANT_MODEL)
#define BIRD_MODEL_INPUT_SCALE_FACTOR 9
#define BIRD_MODEL_OUTPUT_SCALE_FACTOR 9
typedef int16_t bird_model_input_t;
typedef int16_t bird_model_output_t;

static inline bird_model_input_t bird_model_quantize_input(float value)
{
    float scaled = value * (float)(1 << BIRD_MODEL_INPUT_SCALE_FACTOR);

    if (scaled > 32767.0f) {
        return INT16_MAX;
    }
    if (scaled < -32768.0f) {
        return INT16_MIN;
    }

    return (bird_model_input_t)floorf(scaled);
}

static inline float bird_model_output_to_float(bird_model_output_t value)
{
    return (float)value / (float)(1 << BIRD_MODEL_OUTPUT_SCALE_FACTOR);
}
#else
typedef float bird_model_input_t;
typedef float bird_model_output_t;

static inline bird_model_input_t bird_model_quantize_input(float value)
{
    return value;
}

static inline float bird_model_output_to_float(bird_model_output_t value)
{
    return value;
}
#endif

void bird_detect_model_run(
    const bird_model_input_t input[BIRD_MODEL_INPUT_DIM_0][BIRD_MODEL_INPUT_DIM_1],
    bird_model_output_t output[BIRD_DETECT_OUTPUT_SAMPLES]);

void ezbird_model_run(
    const bird_model_input_t input[BIRD_MODEL_INPUT_DIM_0][BIRD_MODEL_INPUT_DIM_1],
    bird_model_output_t output[EZBIRD_OUTPUT_SAMPLES]);

#ifdef __cplusplus
}
#endif

#endif /* QUALIA_MODEL_WRAPPERS_H_ */
