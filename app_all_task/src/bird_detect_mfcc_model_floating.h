#define SINGLE_FILE
/**
  ******************************************************************************
  * @file    defines.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, Université Côte d'Azur, LEAT, France
  * @version 2.1.0
  * @date    10 january 2024
  * @brief   Global C pre-processor definitions to use to build all source files (incl. CMSIS-NN)
  */

/* CMSIS-NN round mode definition */
#if defined(WITH_CMSIS_NN) || defined(WITH_NMSIS_NN)


#define ARM_NN_TRUNCATE 1
#define RISCV_NN_TRUNCATE 1

#endif // defined(WITH_CMSIS_NN) || defined(WITH_NMSIS_NN)
/**
  ******************************************************************************
  * @file    number.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    2 february 2021
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __NUMBER_H__
#define __NUMBER_H__

#include <stdint.h>
#include <stddef.h>
#include <math.h>

#ifdef TRAPV_SHIFT
#include <limits.h>
#include <stdio.h>
#include <assert.h>
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#endif

#define _clamp_to(type, number) clamp_to_number_t_ ## type (number)
#define clamp_to(type, number) _clamp_to(type, number)
#define _scale(type, number, scale_factor, round_mode) scale_number_t_ ## type (number, scale_factor, round_mode)
#define scale(type, number, scale_factor, round_mode) _scale(type, number, scale_factor, round_mode)
#define _scale_and_clamp_to(type, number, scale_factor, round_mode) scale_and_clamp_to_number_t_ ## type (number, scale_factor, round_mode)
#define scale_and_clamp_to(type, number, scale_factor, round_mode) _scale_and_clamp_to(type, number, scale_factor, round_mode)

typedef enum {
  ROUND_MODE_NONE,
  ROUND_MODE_FLOOR,
  ROUND_MODE_NEAREST,
} round_mode_t;

// Idea 1: Write the smallest min max interval of the net, could be an issue for hybrid int type network
// Idea 2: listing any interval and add type in name in a switch case like <- better but painfull
// #define NUMBER_MIN		// Max value for this numeric type
// #define NUMBER_MAX		// Min value for this numeric type

// // Idea 1: List of all types and write any corresponding function 
// typedef  number_t;		// Standard size numeric type used for weights and activations
// typedef  long_number_t;	// Long numeric type used for intermediate results

#define NUMBER_MIN_INT32_T -2147483648
#define NUMBER_MAX_INT32_T 2147483647

static inline int64_t min_int32_t(
    int64_t a,
    int64_t b) {
	if (a <= b)
		return a;
	return b;
}

static inline int64_t max_int32_t(
    int64_t a,
    int64_t b) {
	if (a >= b)
		return a;
	return b;
}

static inline int64_t scale_number_t_int32_t(
  int64_t number, int scale_factor, round_mode_t round_mode) {


  if (scale_factor <= 0) {
#ifdef TRAPV_SHIFT
    // Check for possible overflow of left shift
    if (number > INT64_MAX >> -scale_factor) {
      fprintf(stderr,
              "Error: scale() overflow, number=%ld, scale_factor=%d, limit=%d\n",
              number,
              scale_factor,
              INT16_MAX >> -scale_factor);
      assert(number <= INT64_MAX >> -scale_factor);
    }
#endif
    // No rounding to apply when shifting left
    return number << - scale_factor;
  } else {
    if (round_mode == ROUND_MODE_NEAREST) {
      number += (1 << (scale_factor - 1)); // +0.5 in fixed-point
    }
    return number >> scale_factor;
  }
}
static inline int32_t clamp_to_number_t_int32_t(
  int64_t number) {
	return (int32_t) max_int32_t(
      NUMBER_MIN_INT32_T,
      min_int32_t(
        NUMBER_MAX_INT32_T, number));
}
static inline int32_t scale_and_clamp_to_number_t_int32_t(
  int64_t number, int scale_factor, round_mode_t round_mode) {
#ifdef WITH_CMSIS_NN
  // Not really CMSIS-NN but use SSAT anyway
  if (scale_factor <= 0) {
    // No rounding to apply when shifting left
    return __SSAT(number << - scale_factor, sizeof(int32_t) * 8);
  } else {
    if (round_mode == ROUND_MODE_NEAREST) {
      number += (1 << (scale_factor - 1)); // +0.5 in fixed-point
    }
    return __SSAT(number >> scale_factor, sizeof(int32_t) * 8);
  }
#else
  number = scale_number_t_int32_t(number, scale_factor, round_mode);
  return clamp_to_number_t_int32_t(number);
#endif
}

#define NUMBER_MIN_INT16_T -32768
#define NUMBER_MAX_INT16_T 32767

static inline int32_t min_int16_t(
    int32_t a,
    int32_t b) {
	if (a <= b)
		return a;
	return b;
}

static inline int32_t max_int16_t(
    int32_t a,
    int32_t b) {
	if (a >= b)
		return a;
	return b;
}

static inline int32_t scale_number_t_int16_t(
  int32_t number, int scale_factor, round_mode_t round_mode) {


  if (scale_factor <= 0) {
#ifdef TRAPV_SHIFT
    // Check for possible overflow of left shift
    if (number > INT32_MAX >> -scale_factor) {
      fprintf(stderr,
              "Error: scale() overflow, number=%d, scale_factor=%d, limit=%d\n",
              number,
              scale_factor,
              INT16_MAX >> -scale_factor);
      assert(number <= INT32_MAX >> -scale_factor);
    }
#endif
    // No rounding to apply when shifting left
    return number << - scale_factor;
  } else {
    if (round_mode == ROUND_MODE_NEAREST) {
      number += (1 << (scale_factor - 1)); // +0.5 in fixed-point
    }
    return number >> scale_factor;
  }
}
static inline int16_t clamp_to_number_t_int16_t(
  int32_t number) {
	return (int16_t) max_int16_t(
      NUMBER_MIN_INT16_T,
      min_int16_t(
        NUMBER_MAX_INT16_T, number));
}
static inline int16_t scale_and_clamp_to_number_t_int16_t(
  int32_t number, int scale_factor, round_mode_t round_mode) {
#ifdef WITH_CMSIS_NN
  // Not really CMSIS-NN but use SSAT anyway
  if (scale_factor <= 0) {
    // No rounding to apply when shifting left
    return __SSAT(number << - scale_factor, sizeof(int16_t) * 8);
  } else {
    if (round_mode == ROUND_MODE_NEAREST) {
      number += (1 << (scale_factor - 1)); // +0.5 in fixed-point
    }
    return __SSAT(number >> scale_factor, sizeof(int16_t) * 8);
  }
#else
  number = scale_number_t_int16_t(number, scale_factor, round_mode);
  return clamp_to_number_t_int16_t(number);
#endif
}




static inline void int64_t_to_float(int64_t * tabint, float * tabfloat, long tabdim, int scale_factor){
  for (int i=0; i<tabdim; i++){
    tabfloat[i] = (float)tabint[i] / (1<<scale_factor);
  }
}

static inline void int32_t_to_float(int32_t * tabint, float * tabfloat, long tabdim, int scale_factor){
  for (int i=0; i<tabdim; i++){
    tabfloat[i] = (float)tabint[i] / (1<<scale_factor);
  }
}

static inline void int16_t_to_float(int16_t * tabint, float * tabfloat, long tabdim, int scale_factor){
  for (int i=0; i<tabdim; i++){
    tabfloat[i] = ((float)tabint[i]) / (1<<scale_factor);
  }
}

static inline void int8_t_to_float(int8_t * tabint, float * tabfloat, long tabdim, int scale_factor){
  for (int i=0; i<tabdim; i++){
    tabfloat[i] = ((float)tabint[i]) / (1<<scale_factor);
  }
}
#endif //__NUMBER_H__

#ifdef __cplusplus
} // extern "C"
#endif
/**
  ******************************************************************************
  * @file    conv1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _CNN_LAYERS_CONV1_H_
#define _CNN_LAYERS_CONV1_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      10
#define INPUT_SAMPLES       186
#define CONV_FILTERS        16
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         2

#define ZEROPADDING_LEFT    1
#define ZEROPADDING_RIGHT   1

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

typedef int16_t cnn_layers_conv1_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void cnn_layers_conv1(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const number_t kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS],  // IN

  const number_t bias[CONV_FILTERS],						                          // IN

  number_t output[CONV_OUTSAMPLES][CONV_FILTERS]);                       // OUT
#endif

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_STRIDE
#undef ZEROPADDING_LEFT
#undef ZEROPADDING_RIGHT
#undef CONV_OUTSAMPLES

#endif//_CNN_LAYERS_CONV1_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "cnn_layers_conv1.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#elif defined(WITH_NMSIS_NN)
#include "riscv_nnfunctions.h"
#endif

#define INPUT_CHANNELS      10
#define INPUT_SAMPLES       186
#define CONV_FILTERS        16
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         2
#define CONV_GROUPS         1
#define CHANNELS_PER_GROUP  (INPUT_CHANNELS / CONV_GROUPS)
#define FILTERS_PER_GROUP   (CONV_FILTERS / CONV_GROUPS)

#define ZEROPADDING_LEFT    1
#define ZEROPADDING_RIGHT   1

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

#define ACTIVATION_RELU

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 9
#define BIASES_SCALE_FACTOR 9
#define TMP_SCALE_FACTOR 9
#define INPUT_SCALE_FACTOR 9
#define OUTPUT_SCALE_FACTOR 9
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void cnn_layers_conv1(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const NUMBER_T kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS],  // IN

  const NUMBER_T bias[CONV_FILTERS],						                          // IN

  NUMBER_T output[CONV_OUTSAMPLES][CONV_FILTERS]) {                       // OUT

#if !defined(WITH_CMSIS_NN) && !defined(WITH_NMSIS_NN)
  unsigned short pos_x, z, k; 	// loop indexes for output volume
  unsigned short x;
  int input_x;
  LONG_NUMBER_T output_acc;

  for (pos_x = 0; pos_x < CONV_OUTSAMPLES; pos_x++) { 
    for (k = 0; k < CONV_FILTERS; k++) { 
      output_acc = 0;

      for (x = 0; x < CONV_KERNEL_SIZE; x++) {
        input_x = pos_x * CONV_STRIDE - ZEROPADDING_LEFT + x;

        if (input_x >= 0 && input_x < INPUT_SAMPLES) { // ZeroPadding1D
          for (z = 0; z < INPUT_CHANNELS / CONV_GROUPS; z++) {
            output_acc += (LONG_NUMBER_T)input[input_x][z + (k / FILTERS_PER_GROUP) * CHANNELS_PER_GROUP] * (LONG_NUMBER_T)kernel[k][x][z];
          }
        }
      }

    // Scale for possible additional precision of bias
    output_acc = scale(NUMBER_T, output_acc, WEIGHTS_SCALE_FACTOR - TMP_SCALE_FACTOR, OUTPUT_ROUND_MODE);

    // Scale bias to match accumulator
    output_acc += scale(NUMBER_T, (LONG_NUMBER_T)bias[k], BIASES_SCALE_FACTOR - TMP_SCALE_FACTOR - INPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);

      
#ifdef ACTIVATION_LINEAR
      output[pos_x][k] = scale_and_clamp_to(NUMBER_T, output_acc, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU) || defined(ACTIVATION_RELU6)
      // Activation function: ReLU
      if (output_acc < 0) {
        output[pos_x][k] = 0;
      } else {
#if defined(ACTIVATION_RELU6)
        if (output_acc > scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE)) {
          output_acc = scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE);
        }
#endif
        output[pos_x][k] = scale_and_clamp_to(NUMBER_T, output_acc, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      }
#else
#error "Unsupported activation function"
#endif
    }
  }

#else

#if BIASES_SCALE_FACTOR > WEIGHTS_SCALE_FACTOR
#error "CMSIS-NN does not support BIASES_SCALE_FACTOR larger than WEIGHTS_SCALE_FACTOR"
#endif

  static q15_t bufferA[2*INPUT_CHANNELS*CONV_KERNEL_SIZE];
#if INPUT_CHANNELS % 2 == 0 && CONV_FILTERS % 2 == 0 && CONV_OUTSAMPLES % 2 == 0
#ifdef WITH_CMSIS_NN
  arm_convolve_HWC_q15_fast_nonsquare(
#elif defined(WITH_NMSIS_NN)
  riscv_convolve_HWC_q15_fast_nonsquare(
#endif
#else
#ifdef WITH_CMSIS_NN
  arm_convolve_HWC_q15_basic_nonsquare(
#elif defined(WITH_NMSIS_NN)
  riscv_convolve_HWC_q15_basic_nonsquare(
#endif
#endif
                                      (q15_t*)input, //Im_in
                                      INPUT_SAMPLES, //dim_im_in_x
                                      1, //dim_im_in_y
                                      INPUT_CHANNELS, //ch_im_in
                                      (q15_t*)kernel, //wt
                                      CONV_FILTERS, //ch_im_out
                                      CONV_KERNEL_SIZE, //dim_kernel_x
                                      1, //dim_kernel_y
                                      ZEROPADDING_LEFT, //padding_x, left and right must be equal
                                      0, //padding_y
                                      CONV_STRIDE, //stride_x
                                      1, //stride_y
                                      (q15_t*)bias, //bias
                                      INPUT_SCALE_FACTOR + WEIGHTS_SCALE_FACTOR - BIASES_SCALE_FACTOR, //bias_shift
                                      INPUT_SCALE_FACTOR + WEIGHTS_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, //out_shift
                                      (q15_t*)output, //Im_out
                                      CONV_OUTSAMPLES, //dim_im_out_x
                                      1, //dim_im_out_y
                                      bufferA, //bufferA
                                      NULL //bufferB, unused
                                      );
#ifdef ACTIVATION_RELU
#ifdef WITH_CMSIS_NN
  arm_relu_q15((q15_t*)output, CONV_FILTERS * CONV_OUTSAMPLES);
#elif defined(WITH_NMSIS_NN)
  riscv_relu_q15((q15_t*)output, CONV_FILTERS * CONV_OUTSAMPLES);
#endif
#elif !defined(ACTIVATION_LINEAR)
#error "Unsupported activation with CMSIS-NN"
#endif


#endif
}

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_STRIDE
#undef CONV_GROUPS
#undef CHANNELS_PER_GROUP
#undef FILTERS_PER_GROUP
#undef ZEROPADDING_LEFT
#undef ZEROPADDING_RIGHT
#undef CONV_OUTSAMPLES
#undef ACTIVATION_RELU
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
#undef NUMBER_T
#undef LONG_NUMBER_T
/**
  ******************************************************************************
  * @file    weights/conv1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

#define INPUT_CHANNELS    10
#define CONV_FILTERS      16
#define CONV_KERNEL_SIZE  3
#define CONV_GROUPS       1


const int16_t  cnn_layers_conv1_bias[CONV_FILTERS] = {-1248, 1672, -845, -872, -531, 1573, 108, 2417, 1676, -2857, -844, -167, 287, -1467, -1664, -1062}
;

const int16_t  cnn_layers_conv1_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{38, 16, 35, -19, 3, -87, 98, -78, -121, -119}
, {-39, 81, -28, 25, 97, -162, 110, -24, -105, -122}
, {55, 45, 58, 9, 139, -221, -28, -7, -89, -83}
}
, {{-89, 16, 30, 28, 27, 19, 11, -11, 16, 7}
, {-4, -19, -4, 40, 4, 47, 0, 10, 43, -6}
, {36, -5, -2, 10, -13, 36, 9, -10, 33, 21}
}
, {{67, 35, 16, 6, -107, -1, 8, -85, 71, 77}
, {-53, 63, -41, -70, -43, -47, 36, -139, 39, 9}
, {24, 5, -18, 29, -39, -108, 115, -86, 25, 22}
}
, {{-19, 10, -177, 4, -3, 199, -146, -71, -23, 213}
, {-23, 81, -237, 31, 130, 173, -118, -182, 60, 193}
, {76, -4, -160, -16, 89, 191, 38, -260, 101, 253}
}
, {{71, 55, -51, -47, -92, -29, -30, 69, 187, 58}
, {17, 34, -35, -21, -122, -140, -104, 143, 193, 9}
, {-72, -84, -140, 67, 22, -211, -177, 97, 110, -74}
}
, {{-1, 78, -60, -22, -7, 98, 67, 87, -24, 73}
, {4, -56, 194, -241, 26, 64, -115, 378, -248, 165}
, {-74, -103, 136, -83, -84, 178, -204, 416, -277, 147}
}
, {{-54, 172, -186, 95, -42, -125, 383, -260, 58, 138}
, {-5, -62, 98, -52, -108, -92, 332, -397, 251, 20}
, {48, -148, 243, -191, -222, 114, -84, -120, 176, -102}
}
, {{-77, -56, -5, 143, 25, -86, 40, -80, -196, -86}
, {-1, -35, -12, 103, -1, -88, -91, -108, -126, -71}
, {-7, 85, 15, 25, -42, 38, -17, -141, -132, -33}
}
, {{-50, 16, 33, 22, 89, 112, -9, 76, 28, -32}
, {42, 81, -29, 49, 2, 77, 6, 114, 57, -31}
, {-33, 31, -85, 155, 15, 60, -16, 153, -53, -30}
}
, {{39, -74, 10, 23, 78, -21, -20, 23, 76, 143}
, {35, -104, -4, 100, -57, 192, -202, -26, 74, 40}
, {-1, -18, 47, 45, 42, 139, -93, -85, 207, 6}
}
, {{10, -59, 127, -23, -113, 198, 2, 94, 110, 1}
, {-59, -22, -32, 2, -1, -19, 110, 125, 1, 41}
, {88, 166, -23, -84, -39, -70, 75, 130, 15, 182}
}
, {{-51, 68, -41, -47, 98, -112, -6, 47, 141, -217}
, {16, -35, -41, -96, 257, -242, 58, -15, 51, -21}
, {36, -31, 109, -258, 201, -101, -66, 159, -19, -63}
}
, {{66, -66, -88, 11, 39, 28, -25, -75, -65, -26}
, {-57, -64, -93, -32, 30, 3, 8, -76, -65, 15}
, {-40, -19, -76, -39, -28, 7, -32, 1, -40, 25}
}
, {{56, -28, -86, 119, -146, 168, 65, -286, 147, -87}
, {58, 74, 1, 23, -42, 86, 17, -133, 107, -117}
, {-67, 10, 24, 66, -76, 50, -8, -69, 68, -76}
}
, {{61, -109, 234, -117, 92, -4, -104, 109, -3, -18}
, {-49, -21, 108, -70, -97, 193, -207, 182, 173, -207}
, {29, 32, 9, 39, -204, 97, -112, 85, 194, -148}
}
, {{9, -41, -170, 268, -289, 29, 67, 41, 56, -217}
, {35, -62, -137, 229, -139, -6, 61, 85, -8, -48}
, {-20, -9, -127, 153, 0, -78, 75, 97, -119, 30}
}
}
;

#undef INPUT_CHANNELS
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_GROUPS
/**
  ******************************************************************************
  * @file    conv1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _CNN_LAYERS_CONV2_H_
#define _CNN_LAYERS_CONV2_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      16
#define INPUT_SAMPLES       93
#define CONV_FILTERS        32
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         2

#define ZEROPADDING_LEFT    1
#define ZEROPADDING_RIGHT   1

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

typedef int16_t cnn_layers_conv2_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void cnn_layers_conv2(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const number_t kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS],  // IN

  const number_t bias[CONV_FILTERS],						                          // IN

  number_t output[CONV_OUTSAMPLES][CONV_FILTERS]);                       // OUT
#endif

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_STRIDE
#undef ZEROPADDING_LEFT
#undef ZEROPADDING_RIGHT
#undef CONV_OUTSAMPLES

#endif//_CNN_LAYERS_CONV2_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "cnn_layers_conv2.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#elif defined(WITH_NMSIS_NN)
#include "riscv_nnfunctions.h"
#endif

#define INPUT_CHANNELS      16
#define INPUT_SAMPLES       93
#define CONV_FILTERS        32
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         2
#define CONV_GROUPS         1
#define CHANNELS_PER_GROUP  (INPUT_CHANNELS / CONV_GROUPS)
#define FILTERS_PER_GROUP   (CONV_FILTERS / CONV_GROUPS)

#define ZEROPADDING_LEFT    1
#define ZEROPADDING_RIGHT   1

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

#define ACTIVATION_RELU

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 9
#define BIASES_SCALE_FACTOR 9
#define TMP_SCALE_FACTOR 9
#define INPUT_SCALE_FACTOR 9
#define OUTPUT_SCALE_FACTOR 9
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void cnn_layers_conv2(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const NUMBER_T kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS],  // IN

  const NUMBER_T bias[CONV_FILTERS],						                          // IN

  NUMBER_T output[CONV_OUTSAMPLES][CONV_FILTERS]) {                       // OUT

#if !defined(WITH_CMSIS_NN) && !defined(WITH_NMSIS_NN)
  unsigned short pos_x, z, k; 	// loop indexes for output volume
  unsigned short x;
  int input_x;
  LONG_NUMBER_T output_acc;

  for (pos_x = 0; pos_x < CONV_OUTSAMPLES; pos_x++) { 
    for (k = 0; k < CONV_FILTERS; k++) { 
      output_acc = 0;

      for (x = 0; x < CONV_KERNEL_SIZE; x++) {
        input_x = pos_x * CONV_STRIDE - ZEROPADDING_LEFT + x;

        if (input_x >= 0 && input_x < INPUT_SAMPLES) { // ZeroPadding1D
          for (z = 0; z < INPUT_CHANNELS / CONV_GROUPS; z++) {
            output_acc += (LONG_NUMBER_T)input[input_x][z + (k / FILTERS_PER_GROUP) * CHANNELS_PER_GROUP] * (LONG_NUMBER_T)kernel[k][x][z];
          }
        }
      }

    // Scale for possible additional precision of bias
    output_acc = scale(NUMBER_T, output_acc, WEIGHTS_SCALE_FACTOR - TMP_SCALE_FACTOR, OUTPUT_ROUND_MODE);

    // Scale bias to match accumulator
    output_acc += scale(NUMBER_T, (LONG_NUMBER_T)bias[k], BIASES_SCALE_FACTOR - TMP_SCALE_FACTOR - INPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);

      
#ifdef ACTIVATION_LINEAR
      output[pos_x][k] = scale_and_clamp_to(NUMBER_T, output_acc, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU) || defined(ACTIVATION_RELU6)
      // Activation function: ReLU
      if (output_acc < 0) {
        output[pos_x][k] = 0;
      } else {
#if defined(ACTIVATION_RELU6)
        if (output_acc > scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE)) {
          output_acc = scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE);
        }
#endif
        output[pos_x][k] = scale_and_clamp_to(NUMBER_T, output_acc, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      }
#else
#error "Unsupported activation function"
#endif
    }
  }

#else

#if BIASES_SCALE_FACTOR > WEIGHTS_SCALE_FACTOR
#error "CMSIS-NN does not support BIASES_SCALE_FACTOR larger than WEIGHTS_SCALE_FACTOR"
#endif

  static q15_t bufferA[2*INPUT_CHANNELS*CONV_KERNEL_SIZE];
#if INPUT_CHANNELS % 2 == 0 && CONV_FILTERS % 2 == 0 && CONV_OUTSAMPLES % 2 == 0
#ifdef WITH_CMSIS_NN
  arm_convolve_HWC_q15_fast_nonsquare(
#elif defined(WITH_NMSIS_NN)
  riscv_convolve_HWC_q15_fast_nonsquare(
#endif
#else
#ifdef WITH_CMSIS_NN
  arm_convolve_HWC_q15_basic_nonsquare(
#elif defined(WITH_NMSIS_NN)
  riscv_convolve_HWC_q15_basic_nonsquare(
#endif
#endif
                                      (q15_t*)input, //Im_in
                                      INPUT_SAMPLES, //dim_im_in_x
                                      1, //dim_im_in_y
                                      INPUT_CHANNELS, //ch_im_in
                                      (q15_t*)kernel, //wt
                                      CONV_FILTERS, //ch_im_out
                                      CONV_KERNEL_SIZE, //dim_kernel_x
                                      1, //dim_kernel_y
                                      ZEROPADDING_LEFT, //padding_x, left and right must be equal
                                      0, //padding_y
                                      CONV_STRIDE, //stride_x
                                      1, //stride_y
                                      (q15_t*)bias, //bias
                                      INPUT_SCALE_FACTOR + WEIGHTS_SCALE_FACTOR - BIASES_SCALE_FACTOR, //bias_shift
                                      INPUT_SCALE_FACTOR + WEIGHTS_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, //out_shift
                                      (q15_t*)output, //Im_out
                                      CONV_OUTSAMPLES, //dim_im_out_x
                                      1, //dim_im_out_y
                                      bufferA, //bufferA
                                      NULL //bufferB, unused
                                      );
#ifdef ACTIVATION_RELU
#ifdef WITH_CMSIS_NN
  arm_relu_q15((q15_t*)output, CONV_FILTERS * CONV_OUTSAMPLES);
#elif defined(WITH_NMSIS_NN)
  riscv_relu_q15((q15_t*)output, CONV_FILTERS * CONV_OUTSAMPLES);
#endif
#elif !defined(ACTIVATION_LINEAR)
#error "Unsupported activation with CMSIS-NN"
#endif


#endif
}

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_STRIDE
#undef CONV_GROUPS
#undef CHANNELS_PER_GROUP
#undef FILTERS_PER_GROUP
#undef ZEROPADDING_LEFT
#undef ZEROPADDING_RIGHT
#undef CONV_OUTSAMPLES
#undef ACTIVATION_RELU
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
#undef NUMBER_T
#undef LONG_NUMBER_T
/**
  ******************************************************************************
  * @file    weights/conv1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

#define INPUT_CHANNELS    16
#define CONV_FILTERS      32
#define CONV_KERNEL_SIZE  3
#define CONV_GROUPS       1


const int16_t  cnn_layers_conv2_bias[CONV_FILTERS] = {335, -385, 119, 272, 210, 377, -169, 439, 580, -345, 569, -287, 315, -276, 22, 317, 304, 411, 282, 156, 310, 464, 14, 31, -1, 202, -81, -42, -229, 400, -21, -257}
;

const int16_t  cnn_layers_conv2_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{-24, 100, 47, 10, -84, -55, -11, 84, -43, -107, 48, -24, -57, -125, -92, -29}
, {55, -89, 32, -77, -28, -35, -204, -22, 2, -35, -47, -68, 1, -63, -93, -83}
, {26, 19, 3, 13, -67, -77, -59, -72, -68, 39, 72, -43, 55, -6, -83, -113}
}
, {{191, 82, 43, 76, -87, 25, 72, -36, 58, 97, 88, -95, -138, -30, 28, -147}
, {33, 135, 91, 6, 83, 28, 95, 26, 220, 118, 43, 73, -1, -74, 53, -85}
, {114, -3, -56, -19, -3, 62, 35, -66, 40, 106, 0, -118, -2, -4, -36, -81}
}
, {{-253, 63, -43, 23, -178, 126, 183, -29, -9, 223, 115, -158, 65, 121, -21, 25}
, {-79, 36, 15, -24, -90, -30, 56, -16, 132, 38, -26, -215, -13, -33, -80, 9}
, {-250, 7, 73, -34, -166, 233, 176, 29, 20, 4, 30, -304, -173, -150, -42, -2}
}
, {{32, 47, 104, -48, -15, -110, -94, -53, 52, 99, -2, -31, -9, 46, -254, -219}
, {-56, 100, 132, 34, -44, -216, -124, 33, 15, 86, 90, -74, 29, -110, -108, -220}
, {-1, 124, 45, -48, -22, -7, -127, 62, 11, 12, 67, 62, -82, -70, -154, -31}
}
, {{97, -30, -93, 203, -37, -195, -43, 144, -16, 328, -226, 146, 272, 207, 155, 470}
, {52, -196, -46, -266, -68, -97, 26, 94, 74, 267, -179, -121, 174, 116, -37, -145}
, {98, -50, 147, -313, -17, -170, -39, -98, -30, -185, -90, -213, -28, -187, -231, -592}
}
, {{19, 30, -94, -134, -55, -96, -86, 10, -83, 53, 72, -18, 16, 74, 3, -62}
, {-48, 16, -161, -54, 85, -8, -41, -85, -143, -34, -81, -157, 11, 83, 85, 67}
, {-130, 65, -202, -120, -166, -112, 5, -47, -27, -15, -148, -68, -54, 104, 64, -48}
}
, {{58, 2, 19, 101, 39, 32, -50, 0, 0, -127, 18, 13, -151, 54, 119, -76}
, {71, 92, 49, 82, 53, 65, 77, -13, 71, -132, -5, -55, -41, 76, 18, -137}
, {19, -15, -96, 20, 115, 111, 18, 63, 92, -129, 29, -70, -15, 49, 32, -100}
}
, {{21, -126, -109, 97, -66, 4, 27, -23, -80, -59, -48, 50, 71, -280, -78, -202}
, {-88, 27, -92, 27, -15, -123, 3, -207, 51, 69, -44, -275, 216, -137, 78, 6}
, {36, 68, -14, -71, -28, 5, 14, 50, 11, 44, 6, -233, 74, -37, -152, 104}
}
, {{-50, -79, -44, -7, -73, -30, -91, 48, -21, -124, 30, -112, -88, -132, -10, -65}
, {-92, 18, 51, -46, -7, -18, 20, 29, -7, 41, -1, -19, -126, -129, 29, -144}
, {55, -11, -103, 10, -48, -147, -51, -40, -34, -22, -82, 7, -75, -95, 56, -146}
}
, {{200, -22, 15, 329, -144, -58, 118, -385, 304, -118, -86, -76, -24, -446, -374, -370}
, {-142, -170, -147, 358, -48, 210, 108, 115, -3, 211, -199, 267, -20, -7, 300, 247}
, {-180, -227, 187, -139, -353, 97, 132, 166, -259, 92, -26, 241, -33, 272, 102, 198}
}
, {{-216, -32, -15, -49, 66, -117, 142, 23, 100, -75, -103, -178, -90, -180, -116, -180}
, {-20, 88, -72, -29, 19, -49, 7, -26, 70, -58, -12, -32, 29, -135, -16, -133}
, {-136, 74, -16, 74, 70, -93, 89, -30, -72, 88, -144, -54, 45, -176, -10, -147}
}
, {{-43, -135, 211, -76, -137, -111, 170, 25, -43, 144, -35, 25, -68, 116, -46, -53}
, {154, -233, 30, 35, -168, -196, 207, 99, 52, 88, 6, 110, 53, 133, -59, 156}
, {68, -158, -30, 149, -193, 1, 0, 210, -98, -50, -21, 126, 176, -13, -148, 97}
}
, {{63, -18, 98, -128, -40, 32, -70, -122, 39, 84, -36, -123, 112, -71, 43, -48}
, {64, -63, 17, -76, -24, -28, -173, -159, -16, 50, 16, -12, 102, 21, 30, -60}
, {43, 16, -90, -136, -7, 6, -111, -123, 65, 25, -92, -96, 57, 42, -74, -29}
}
, {{169, -40, -198, 81, 211, -222, -226, 115, 118, -170, 133, -173, -79, -228, 104, 40}
, {94, -281, -208, 270, 116, -160, 56, -142, 69, -85, 31, 332, 97, -301, -191, 153}
, {-84, 14, -236, 44, -119, 260, 411, -127, -6, 273, -144, -82, -16, -38, 279, 73}
}
, {{-98, 20, -152, -28, -151, 34, -67, 43, -3, 15, -19, 78, -123, -113, -141, -1}
, {-4, 134, 17, 8, -135, -23, 3, 68, -43, 28, 138, 118, 11, -146, -24, -5}
, {21, 64, 5, -17, -37, 27, -28, 68, -41, -44, 186, -29, -83, -66, -28, 15}
}
, {{-113, 83, 39, 47, 22, 185, 60, -40, 4, 5, -16, 97, 22, 224, 102, -67}
, {-20, -54, 91, -133, 36, -100, -191, 115, -72, -160, 56, -272, -10, 101, 41, -37}
, {-81, -110, 68, -20, 79, -118, -284, 18, 21, -234, 50, -124, -156, -29, -36, -240}
}
, {{-102, 12, 54, 50, -62, -65, 0, 12, 44, -201, 31, 17, -94, -107, -122, 41}
, {22, -102, -9, 10, -45, -101, -73, -115, 21, -190, 18, -93, -147, 50, -17, 54}
, {-18, 19, 47, 34, -16, -82, -80, -85, -25, -175, 98, -56, -24, 13, 36, -59}
}
, {{-79, 54, 106, -17, 132, -157, -156, 103, -21, 71, 153, -140, -24, 9, -94, -177}
, {-181, -71, -2, 9, 36, -182, -54, 159, 152, -66, 86, -30, -36, 160, 43, -199}
, {-130, -17, -16, -220, 25, -334, -285, 90, -5, -45, -1, -79, 77, -46, 104, -214}
}
, {{4, -35, -35, -47, 41, -38, 15, -17, 24, 34, -186, -63, 14, -79, -53, -177}
, {-114, -25, -158, -70, -89, -27, 60, -1, 49, -33, -70, -32, 67, -99, -29, -65}
, {30, -24, -15, 14, -69, -4, 25, -55, -10, -27, 51, -64, 45, -60, -10, -89}
}
, {{11, 34, -16, 39, -78, -76, -144, -12, 11, -13, -85, -42, -69, -67, -165, 10}
, {-32, 89, -58, -62, -82, -17, -48, 115, 120, -19, -69, 38, -38, -86, -147, -20}
, {46, -43, -50, 36, -58, -37, -125, 85, 105, 68, 27, -44, 48, -69, -114, -37}
}
, {{-54, -12, 63, 30, 49, -157, 8, -74, -64, -93, 34, -141, -147, 64, -123, -53}
, {52, 24, 71, 20, 66, -73, -22, -100, 97, -171, -30, -135, -186, 79, -209, -24}
, {-56, 108, 34, 74, 25, -153, 11, -56, 20, -241, -9, -17, 33, -70, -56, -81}
}
, {{36, 6, -27, -26, 8, -83, -116, -56, -117, -91, 13, -17, -87, -46, -93, -23}
, {-57, -53, 56, -38, -49, -2, -134, -68, -38, -93, 29, -32, -38, -83, -62, -34}
, {45, -47, -54, -48, 42, -17, -177, -82, -32, -81, -47, 15, -36, 53, -116, -3}
}
, {{93, -126, -62, 17, 219, -70, -137, 178, -119, 47, -151, 121, -84, -217, -23, 57}
, {-33, -87, -28, -225, 224, -116, -15, 165, -143, -15, -134, 35, -37, -103, 97, 58}
, {139, -196, 46, -219, 52, -67, -78, 146, -115, 26, -68, 28, 89, -52, -34, -11}
}
, {{74, 35, 38, -28, -74, -27, -95, -29, 39, 117, 3, 26, -39, 13, -29, -215}
, {35, 33, 48, -43, 62, -31, 48, 20, -5, 67, 66, 73, -1, -8, 1, -114}
, {-23, -39, 2, -11, -80, -33, -6, 45, 5, 31, 25, 78, -2, -73, -14, -159}
}
, {{-17, 91, 36, 46, 90, -91, 66, -159, -25, -208, -92, -96, 83, 11, -27, 9}
, {61, -88, -57, 178, 84, -8, 169, -81, -138, 70, -130, -10, 154, -32, -21, 111}
, {60, -168, -222, 93, -187, 212, 118, -230, -90, 81, -87, 4, 24, -95, 123, 50}
}
, {{-36, 1, 75, 31, -3, 28, 27, -136, 8, -126, -66, -76, -150, -34, -71, -134}
, {-24, 67, -28, 41, -53, -46, 114, -23, 32, -78, 50, -4, -104, -60, 2, -123}
, {22, 33, 82, -29, 47, -28, -28, 24, 44, 60, -74, -70, -25, 37, -94, -204}
}
, {{109, -177, -7, 227, 27, 7, 91, -250, -114, -145, -69, 266, -159, -139, -267, -357}
, {-112, 162, 80, -9, 40, 234, 88, -132, 186, -54, -74, 60, -208, -154, 106, -53}
, {-213, 203, 22, -155, 86, -290, 37, -120, -29, 104, 77, -277, -86, 25, 111, 437}
}
, {{13, -55, -87, -63, 132, 249, 206, -295, 199, 85, 156, 52, -265, -110, 172, -224}
, {-156, 88, 32, -126, 78, 107, 139, -81, -60, 230, 114, -240, -78, 99, 111, 311}
, {-132, -72, -16, -31, 141, -285, -197, -76, 152, -182, 275, -363, -56, -91, -242, 145}
}
, {{-115, -6, 40, 100, 76, -97, -84, -108, 112, -19, 21, -26, -93, -123, -54, -59}
, {66, -103, -90, 132, -35, -91, 140, -105, -55, 56, -84, 52, 81, 125, -23, 128}
, {-19, -30, -31, 158, -39, -3, 76, 3, 1, 201, -110, 58, 16, 37, 23, 89}
}
, {{-65, -72, 158, -78, 121, -46, -68, -49, -175, 92, -255, -36, -50, 112, -124, -44}
, {-187, 4, 60, -49, 131, -35, 5, -151, -149, -68, -180, -76, 52, -58, -66, -21}
, {-133, 6, 83, 2, 139, 13, -62, 34, -67, -106, -97, 20, 66, -28, -18, -75}
}
, {{75, -190, 93, -332, -196, 300, 31, -51, 39, -194, 138, 108, -282, -254, -327, -292}
, {185, -302, 49, 134, -247, 92, 178, 77, -120, -41, -80, 92, -33, 4, -80, 340}
, {-167, -187, 10, 221, -233, -254, 70, 204, 26, 163, -182, -38, 274, 150, 12, -6}
}
, {{-63, -169, 275, 13, 183, -71, -110, 30, 109, -171, 87, 15, -71, -313, -297, 121}
, {32, -23, 245, -243, -13, 134, 217, -167, -139, 36, 52, 201, -243, 42, -161, -150}
, {96, -148, 34, -167, -34, 139, 240, 48, -229, 126, -31, 259, -297, 102, 298, -11}
}
}
;

#undef INPUT_CHANNELS
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_GROUPS
/**
  ******************************************************************************
  * @file    conv1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _CNN_LAYERS_CONV3_H_
#define _CNN_LAYERS_CONV3_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      32
#define INPUT_SAMPLES       47
#define CONV_FILTERS        32
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         2

#define ZEROPADDING_LEFT    1
#define ZEROPADDING_RIGHT   1

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

typedef int16_t cnn_layers_conv3_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void cnn_layers_conv3(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const number_t kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS],  // IN

  const number_t bias[CONV_FILTERS],						                          // IN

  number_t output[CONV_OUTSAMPLES][CONV_FILTERS]);                       // OUT
#endif

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_STRIDE
#undef ZEROPADDING_LEFT
#undef ZEROPADDING_RIGHT
#undef CONV_OUTSAMPLES

#endif//_CNN_LAYERS_CONV3_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "cnn_layers_conv3.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#elif defined(WITH_NMSIS_NN)
#include "riscv_nnfunctions.h"
#endif

#define INPUT_CHANNELS      32
#define INPUT_SAMPLES       47
#define CONV_FILTERS        32
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         2
#define CONV_GROUPS         1
#define CHANNELS_PER_GROUP  (INPUT_CHANNELS / CONV_GROUPS)
#define FILTERS_PER_GROUP   (CONV_FILTERS / CONV_GROUPS)

#define ZEROPADDING_LEFT    1
#define ZEROPADDING_RIGHT   1

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

#define ACTIVATION_RELU

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 9
#define BIASES_SCALE_FACTOR 9
#define TMP_SCALE_FACTOR 9
#define INPUT_SCALE_FACTOR 9
#define OUTPUT_SCALE_FACTOR 9
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void cnn_layers_conv3(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const NUMBER_T kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS],  // IN

  const NUMBER_T bias[CONV_FILTERS],						                          // IN

  NUMBER_T output[CONV_OUTSAMPLES][CONV_FILTERS]) {                       // OUT

#if !defined(WITH_CMSIS_NN) && !defined(WITH_NMSIS_NN)
  unsigned short pos_x, z, k; 	// loop indexes for output volume
  unsigned short x;
  int input_x;
  LONG_NUMBER_T output_acc;

  for (pos_x = 0; pos_x < CONV_OUTSAMPLES; pos_x++) { 
    for (k = 0; k < CONV_FILTERS; k++) { 
      output_acc = 0;

      for (x = 0; x < CONV_KERNEL_SIZE; x++) {
        input_x = pos_x * CONV_STRIDE - ZEROPADDING_LEFT + x;

        if (input_x >= 0 && input_x < INPUT_SAMPLES) { // ZeroPadding1D
          for (z = 0; z < INPUT_CHANNELS / CONV_GROUPS; z++) {
            output_acc += (LONG_NUMBER_T)input[input_x][z + (k / FILTERS_PER_GROUP) * CHANNELS_PER_GROUP] * (LONG_NUMBER_T)kernel[k][x][z];
          }
        }
      }

    // Scale for possible additional precision of bias
    output_acc = scale(NUMBER_T, output_acc, WEIGHTS_SCALE_FACTOR - TMP_SCALE_FACTOR, OUTPUT_ROUND_MODE);

    // Scale bias to match accumulator
    output_acc += scale(NUMBER_T, (LONG_NUMBER_T)bias[k], BIASES_SCALE_FACTOR - TMP_SCALE_FACTOR - INPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);

      
#ifdef ACTIVATION_LINEAR
      output[pos_x][k] = scale_and_clamp_to(NUMBER_T, output_acc, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU) || defined(ACTIVATION_RELU6)
      // Activation function: ReLU
      if (output_acc < 0) {
        output[pos_x][k] = 0;
      } else {
#if defined(ACTIVATION_RELU6)
        if (output_acc > scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE)) {
          output_acc = scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE);
        }
#endif
        output[pos_x][k] = scale_and_clamp_to(NUMBER_T, output_acc, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      }
#else
#error "Unsupported activation function"
#endif
    }
  }

#else

#if BIASES_SCALE_FACTOR > WEIGHTS_SCALE_FACTOR
#error "CMSIS-NN does not support BIASES_SCALE_FACTOR larger than WEIGHTS_SCALE_FACTOR"
#endif

  static q15_t bufferA[2*INPUT_CHANNELS*CONV_KERNEL_SIZE];
#if INPUT_CHANNELS % 2 == 0 && CONV_FILTERS % 2 == 0 && CONV_OUTSAMPLES % 2 == 0
#ifdef WITH_CMSIS_NN
  arm_convolve_HWC_q15_fast_nonsquare(
#elif defined(WITH_NMSIS_NN)
  riscv_convolve_HWC_q15_fast_nonsquare(
#endif
#else
#ifdef WITH_CMSIS_NN
  arm_convolve_HWC_q15_basic_nonsquare(
#elif defined(WITH_NMSIS_NN)
  riscv_convolve_HWC_q15_basic_nonsquare(
#endif
#endif
                                      (q15_t*)input, //Im_in
                                      INPUT_SAMPLES, //dim_im_in_x
                                      1, //dim_im_in_y
                                      INPUT_CHANNELS, //ch_im_in
                                      (q15_t*)kernel, //wt
                                      CONV_FILTERS, //ch_im_out
                                      CONV_KERNEL_SIZE, //dim_kernel_x
                                      1, //dim_kernel_y
                                      ZEROPADDING_LEFT, //padding_x, left and right must be equal
                                      0, //padding_y
                                      CONV_STRIDE, //stride_x
                                      1, //stride_y
                                      (q15_t*)bias, //bias
                                      INPUT_SCALE_FACTOR + WEIGHTS_SCALE_FACTOR - BIASES_SCALE_FACTOR, //bias_shift
                                      INPUT_SCALE_FACTOR + WEIGHTS_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, //out_shift
                                      (q15_t*)output, //Im_out
                                      CONV_OUTSAMPLES, //dim_im_out_x
                                      1, //dim_im_out_y
                                      bufferA, //bufferA
                                      NULL //bufferB, unused
                                      );
#ifdef ACTIVATION_RELU
#ifdef WITH_CMSIS_NN
  arm_relu_q15((q15_t*)output, CONV_FILTERS * CONV_OUTSAMPLES);
#elif defined(WITH_NMSIS_NN)
  riscv_relu_q15((q15_t*)output, CONV_FILTERS * CONV_OUTSAMPLES);
#endif
#elif !defined(ACTIVATION_LINEAR)
#error "Unsupported activation with CMSIS-NN"
#endif


#endif
}

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_STRIDE
#undef CONV_GROUPS
#undef CHANNELS_PER_GROUP
#undef FILTERS_PER_GROUP
#undef ZEROPADDING_LEFT
#undef ZEROPADDING_RIGHT
#undef CONV_OUTSAMPLES
#undef ACTIVATION_RELU
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
#undef NUMBER_T
#undef LONG_NUMBER_T
/**
  ******************************************************************************
  * @file    weights/conv1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

#define INPUT_CHANNELS    32
#define CONV_FILTERS      32
#define CONV_KERNEL_SIZE  3
#define CONV_GROUPS       1


const int16_t  cnn_layers_conv3_bias[CONV_FILTERS] = {-72, 526, 454, -301, -327, -347, 574, -308, 234, -305, -388, 376, -316, -350, 494, -371, -212, 555, -272, 3, 444, -593, 317, -502, -320, -252, -272, 228, 240, 106, -292, -557}
;

const int16_t  cnn_layers_conv3_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{120, 42, -16, 48, -96, 55, 38, -174, 70, -190, 89, 1, -215, 104, 136, 130, 90, 187, 61, 187, -151, 37, 15, -25, -145, 17, -44, -58, 75, -112, 6, 133}
, {-7, -93, 11, -13, 111, 20, -85, -106, 125, -51, 18, -33, -25, -53, 156, 146, 68, 71, 108, 129, -201, 27, -162, -1, -93, -55, -154, -41, 24, -211, 40, -24}
, {-39, -69, -55, 6, 17, 81, 22, 16, 61, -84, 29, 22, -68, -37, -43, 316, -30, 204, -46, 136, 16, -20, -77, 60, -127, 82, -77, -75, 50, -118, 12, 83}
}
, {{-98, 8, -126, -172, 82, 7, -115, 85, -47, -111, 7, 90, 108, -39, -53, -119, -123, -18, 78, -63, -46, -8, 44, -31, 34, -55, -29, -73, -46, 19, -33, -111}
, {9, -17, -96, -200, 159, 48, -25, 34, -81, -108, -59, 97, 70, -42, 24, -130, -105, -7, 27, -22, -124, -66, 105, 29, 87, -43, -97, -35, 52, 9, -46, -87}
, {-90, 93, -91, -183, 56, 65, -147, 0, -78, -70, 4, 66, 75, 24, -12, -111, -126, -14, -1, -55, -67, 23, 19, -67, 117, -92, 87, 16, 35, 66, -24, -58}
}
, {{-75, -22, -102, -73, -89, -8, -129, 98, 176, -108, 225, 20, -55, 41, -29, -7, 2, -34, 59, 76, -69, -13, 69, -141, 0, 17, -55, -77, 29, -52, -154, -111}
, {-52, -58, 28, -87, -54, 70, -103, 129, 25, -149, 97, 5, -29, -3, 96, -79, -132, -36, 36, -17, -206, -87, -58, -140, 57, -37, -108, -125, 77, 29, -115, -207}
, {-65, -39, -19, -19, -26, 142, -125, 121, 42, -136, 34, 73, -49, -3, 41, -63, -52, 30, 125, 20, -130, 15, -56, -188, -6, 41, 40, -75, 81, 83, -142, -251}
}
, {{-117, -193, 11, -71, 104, 32, 25, 50, -83, 282, -11, 130, -127, 280, -176, 127, -158, -118, -9, -98, -222, -86, 215, -69, 28, -133, 334, 179, 79, -211, 260, 183}
, {-106, -231, -79, 52, 152, -17, -23, 151, -85, 277, -35, 14, -95, 165, -117, 158, -102, 84, -67, -39, -43, -103, 115, 32, 51, -223, 240, 2, 67, -29, 180, 123}
, {145, 32, -139, 68, -50, 134, 48, -4, 194, -282, 24, 92, -22, -146, -34, 126, 3, 257, 79, 110, 89, -3, 20, 30, -157, 71, -135, -36, -220, 162, -108, -184}
}
, {{100, -97, -120, 128, -23, -147, -22, 77, 160, 154, 224, 119, -25, 228, 21, 0, 69, 97, 92, 86, 10, -12, -12, -114, 74, 13, 205, 46, 66, -152, 173, 165}
, {-139, -103, 9, -29, 163, -95, -35, 30, -111, 185, -162, 139, -103, 344, -56, 17, -183, -87, -5, -89, -161, -172, 221, -31, 29, -143, 296, 54, -43, -99, 173, 183}
, {-102, -138, -97, -55, 129, -42, 49, 6, 177, -140, 6, -49, 8, -133, -13, 144, 26, 98, 11, -81, -1, -60, 171, 57, -47, -41, 35, 54, -113, 92, -33, -111}
}
, {{0, -116, 27, -117, 32, -126, 77, 159, 51, 121, 164, 219, -49, 167, -52, 76, 6, -49, -89, -15, -189, 68, -29, -57, 118, -123, 235, 141, 3, -153, 261, 170}
, {-166, -51, -58, -9, 106, -72, -109, 56, -6, 286, -148, 171, -54, 142, -174, 83, -120, 27, -145, -12, -98, -182, 81, -25, 21, -248, 244, 114, 30, -77, 239, 96}
, {71, -86, -214, 2, 47, -15, 91, 11, 187, -208, 75, 93, -70, -136, -44, 152, 23, 254, 60, 65, 38, -85, -15, 119, -132, 19, 1, -93, -195, 88, -24, -24}
}
, {{58, -101, -49, -22, -52, 96, -153, 125, 77, -125, 88, -74, -48, -19, 34, -147, 26, 35, 111, 15, -163, 32, -14, -93, -21, 5, -78, -139, -14, -52, -21, -170}
, {-32, -46, -23, -3, -31, 19, -192, 96, -22, -146, -4, -20, -4, -72, 67, -35, -81, 22, 53, 74, -191, -78, -52, -86, 39, -104, -152, -36, -47, 15, -66, -196}
, {0, -21, -81, 23, 86, 27, -148, 94, 17, -159, 48, 48, 44, -85, 34, -44, -81, -17, -16, -16, -96, -8, -18, 48, 0, -9, -83, 63, -5, -50, -109, -131}
}
, {{-72, -122, 177, -228, 263, -13, -66, -69, -166, 329, -208, 90, -153, 259, -57, 32, -288, -234, 19, -216, -295, -6, 260, 65, 160, -367, 323, 283, 179, -41, 197, 152}
, {42, -10, -49, 18, -17, 91, -90, 56, 16, -36, -23, 180, -42, 3, -62, 150, -82, 80, 13, -58, 71, -32, 112, 48, -89, -50, 198, -63, -145, 161, 5, -174}
, {128, 103, -272, 217, -136, 1, 115, 15, 118, -228, 151, 50, 36, -106, 15, 179, 129, 335, 82, 40, 195, 70, -36, 0, -200, 71, -82, -68, -25, 89, -73, -299}
}
, {{-26, -101, -137, -36, -147, -50, -51, 176, 186, -80, 200, -40, 12, 19, 29, -24, -51, 40, 48, -69, -73, 116, 6, -80, -54, 14, -58, -141, -33, -34, -4, -47}
, {67, -35, -47, -1, -131, 24, -72, 92, 13, -106, 99, 37, -10, -24, 27, -57, 6, -91, 48, 39, -135, -30, 84, -82, 13, -73, -106, -113, 2, -54, -101, -157}
, {27, -92, 25, 7, -132, 99, -35, 83, 112, -99, 137, 52, 26, -43, 42, -33, 13, 48, 6, 62, -95, 63, 92, -134, 40, -43, -26, -39, -10, -21, -90, -140}
}
, {{123, 52, -77, 88, -158, -67, -4, 45, 203, -381, 204, -91, 122, -18, -15, -145, 112, 185, 54, 19, 80, 116, -59, 51, -16, 209, -89, -90, -38, 1, -91, -12}
, {16, -166, -103, 50, 47, -41, -10, 25, -35, 147, 3, 65, -39, 242, -9, 88, -42, -57, -134, 50, -127, -167, -43, -138, 62, -42, 206, 214, 31, -147, 213, 175}
, {-211, -177, 60, -22, 257, 103, 62, 61, 4, 327, -25, -2, -65, 184, -4, 45, -127, -114, 20, -80, -114, -240, -11, -68, 237, -156, 270, -16, 113, -166, 152, 197}
}
, {{205, -64, -261, 163, 96, -107, 71, 58, -85, 83, -50, 9, 59, 25, 94, 102, 28, 73, 18, 71, 106, 56, -119, 72, 154, 110, -106, -72, 109, -148, -124, 112}
, {96, 41, -135, 15, 225, -142, -3, -145, -108, 163, -115, 32, 189, -20, 24, 221, 179, -8, 51, 123, 32, 47, -154, -12, 97, 102, 26, 37, 42, -141, -15, 80}
, {114, -53, -265, 53, 82, -201, 119, 5, -9, 140, -190, -121, 152, 4, -20, 82, 57, 68, 26, 141, 86, 73, -208, -48, 188, 35, -64, -24, 86, -199, -7, 104}
}
, {{-168, -40, -4, 51, 180, 85, -16, -123, -116, -147, 126, 31, 133, -225, 160, -49, -239, 109, 272, 145, -313, -274, -280, 48, 163, 254, 100, 15, -17, 20, -56, -49}
, {-28, 32, -166, -69, 279, 45, -149, -25, -147, -121, -51, 146, -33, -126, 24, -131, -145, -51, 25, 140, -222, -346, -222, 22, 83, 161, 82, 13, 95, 180, -70, 63}
, {-172, 97, -119, -7, 175, -22, -132, 50, 92, -185, 103, 116, -10, -132, 134, -171, -57, -77, 143, 41, -104, -220, -296, 153, 25, 50, 262, 73, 85, -26, 87, 121}
}
, {{179, -95, -105, 242, -133, -6, 167, -112, 195, -371, 235, -188, 68, 80, -97, -110, 159, 177, 95, 64, 73, -34, 38, -5, 49, 123, -111, -51, -170, -37, -108, -64}
, {70, -197, -70, 61, 31, -48, -53, -92, -49, 59, 47, 55, -13, 267, -144, 8, -69, 0, -58, 79, -160, -136, -72, -7, 47, 71, 200, 144, 120, -107, 138, 88}
, {-150, -217, 95, -94, 208, -12, 21, 63, -19, 304, 1, 221, -10, 276, -62, -19, -131, -142, 13, -93, -157, -270, 239, -13, 220, -48, 255, 81, 57, -140, 34, 252}
}
, {{127, -54, -234, 169, -86, -52, -65, 197, 117, 192, 201, -124, 56, 218, -99, 2, 57, -13, 9, 168, -25, 5, -161, -6, 74, 68, 339, 43, 188, -174, 143, 130}
, {-147, -179, 71, 45, 98, -68, -41, 75, -61, 194, -89, -19, -38, 368, -133, 48, -92, -9, 4, -41, -174, -218, 41, -25, 90, -70, 313, 139, 22, -149, 170, 206}
, {6, -10, 12, -119, 88, 17, 42, -35, 122, -136, 30, 162, 33, -201, -94, 132, -64, 25, 58, -11, -45, -116, 91, 37, -129, -172, 150, -110, -67, 30, 30, -52}
}
, {{-30, -53, -37, 45, -227, -5, -71, 69, 187, -95, 213, -16, -71, 17, 7, -143, -48, -44, 72, 44, -97, 20, 65, -143, 33, -43, 19, -205, 48, 0, -10, -144}
, {20, -32, 8, 8, -209, -56, -73, 37, 122, -138, 19, 6, 10, -5, -4, -111, 35, -11, -23, 67, -184, -68, 74, -76, 21, -144, -111, -189, 91, -9, -49, -163}
, {-23, -104, -63, 26, -81, 92, -124, 47, 116, -117, 167, 41, -40, -32, 122, -68, -33, -23, 44, 105, -159, 34, 47, -26, -34, -47, -15, -105, 120, 17, -115, -219}
}
, {{31, -51, -79, -67, 9, -188, -22, 137, 17, 169, 54, 175, -78, 176, -39, 92, -49, -34, -22, -36, -132, 75, 23, -180, 114, -59, 295, 207, 141, -194, 224, 126}
, {-40, -139, -79, 45, 181, 19, -50, 150, -62, 293, -80, 186, -111, 226, -57, 156, -182, -67, -44, -179, -68, -247, 202, -44, -116, -99, 243, -19, 75, -160, 166, 159}
, {135, -96, -93, 65, 64, 15, 57, 23, 210, -264, 34, 14, -18, -127, -64, 106, -33, 265, 13, 51, 10, -54, 43, 46, -195, 36, 28, -22, -153, 148, -8, -89}
}
, {{195, -39, -212, 107, -143, -13, 25, -70, 124, -331, 197, -138, 102, -55, -138, -30, 102, 124, 47, 78, -48, 17, -113, -57, -110, 80, 84, -69, 2, 29, -36, 68}
, {-7, -2, -132, 110, 74, -164, -23, 42, 70, 118, 38, -5, -81, 243, 9, 34, -46, -64, -21, 82, -81, -142, 5, -57, 107, 72, 226, 173, 89, -189, 185, 81}
, {-163, -180, 70, -155, 243, -29, 31, 199, -95, 313, 15, 101, -84, 202, -21, 96, -106, -87, 73, -101, -158, -223, -10, -96, 22, -210, 267, 74, 70, -177, 186, 179}
}
, {{18, -71, -87, -113, -153, -119, -88, 178, 268, -113, 139, 84, 27, 3, -16, -121, 71, -82, -46, -6, -198, -1, 70, -133, 71, 97, -36, -253, 44, -64, -14, -128}
, {31, -45, -13, -112, -224, 89, -87, 15, 63, -148, 38, 47, -49, 17, 11, -99, -125, 32, 60, -23, -89, -57, 54, -88, 104, 19, -106, -173, 154, -93, -37, -230}
, {5, -77, -111, -130, -164, 19, -96, 75, 97, -85, 170, 166, -136, -31, 56, 15, -78, 27, 39, 68, -112, -73, 17, -146, -11, 43, -5, -84, 43, 81, -34, -274}
}
, {{-172, 82, 14, 64, 170, -29, 53, 3, -86, 16, 225, -55, 234, -80, 136, -41, -300, 20, 171, 17, -162, -212, -142, -12, 12, 21, 36, 120, -48, 17, -94, 92}
, {-151, 95, -22, 111, 296, -178, 36, 136, -146, -4, 197, -19, 136, -66, 94, -95, -236, -74, 252, -7, -130, -248, -173, 130, 80, 157, 80, 60, -86, -17, -27, 93}
, {23, 40, 11, 118, 187, -126, 90, 133, 7, 56, 144, 57, -12, -36, 46, -53, -208, -125, 207, 99, -101, -222, -83, 73, -47, 43, 117, -6, -33, 27, 54, 110}
}
, {{-148, 16, 2, -36, -85, 23, -16, 138, 115, -100, 112, -32, 33, 14, -69, -130, -74, 65, 91, 10, -77, 74, 86, -22, -7, -19, 23, -31, 41, 53, -104, -126}
, {-152, -2, -66, -130, 7, 101, 1, 177, 49, -23, 177, -47, -17, -10, -14, -2, -74, -13, 88, -77, -91, 37, -46, -98, 61, -61, -72, -53, 41, 123, -92, -145}
, {-35, -63, -101, -121, 27, 116, -27, 172, 98, -49, 165, -11, -47, 60, -39, -45, -70, 62, 245, -73, -63, -85, 0, -37, -2, -4, 68, 32, -48, 79, -80, -184}
}
, {{145, -73, -177, 20, -104, -3, 3, 121, 33, -124, 72, -4, 57, -86, -14, -99, 53, 72, -61, 36, -126, 92, -96, -32, 53, -74, -119, -145, -12, 9, -25, 61}
, {-12, -87, -136, -94, 48, -39, -97, 23, 62, -115, -33, 49, 93, -61, -47, 2, 2, -25, -66, 74, -103, -69, -100, -72, 48, 4, -183, -33, -11, -9, -39, -136}
, {6, -55, -221, -13, -24, -8, -176, 63, 66, -119, -67, 75, 17, -77, 22, 57, 24, 98, -12, 99, -126, 3, -77, -88, 36, 40, -56, -56, 61, 84, -55, -98}
}
, {{89, -64, -197, 170, -108, -160, 142, 81, 202, -47, 149, -26, 41, 144, -25, -4, 165, 131, 48, 154, 30, -16, -7, -19, 94, 107, 169, 11, 85, -57, 122, 94}
, {-89, -185, -85, -24, 123, -144, -20, 163, 45, 196, -2, 110, -47, 272, -77, 80, -167, -68, 0, -47, -153, -97, 22, -67, 115, -77, 280, 186, 89, -195, 177, 160}
, {-31, -167, -26, -175, 278, 106, 22, 8, 19, 146, 66, 65, -38, -8, -5, 152, -145, -8, 80, 95, -130, -28, 72, 33, -94, -125, 234, 10, -67, -7, 201, 103}
}
, {{141, -24, -227, -16, -36, 22, -97, -51, 84, -115, 4, -36, 29, -79, -42, -59, 133, 45, 3, 18, -181, 4, -37, -63, 107, 63, -162, -114, -118, 29, -142, 37}
, {-10, 41, -185, 17, 103, 58, -36, -67, 94, -198, -73, -56, -40, -148, 21, -74, -40, 11, -26, 8, -158, 38, -45, -70, 21, -115, -108, 13, -46, -127, -32, 51}
, {108, -74, -144, -30, 195, -2, -78, 91, 94, -71, -38, 42, 76, -125, -26, 204, 57, 142, -42, 77, -121, -17, -18, 23, -1, -46, -89, 51, 16, 14, -79, 85}
}
, {{-57, -117, 78, -131, 191, 5, -39, 60, -73, 234, -63, 275, -3, 250, 114, 64, -187, -110, 40, -132, -189, -84, 93, -111, 127, -242, 376, 204, 157, -179, 325, 264}
, {-60, -181, -22, -2, 58, -37, -17, 126, -149, 244, -150, 84, -135, -62, -43, 175, -86, 164, 44, -90, -13, -51, 182, 87, -91, -146, 274, -7, 60, 49, 123, 23}
, {55, -61, -176, 149, -68, 91, 112, 6, 250, -269, 128, 92, 18, -56, 19, 196, 17, 269, 156, 213, 64, 30, -124, 69, -138, 114, 16, -64, -240, 123, -97, -124}
}
, {{83, 54, -160, 141, 181, 16, 43, -74, -38, 40, -50, -60, 104, -74, -26, 53, 134, 118, -24, 135, 52, 128, -112, -20, -2, 48, -151, -33, 11, -184, -51, 141}
, {191, 76, -172, 81, 147, 6, 35, -40, -41, 41, -191, 23, 84, -25, 137, 109, 122, 11, 34, 86, 70, 93, -221, 25, 107, 58, 72, -36, -67, -236, -1, 116}
, {98, -92, -213, 11, 155, -93, 68, -68, 69, 45, -166, -47, 120, 3, -13, 192, -1, 53, -77, 162, 79, -23, -214, 69, 49, 9, -92, -13, 55, -56, -30, 75}
}
, {{185, -11, -105, 200, 100, -114, 67, -107, -100, 103, -21, 18, 55, 40, 75, 101, 71, -6, 23, 89, 86, 106, -83, 19, 106, -14, -107, -114, 82, -202, -132, 53}
, {148, 65, -124, 164, 80, -117, 46, -22, -18, 51, -67, 50, -51, -4, 133, 92, 123, -9, -22, 156, -53, -67, -166, 88, 137, -57, 12, -69, 129, -217, -141, 131}
, {178, -14, -137, 98, 117, -118, 89, 42, 57, 69, -126, -62, 22, 72, 111, 52, -29, 100, 9, 113, 112, -9, -209, -7, 63, 95, -1, -103, 98, -178, -27, 50}
}
, {{166, -47, -247, 122, -21, -40, 44, 29, 183, 186, 107, -30, -79, 246, -45, 33, -92, -11, 12, 34, -56, -39, 94, -96, 176, 53, 302, 92, 123, -168, 190, 49}
, {-175, -225, -21, -85, 131, -102, 7, 52, -70, 226, -105, 55, -27, 247, 20, 97, -119, -11, -59, -41, -170, -195, 264, -20, -44, -69, 203, 69, 89, -52, 195, 182}
, {-50, -185, -67, 23, 155, -13, 137, -45, 202, -223, 28, 46, -64, -76, -55, 29, -51, 117, 73, -37, -59, -93, 129, 24, -83, 24, -49, -26, -143, 30, -26, 11}
}
, {{-119, 71, -114, -130, 80, -16, -25, 92, -93, -55, -65, 25, 52, -4, -105, -7, -55, -27, -7, -63, -115, 57, -22, -29, 148, -57, 9, -24, 25, 92, 21, -33}
, {-46, -5, -74, -110, 52, -3, -147, 55, -67, 18, -116, 45, 122, -27, -19, -127, -84, -35, 15, -43, -12, -34, 56, -6, 285, 23, -50, -40, 15, 58, -83, -61}
, {-29, 76, -107, -169, 65, 5, 6, 112, 9, -15, 13, -45, 109, 48, -26, -203, -98, 19, -32, -114, -33, 55, 87, 26, 195, -67, -9, 38, 53, 48, 25, -23}
}
, {{74, -58, -31, -13, -24, 52, -42, 110, 58, -166, 171, -19, -29, 2, -35, -38, -35, -20, 81, -10, -40, 56, -9, -70, 17, 30, -123, -11, -57, -7, -127, -89}
, {-31, -41, -22, -32, -22, 74, -155, 78, 27, -72, 58, -49, 8, -36, 26, 20, -11, -63, 93, -45, -56, -26, 37, -37, 36, -39, -127, -80, -47, 49, -32, -113}
, {-25, -96, -69, -60, 30, 77, -63, 50, 97, -105, 122, -11, -18, -12, 44, 18, -95, 5, 5, 43, -35, -18, -5, -57, -38, -23, -18, -2, -10, 26, -106, -164}
}
, {{133, -104, -45, 164, -138, 36, -3, 50, 209, 29, 2, 74, -224, 36, 95, 75, 172, 209, 65, 102, -137, 95, -111, 49, -172, 47, -38, -171, 22, -162, 22, -13}
, {-14, -47, 47, 37, -23, 165, 16, -64, 70, -28, 70, -3, -142, -112, 127, 46, -23, 64, -39, 167, -34, -6, -98, -63, -9, 2, -27, -156, 124, -134, -39, -80}
, {9, -104, -97, 138, -163, 93, 38, -49, 65, -41, -85, 95, -184, -105, -27, 140, 30, 129, -90, 163, 27, -138, -113, 45, -29, 10, -101, -21, 76, -235, -90, 22}
}
, {{116, -39, -116, 54, -114, -84, -12, 122, -5, 162, 200, -80, 21, 211, -79, 28, 3, 80, 74, -44, -14, 94, 123, -96, 35, 101, 176, 105, 186, -182, 185, 130}
, {-181, -97, -11, -69, 201, -99, -108, 111, -183, 235, -26, 43, -181, 321, -40, 72, -135, -52, -140, -71, -163, -85, 174, 3, 99, -138, 257, 137, 54, -166, 245, 238}
, {-27, -141, -126, -26, 129, 64, 87, 81, 184, -120, -4, 77, -10, -224, -76, 109, -59, 121, 65, 36, -55, -110, 84, 41, -253, -79, 78, -30, -75, -24, 52, -124}
}
, {{155, -20, 36, 238, -191, 63, 2, 77, 210, -421, 288, -54, 138, -62, -122, -136, 231, 319, 104, 92, 134, 38, -102, 94, -68, 119, -94, -139, -127, 67, -202, -75}
, {92, -26, 24, 100, 26, -43, -109, -38, -110, -9, 150, -1, 11, 257, -80, 31, -9, -97, 85, -25, 31, 16, -51, -19, 112, 137, 245, 210, 8, -185, 211, 106}
, {-147, -176, 112, -47, 239, 11, 32, 131, -142, 386, -30, 180, -83, 209, -76, -81, -123, -317, 0, -121, -307, -165, 24, 17, 124, -163, 256, 130, 101, -140, 155, 270}
}
}
;

#undef INPUT_CHANNELS
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_GROUPS
/**
  ******************************************************************************
  * @file    averagepool1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    21 april 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _CNN_LAYERS_POSTPOOL_H_
#define _CNN_LAYERS_POSTPOOL_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS  32
#define INPUT_SAMPLES   24
#define POOL_SIZE       24
#define POOL_STRIDE     24
#define POOL_PAD        0 // Unsupported
#define POOL_LENGTH	    ( ( (INPUT_SAMPLES - POOL_SIZE + (2*POOL_PAD) ) / POOL_STRIDE ) + 1 )

typedef int16_t cnn_layers_postpool_output_type[POOL_LENGTH][INPUT_CHANNELS];

#if 0
void cnn_layers_postpool(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS], 	    // IN
  number_t output[POOL_LENGTH][INPUT_CHANNELS]); 	// OUT
#endif

#undef INPUT_CHANNELS  
#undef INPUT_SAMPLES
#undef POOL_SIZE
#undef POOL_STRIDE
#undef POOL_PAD
#undef POOL_LENGTH

#endif//_CNN_LAYERS_POSTPOOL_H_
/**
  ******************************************************************************
  * @file    averagepool.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "cnn_layers_postpool.h"
#include "number.h"
#endif

#define INPUT_CHANNELS  32
#define INPUT_SAMPLES   24
#define POOL_SIZE       24
#define POOL_STRIDE     24
#define POOL_PAD        0 // Unsupported
#define POOL_LENGTH	    ( ( (INPUT_SAMPLES - POOL_SIZE + (2*POOL_PAD) ) / POOL_STRIDE ) + 1 )

#define ACTIVATION_LINEAR

// For fixed point quantization
#define INPUT_SCALE_FACTOR 9
#define OUTPUT_SCALE_FACTOR 9
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void cnn_layers_postpool(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS], 	    // IN
  NUMBER_T output[POOL_LENGTH][INPUT_CHANNELS]) {	// OUT

  unsigned short pos_x, k; 	// loop indexes for output volume
  unsigned int x;
  LONG_NUMBER_T avg, tmp;

  for (k = 0; k < INPUT_CHANNELS; k++) 
    for (pos_x = 0; pos_x < POOL_LENGTH; pos_x++) {
      tmp = 0;
      for (x = 0; x < POOL_SIZE; x++) {
        tmp += input[(pos_x*POOL_STRIDE)+x][k];
      }
#ifdef ACTIVATION_RELU
      if (tmp < 0) {
        tmp = 0;
      }
#elif !defined(ACTIVATION_LINEAR)
#error "Unsupported activation function"
#endif
      avg = tmp / POOL_SIZE;

      output[pos_x][k] = scale_and_clamp_to(NUMBER_T, avg, INPUT_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
    }
}

#undef INPUT_CHANNELS  
#undef INPUT_SAMPLES
#undef POOL_SIZE
#undef POOL_STRIDE
#undef POOL_PAD
#undef POOL_LENGTH
#undef ACTIVATION_LINEAR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
/**
  ******************************************************************************
  * @file    flatten.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _CNN_LAYERS_FLATTEN_H_
#define _CNN_LAYERS_FLATTEN_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define OUTPUT_DIM 32

typedef int16_t cnn_layers_flatten_output_type[OUTPUT_DIM];

#if 0
void cnn_layers_flatten(
  const number_t input[1][32], 			      // IN
	number_t output[OUTPUT_DIM]); 			                // OUT
#endif

#undef OUTPUT_DIM

#endif//_CNN_LAYERS_FLATTEN_H_
/**
  ******************************************************************************
  * @file    flatten.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 2.0.0
  * @date    26 november 2021
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "cnn_layers_flatten.h"
#include "number.h"
#endif

#define OUTPUT_DIM 32

#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t

static inline void cnn_layers_flatten(
  const NUMBER_T input[1][32], 			      // IN
	NUMBER_T output[OUTPUT_DIM]) {			                // OUT

  NUMBER_T *input_flat = (NUMBER_T *)input;

  // Copy data from input to output only if input and output don't point to the same memory address already
  if (input_flat != output) {
    for (size_t i = 0; i < OUTPUT_DIM; i++) {
      output[i] = input_flat[i];
    }
  }
}

#undef OUTPUT_DIM
#undef NUMBER_T
#undef LONG_NUMBER_T
/**
  ******************************************************************************
  * @file    fc.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _CNN_LAYERS_FC1_H_
#define _CNN_LAYERS_FC1_H_

#ifndef SINGLE_FILE
#include "number.h"
#include <stdint.h>
#endif

#define INPUT_SAMPLES 32
#define FC_UNITS 2

typedef int16_t cnn_layers_fc1_output_type[FC_UNITS];

#if 0
void cnn_layers_fc1(
  const number_t input[INPUT_SAMPLES], 			      // IN
	const number_t kernel[FC_UNITS][INPUT_SAMPLES],  // IN

	const number_t bias[FC_UNITS],			              // IN

	number_t output[FC_UNITS]); 			                // OUT
#endif

#undef INPUT_SAMPLES
#undef FC_UNITS

#endif//_CNN_LAYERS_FC1_H_
/**
  ******************************************************************************
  * @file    fc.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "cnn_layers_fc1.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#elif defined(WITH_NMSIS_NN)
#include "riscv_nnfunctions.h"
#endif

#define INPUT_SAMPLES 32
#define FC_UNITS 2
#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 9
#define BIASES_SCALE_FACTOR 9
#define TMP_SCALE_FACTOR 9
#define INPUT_SCALE_FACTOR 9
#define OUTPUT_SCALE_FACTOR 9
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void cnn_layers_fc1(
  const NUMBER_T input[INPUT_SAMPLES], 			      // IN
	const NUMBER_T kernel[FC_UNITS][INPUT_SAMPLES],  // IN

	const NUMBER_T bias[FC_UNITS],			              // IN

	NUMBER_T output[FC_UNITS]) {			                // OUT

#if !defined(WITH_CMSIS_NN) && !defined(WITH_NMSIS_NN)
  unsigned short k, z; 
  LONG_NUMBER_T output_acc;

  for (k = 0; k < FC_UNITS; k++) { 
    output_acc = 0;
    for (z = 0; z < INPUT_SAMPLES; z++) 
      output_acc = output_acc + ((LONG_NUMBER_T)kernel[k][z] * (LONG_NUMBER_T)input[z]);

    output_acc = scale(NUMBER_T, output_acc, WEIGHTS_SCALE_FACTOR - TMP_SCALE_FACTOR, OUTPUT_ROUND_MODE);

    output_acc += scale(NUMBER_T, (LONG_NUMBER_T)bias[k], BIASES_SCALE_FACTOR - TMP_SCALE_FACTOR - INPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);


    // Activation function
#ifdef ACTIVATION_LINEAR
    // Linear (MEANS NONE)
    output[k] = scale_and_clamp_to(NUMBER_T, output_acc, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU) || defined(ACTIVATION_RELU6)
    // ReLU
    if (output_acc < 0) {
      output[k] = 0;
    } else {
#if defined(ACTIVATION_RELU6)
      if (output_acc > scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE)) {
        output_acc = scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE);
      }
#endif
      output[k] = scale_and_clamp_to(NUMBER_T, output_acc, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
    }
#else
#error "Unsupported activation function"
#endif
  }
#else

#if BIASES_SCALE_FACTOR > WEIGHTS_SCALE_FACTOR
#error "CMSIS-NN does not support BIASES_SCALE_FACTOR larger than WEIGHTS_SCALE_FACTOR"
#endif

  static q15_t bufferA[INPUT_SAMPLES];
#ifdef WITH_CMSIS_NN
  arm_fully_connected_q15(
#elif defined(WITH_NMSIS_NN)
  riscv_fully_connected_q15(
#endif
                             (q15_t*)input,
                             (q15_t*)kernel,
                             INPUT_SAMPLES,
                             FC_UNITS,
                             INPUT_SCALE_FACTOR + WEIGHTS_SCALE_FACTOR - BIASES_SCALE_FACTOR,
                             INPUT_SCALE_FACTOR + WEIGHTS_SCALE_FACTOR - OUTPUT_SCALE_FACTOR,
                             (q15_t*)bias,
                             (q15_t*)output,
                             (q15_t*)bufferA);
#ifdef ACTIVATION_RELU
#ifdef WITH_CMSIS_NN
  arm_relu_q15((q15_t*)output, FC_UNITS);
#elif defined(WITH_NMSIS_NN)
  riscv_relu_q15((q15_t*)output, FC_UNITS);
#endif
#elif !defined(ACTIVATION_LINEAR)
#error "Unsupported activation with CMSIS-NN"
#endif


#endif
}

#undef INPUT_SAMPLES
#undef FC_UNITS
#undef ACTIVATION_LINEAR
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
#undef NUMBER_T
#undef LONG_NUMBER_T
/**
  ******************************************************************************
  * @file    weights/fc.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

#define INPUT_SAMPLES 32
#define FC_UNITS 2


const int16_t cnn_layers_fc1_bias[FC_UNITS] = {-52, 67}
;

const int16_t cnn_layers_fc1_kernel[FC_UNITS][INPUT_SAMPLES] = {{-121, -138, -71, 110, 75, 174, -69, 207, -127, 209, -130, -83, 232, 220, -5, 247, 204, -45, -173, -20, -146, 157, -63, 117, -68, -125, 210, -151, -86, -49, 141, 248}
, {80, 139, 45, -272, -192, -219, 130, -199, 61, -230, 23, 155, -222, -131, 139, -163, -195, 125, 108, 115, 24, -170, 83, -239, 104, 29, -200, 71, 33, 146, -242, -163}
}
;

#undef INPUT_SAMPLES
#undef FC_UNITS
/**
  ******************************************************************************
  * @file    model.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    08 july 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */


