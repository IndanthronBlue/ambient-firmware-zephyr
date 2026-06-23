#include "qualia_model_wrappers.h"

/* The Qualia single-file headers export generic global symbols such as cnn()
 * and cnn_layers_conv1_kernel. Prefix the detector symbols so the detector and
 * classifier can coexist in one firmware image.
 */
#define cnn bird_detect_cnn
#define reset bird_detect_reset
#define cnn_layers_conv1 bird_detect_cnn_layers_conv1
#define cnn_layers_conv2 bird_detect_cnn_layers_conv2
#define cnn_layers_conv3 bird_detect_cnn_layers_conv3
#define cnn_layers_postpool bird_detect_cnn_layers_postpool
#define cnn_layers_flatten bird_detect_cnn_layers_flatten
#define cnn_layers_fc1 bird_detect_cnn_layers_fc1
#define cnn_layers_conv1_output_type bird_detect_cnn_layers_conv1_output_type
#define cnn_layers_conv2_output_type bird_detect_cnn_layers_conv2_output_type
#define cnn_layers_conv3_output_type bird_detect_cnn_layers_conv3_output_type
#define cnn_layers_postpool_output_type bird_detect_cnn_layers_postpool_output_type
#define cnn_layers_flatten_output_type bird_detect_cnn_layers_flatten_output_type
#define cnn_layers_fc1_output_type bird_detect_cnn_layers_fc1_output_type
#define cnn_layers_conv1_bias bird_detect_cnn_layers_conv1_bias
#define cnn_layers_conv1_kernel bird_detect_cnn_layers_conv1_kernel
#define cnn_layers_conv2_bias bird_detect_cnn_layers_conv2_bias
#define cnn_layers_conv2_kernel bird_detect_cnn_layers_conv2_kernel
#define cnn_layers_conv3_bias bird_detect_cnn_layers_conv3_bias
#define cnn_layers_conv3_kernel bird_detect_cnn_layers_conv3_kernel
#define cnn_layers_fc1_bias bird_detect_cnn_layers_fc1_bias
#define cnn_layers_fc1_kernel bird_detect_cnn_layers_fc1_kernel
#define input_t bird_detect_input_t
#define output_t bird_detect_output_t

#include "bird_detect_mfcc_model_floating.h"

_Static_assert(MODEL_INPUT_DIM_0 == BIRD_MODEL_INPUT_DIM_0, "bird detector input frame count mismatch");
_Static_assert(MODEL_INPUT_DIM_1 == BIRD_MODEL_INPUT_DIM_1,
               "bird detector model must be regenerated for 186x10 MFCC input");
_Static_assert(MODEL_OUTPUT_SAMPLES == BIRD_DETECT_OUTPUT_SAMPLES, "bird detector output size mismatch");
#if defined(CONFIG_APP_INFERENCE_USE_INT16_QUANT_MODEL)
_Static_assert(MODEL_INPUT_SCALE_FACTOR == BIRD_MODEL_INPUT_SCALE_FACTOR,
               "bird detector input scale factor mismatch");
_Static_assert(MODEL_OUTPUT_SCALE_FACTOR == BIRD_MODEL_OUTPUT_SCALE_FACTOR,
               "bird detector output scale factor mismatch");
#endif
_Static_assert(sizeof(input_t) == sizeof(bird_model_input_t[BIRD_MODEL_INPUT_DIM_0][BIRD_MODEL_INPUT_DIM_1]),
               "bird detector input type mismatch");
_Static_assert(sizeof(output_t) == sizeof(bird_model_output_t[BIRD_DETECT_OUTPUT_SAMPLES]),
               "bird detector output type mismatch");

void bird_detect_model_run(
    const bird_model_input_t input[BIRD_MODEL_INPUT_DIM_0][BIRD_MODEL_INPUT_DIM_1],
    bird_model_output_t output[BIRD_DETECT_OUTPUT_SAMPLES])
{
    bird_detect_cnn(input, output);
}
