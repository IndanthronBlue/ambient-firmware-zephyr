#include "qualia_model_wrappers.h"

#include "ezbirdetect.h"

_Static_assert(MODEL_INPUT_DIM_0 == BIRD_MODEL_INPUT_DIM_0, "EZBird classifier input frame count mismatch");
_Static_assert(MODEL_INPUT_DIM_1 == BIRD_MODEL_INPUT_DIM_1,
               "EZBird classifier model must be regenerated for 186x10 MFCC input");
_Static_assert(MODEL_OUTPUT_SAMPLES == EZBIRD_OUTPUT_SAMPLES, "EZBird classifier output size mismatch");
#if defined(CONFIG_APP_INFERENCE_USE_INT16_QUANT_MODEL)
_Static_assert(MODEL_INPUT_SCALE_FACTOR == BIRD_MODEL_INPUT_SCALE_FACTOR,
               "EZBird classifier input scale factor mismatch");
_Static_assert(MODEL_OUTPUT_SCALE_FACTOR == BIRD_MODEL_OUTPUT_SCALE_FACTOR,
               "EZBird classifier output scale factor mismatch");
#endif
_Static_assert(sizeof(input_t) == sizeof(bird_model_input_t[BIRD_MODEL_INPUT_DIM_0][BIRD_MODEL_INPUT_DIM_1]),
               "EZBird classifier input type mismatch");
_Static_assert(sizeof(output_t) == sizeof(bird_model_output_t[EZBIRD_OUTPUT_SAMPLES]),
               "EZBird classifier output type mismatch");

void ezbird_model_run(
    const bird_model_input_t input[BIRD_MODEL_INPUT_DIM_0][BIRD_MODEL_INPUT_DIM_1],
    bird_model_output_t output[EZBIRD_OUTPUT_SAMPLES])
{
    cnn(input, output);
}