#ifdef __cplusplus
extern "C" {
#endif

#ifndef __MODEL_H__
#define __MODEL_H__

#ifndef SINGLE_FILE
#include "number.h"

 // InputLayer is excluded
#include "cnn_layers_conv1.h" // InputLayer is excluded
#include "cnn_layers_conv2.h" // InputLayer is excluded
#include "cnn_layers_conv3.h" // InputLayer is excluded
#include "cnn_layers_postpool.h" // InputLayer is excluded
#include "cnn_layers_flatten.h" // InputLayer is excluded
#include "cnn_layers_fc1.h"
#endif

#define MODEL_NAME cnn


#define MODEL_INPUT_DIM_0 186
#define MODEL_INPUT_DIM_1 10
#define MODEL_INPUT_DIMS 186 * 10

#define MODEL_OUTPUT_SAMPLES 2

#define MODEL_INPUT_SCALE_FACTOR 9 // scale factor of InputLayer
#define MODEL_INPUT_ROUND_MODE ROUND_MODE_FLOOR
#define MODEL_INPUT_NUMBER_T int16_t
#define MODEL_INPUT_LONG_NUMBER_T int32_t

#define MODEL_OUTPUT_SCALE_FACTOR 9 // scale factor of last layer
#define MODEL_OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define MODEL_OUTPUT_NUMBER_T int16_t
#define MODEL_OUTPUT_LONG_NUMBER_T int32_t

// node 0 is InputLayer so use its output shape as input shape of the model
// typedef  input_t[186][10];
typedef int16_t input_t[186][10];
typedef cnn_layers_fc1_output_type output_t;


void cnn(
  const input_t input,
  output_t output);

void reset(void);

#endif//__MODEL_H__


#ifdef __cplusplus
} // extern "C"
#endif


/**
  ******************************************************************************
  * @file    model.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SINGLE_FILE
#include "number.h"
#include "model.h"
// #include <chrono>
 // InputLayer is excluded
#include "cnn_layers_conv1.c"
#include "weights/cnn_layers_conv1.c" // InputLayer is excluded
#include "cnn_layers_conv2.c"
#include "weights/cnn_layers_conv2.c" // InputLayer is excluded
#include "cnn_layers_conv3.c"
#include "weights/cnn_layers_conv3.c" // InputLayer is excluded
#include "cnn_layers_postpool.c" // InputLayer is excluded
#include "cnn_layers_flatten.c" // InputLayer is excluded
#include "cnn_layers_fc1.c"
#include "weights/cnn_layers_fc1.c"
#endif

void cnn(
  const input_t input,
  cnn_layers_fc1_output_type cnn_layers_fc1_output) {
  
  // Output array allocation
  static union {
    cnn_layers_conv1_output_type cnn_layers_conv1_output;
    cnn_layers_conv3_output_type cnn_layers_conv3_output;
  } activations1;

  static union {
    cnn_layers_conv2_output_type cnn_layers_conv2_output;
    cnn_layers_postpool_output_type cnn_layers_postpool_output;
    cnn_layers_flatten_output_type cnn_layers_flatten_output;
  } activations2;


// Model layers call chain 
  
  
  cnn_layers_conv1( // Model input is passed as model parameter
    input,
    cnn_layers_conv1_kernel,
    cnn_layers_conv1_bias,
    activations1.cnn_layers_conv1_output
    );

  
  
  
  cnn_layers_conv2(
    activations1.cnn_layers_conv1_output,
    cnn_layers_conv2_kernel,
    cnn_layers_conv2_bias,
    activations2.cnn_layers_conv2_output
    );

  
  
  
  cnn_layers_conv3(
    activations2.cnn_layers_conv2_output,
    cnn_layers_conv3_kernel,
    cnn_layers_conv3_bias,
    activations1.cnn_layers_conv3_output
    );

  
  
  
  cnn_layers_postpool(
    activations1.cnn_layers_conv3_output,
    activations2.cnn_layers_postpool_output
    );

  
  
  
  cnn_layers_flatten(
    activations2.cnn_layers_postpool_output,
    activations2.cnn_layers_flatten_output
    );

  
  
  
  cnn_layers_fc1(
    activations2.cnn_layers_flatten_output,
    cnn_layers_fc1_kernel,
    cnn_layers_fc1_bias,// Last layer uses output passed as model parameter
    cnn_layers_fc1_output
    );

  

}

#ifdef __cplusplus
} // extern "C"
#endif
