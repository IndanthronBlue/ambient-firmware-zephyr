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
#define CONV_FILTERS        32
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
#define CONV_FILTERS      32
#define CONV_KERNEL_SIZE  3
#define CONV_GROUPS       1


const int16_t  cnn_layers_conv1_bias[CONV_FILTERS] = {-2329, 2080, 454, 2457, -1562, -382, -188, 1650, 2454, -2685, -97, -1292, 293, -289, -787, -287, -2565, -2614, 817, -1134, -1093, -3418, -2826, -2007, -1883, -2048, -2046, 286, 1728, 2708, 1349, -1060}
;

const int16_t  cnn_layers_conv1_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{24, -67, 96, -63, -10, 59, -35, 15, -8, -18}
, {10, -28, 34, -18, 38, -12, -27, 29, 47, 2}
, {34, -8, -66, 1, 57, -27, -59, 30, 11, -33}
}
, {{-29, 27, 23, -34, 20, 15, 6, -7, -8, -18}
, {-21, 32, -19, -19, 18, 7, 3, -5, 10, -26}
, {-6, 20, -10, -26, 6, 18, 0, 16, 2, -10}
}
, {{24, 132, 72, 50, 3, -30, -69, -58, 123, 171}
, {-65, 98, -48, 45, 67, -98, -131, -27, 53, 139}
, {50, -19, -33, 203, 10, -146, -4, 38, 15, 41}
}
, {{-10, 61, -31, -12, -73, 75, -50, -4, 74, 56}
, {-100, 98, -35, -9, 37, 68, 18, -43, 159, -33}
, {48, 27, -44, 12, 29, 41, 43, -54, 175, -11}
}
, {{57, 71, -102, -35, -26, 99, 94, -51, -120, 7}
, {28, 64, -63, 5, -51, 80, 36, -21, -164, -18}
, {-21, -15, -137, 72, 5, 12, -20, -6, -178, -55}
}
, {{5, 7, 22, -111, 128, 38, -111, 109, -307, 155}
, {42, -61, 140, -137, -36, 143, -211, 170, -268, 83}
, {-49, -72, 113, 116, -83, 201, -150, 92, -108, 17}
}
, {{-63, 137, -117, 60, -6, -37, 71, -60, 60, -17}
, {-7, -43, 66, -61, 57, -16, -21, 59, -41, 41}
, {67, -134, 112, -69, 32, 45, -72, 73, -38, 19}
}
, {{-74, 0, 18, 82, 108, -39, 60, -114, -69, 18}
, {13, -47, 90, -4, 146, 17, -87, 69, -184, 101}
, {6, -10, 47, 98, 37, 100, -87, 63, -158, 106}
}
, {{-54, -37, 79, -94, -4, 14, -43, 17, -162, 8}
, {16, 42, 36, -54, -69, -1, -60, 12, -75, 4}
, {-39, 32, 4, -36, -59, 1, -47, -10, -113, -16}
}
, {{37, -22, -32, 27, -13, 15, 42, -59, 41, 18}
, {33, -35, -19, 37, -26, 20, 38, -34, -2, 48}
, {11, -25, 14, 17, -24, 27, 52, -29, 3, 38}
}
, {{7, -36, 75, 115, -111, 116, 108, -56, -204, -2}
, {-44, -13, -40, 207, -75, -38, 108, 49, -358, 25}
, {44, 103, -14, 72, -128, -14, 24, 92, -307, 33}
}
, {{-72, 223, -196, 66, -70, 61, -68, -92, 78, 46}
, {8, -49, 43, -59, 81, -102, 43, -26, -32, 26}
, {99, -243, 252, -179, 197, 69, -159, 45, 3, 98}
}
, {{35, -35, 61, -41, 79, -48, 58, -91, 72, -90}
, {-16, 5, 42, -66, 91, -118, 139, -156, 106, -96}
, {-34, 30, -9, -37, 6, -63, 68, -83, 63, -21}
}
, {{-4, -8, 108, 56, -22, 3, 324, -167, 46, 117}
, {68, 19, 259, -40, 31, -159, 324, -162, 4, 77}
, {-62, -100, 139, 4, -85, -134, 168, -62, 63, -32}
}
, {{1, -10, -3, 13, 2, -6, -6, 17, 3, -12}
, {-1, -7, -8, 19, 0, -2, -15, 13, 4, -5}
, {30, -21, -15, 26, -9, -2, -12, 14, -3, -1}
}
, {{0, -22, -45, 90, -95, -65, -27, 21, -44, -35}
, {25, -21, -45, 112, -137, -36, -28, 9, -67, -15}
, {-19, 0, -50, 72, -79, 5, -77, -86, -61, -60}
}
, {{12, -15, 13, 65, -80, 3, 12, 107, -88, 67}
, {67, -50, 10, 47, -75, 13, 70, 148, -141, 147}
, {1, 17, -4, 42, -42, 7, 35, 122, -114, 127}
}
, {{45, -25, 30, 0, -4, -40, 34, 29, 21, 30}
, {-2, -13, 37, 1, -48, -107, -7, 16, 2, 29}
, {39, -14, 38, 33, -40, -89, 24, -12, -25, 3}
}
, {{-15, 32, 87, 51, 79, -9, -65, 49, -45, 2}
, {34, 40, 77, 7, 76, -2, -62, 30, -6, -39}
, {-32, 92, 59, 47, 45, 15, -47, -27, 27, -32}
}
, {{-21, -17, -81, 52, 23, -46, -110, 60, 49, 25}
, {33, 11, -117, 57, 76, 6, -136, 62, 69, 20}
, {30, 33, -90, 28, 67, 22, -88, 73, 41, 53}
}
, {{2, 74, -8, -80, 66, -88, 11, 35, -2, 126}
, {20, 28, -2, -97, 135, -145, 15, 31, -40, 68}
, {25, -22, 6, -74, 150, -150, -9, 71, -76, 92}
}
, {{85, 44, -57, 4, -6, 34, -49, -9, 62, 137}
, {51, 16, -97, -6, -7, 16, -37, -64, 141, 118}
, {-24, -65, -5, 90, -22, 78, -41, -79, 164, 133}
}
, {{38, -15, -40, -18, 31, 31, -20, 24, 45, -54}
, {52, -44, 3, -13, 30, 25, -44, 34, 47, -43}
, {-4, -26, 25, 1, 29, 27, 1, -6, 33, -49}
}
, {{65, -37, 58, -87, -31, 42, -103, 151, -13, 108}
, {-3, -32, 77, -30, -50, 117, -182, 184, -79, 82}
, {-9, -20, 36, -39, -91, 80, -89, 90, -63, 36}
}
, {{60, -4, -132, -12, 67, -6, -47, -17, 10, -2}
, {-1, -2, -45, 14, 15, 24, 4, -29, -23, 8}
, {-2, 12, -63, -15, 29, 19, -24, -26, 0, 6}
}
, {{42, -41, 9, -28, 13, 37, -18, 19, 32, -33}
, {46, -86, 20, -25, 38, 56, 4, -17, 9, -55}
, {-30, -6, -1, 13, 36, -15, 59, -63, -3, -41}
}
, {{13, -44, 113, -92, 65, -60, 41, 14, 18, 14}
, {4, -55, 123, -94, 63, -79, 67, -44, 51, 15}
, {36, -52, 82, -42, 59, -68, 52, -60, 40, -9}
}
, {{24, -51, -50, 16, 45, -74, 28, 23, -26, -42}
, {-9, -41, -59, 36, 49, -79, 59, 19, -58, -27}
, {-29, -16, -28, 2, 41, -38, 23, 27, -58, 3}
}
, {{-104, 36, -50, -48, 24, -157, 83, -140, -1, -60}
, {40, 32, -67, -109, 113, -199, 20, -35, -113, 76}
, {12, -5, -20, -103, 72, -146, -81, -14, 35, -34}
}
, {{-65, 54, -76, 89, -41, 32, -139, 17, 39, -56}
, {12, 36, -66, 77, -43, 25, -86, 2, -9, 37}
, {-26, 54, -79, 10, -65, -20, -79, -24, -29, 13}
}
, {{46, -8, -94, 71, 226, -31, 24, 77, -141, 31}
, {-53, -40, -47, 1, 152, -42, -17, 54, -158, 74}
, {-41, -77, 36, -21, 101, -31, 52, -52, -127, 24}
}
, {{44, -80, 58, 60, -100, 39, 55, -112, 65, -62}
, {-9, 1, -28, 112, -192, 142, -35, -72, 101, -69}
, {-10, 54, -76, 69, -40, 91, -69, 64, -7, -20}
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

#ifndef _CNN_LAYERS_CONV2_DW_H_
#define _CNN_LAYERS_CONV2_DW_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      32
#define INPUT_SAMPLES       93
#define CONV_FILTERS        32
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         2

#define ZEROPADDING_LEFT    1
#define ZEROPADDING_RIGHT   1

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

typedef int16_t cnn_layers_conv2_dw_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void cnn_layers_conv2_dw(
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

#endif//_CNN_LAYERS_CONV2_DW_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "cnn_layers_conv2_dw.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#elif defined(WITH_NMSIS_NN)
#include "riscv_nnfunctions.h"
#endif

#define INPUT_CHANNELS      32
#define INPUT_SAMPLES       93
#define CONV_FILTERS        32
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         2
#define CONV_GROUPS         32
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


static inline void cnn_layers_conv2_dw(
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
#define CONV_GROUPS       32


const int16_t  cnn_layers_conv2_dw_bias[CONV_FILTERS] = {-130, -133, 434, 273, -399, 480, 459, -246, -259, -364, 411, -121, -67, 549, 133, 607, -326, 504, 278, -296, 574, 442, -389, -259, -164, 553, -272, -397, 354, -285, 495, 35}
;

const int16_t  cnn_layers_conv2_dw_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{-1011}
, {1700}
, {-852}
}
, {{-1109}
, {1165}
, {200}
}
, {{-377}
, {-276}
, {-195}
}
, {{-455}
, {-89}
, {-182}
}
, {{229}
, {365}
, {374}
}
, {{-330}
, {-397}
, {-239}
}
, {{-640}
, {-829}
, {-617}
}
, {{22}
, {540}
, {1}
}
, {{314}
, {591}
, {-300}
}
, {{-972}
, {85}
, {1723}
}
, {{-296}
, {-236}
, {-333}
}
, {{-305}
, {425}
, {-509}
}
, {{750}
, {897}
, {591}
}
, {{-226}
, {-290}
, {-276}
}
, {{-2905}
, {-531}
, {3380}
}
, {{-318}
, {-386}
, {-461}
}
, {{334}
, {599}
, {151}
}
, {{-496}
, {-523}
, {-254}
}
, {{974}
, {-272}
, {-1348}
}
, {{37}
, {538}
, {391}
}
, {{298}
, {-358}
, {-809}
}
, {{-340}
, {-383}
, {-276}
}
, {{574}
, {557}
, {50}
}
, {{-37}
, {510}
, {546}
}
, {{-57}
, {2552}
, {-2453}
}
, {{-110}
, {-692}
, {-487}
}
, {{620}
, {513}
, {-160}
}
, {{-220}
, {691}
, {545}
}
, {{-449}
, {-316}
, {19}
}
, {{215}
, {595}
, {-103}
}
, {{-222}
, {-318}
, {-390}
}
, {{222}
, {691}
, {421}
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

#ifndef _CNN_LAYERS_CONV2_PW_H_
#define _CNN_LAYERS_CONV2_PW_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      32
#define INPUT_SAMPLES       47
#define CONV_FILTERS        64
#define CONV_KERNEL_SIZE    1
#define CONV_STRIDE         1

#define ZEROPADDING_LEFT    0
#define ZEROPADDING_RIGHT   0

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

typedef int16_t cnn_layers_conv2_pw_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void cnn_layers_conv2_pw(
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

#endif//_CNN_LAYERS_CONV2_PW_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "cnn_layers_conv2_pw.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#elif defined(WITH_NMSIS_NN)
#include "riscv_nnfunctions.h"
#endif

#define INPUT_CHANNELS      32
#define INPUT_SAMPLES       47
#define CONV_FILTERS        64
#define CONV_KERNEL_SIZE    1
#define CONV_STRIDE         1
#define CONV_GROUPS         1
#define CHANNELS_PER_GROUP  (INPUT_CHANNELS / CONV_GROUPS)
#define FILTERS_PER_GROUP   (CONV_FILTERS / CONV_GROUPS)

#define ZEROPADDING_LEFT    0
#define ZEROPADDING_RIGHT   0

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


static inline void cnn_layers_conv2_pw(
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
#define CONV_FILTERS      64
#define CONV_KERNEL_SIZE  1
#define CONV_GROUPS       1


const int16_t  cnn_layers_conv2_pw_bias[CONV_FILTERS] = {170, -127, -63, 165, -454, 342, 195, -149, -47, -34, 191, -108, -133, 67, -153, 10, 128, 174, 126, 87, 604, -50, 960, 424, -516, 163, 854, 550, 205, 321, 241, -198, -564, 173, -596, 111, 67, 134, 207, -219, 242, -252, 451, 179, 246, 103, 466, 555, 363, 452, 257, 223, 104, 167, 226, -832, 228, -454, 605, 844, 138, -97, -236, 250}
;

const int16_t  cnn_layers_conv2_pw_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{-94, 5, 46, 57, -11, -10, 66, 151, -120, -122, -65, -220, -151, -8, -86, -156, 187, -131, -33, -61, 36, -120, -76, -8, 3, 146, 38, 94, -22, -44, -40, 167}
}
, {{24, -217, 48, -164, 94, 257, 128, -19, -42, 20, 40, 29, -14, -281, 63, 112, 277, -108, -44, -40, -228, 151, -65, -71, 20, 106, 200, 188, -115, 159, 14, -96}
}
, {{-72, 76, -79, -38, 210, -2, -214, -308, 20, 203, -80, -129, -151, 3, 580, -71, -16, -117, 46, -48, -37, 287, -121, -8, -29, -103, 85, -28, 87, 112, 175, -61}
}
, {{43, -22, -153, 36, -21, 18, 45, 62, -16, -13, -106, 9, -26, 13, 11, -115, 64, -94, 11, 198, 4, -47, 31, -1, 40, 56, -6, 192, 1, 14, -92, 15}
}
, {{-52, -55, 161, -112, 1, -20, 55, 173, 31, 6, -299, -1, 58, 179, -33, -252, -67, 178, 160, 177, 86, -2, -48, 151, -2, -61, 5, 64, -96, -21, 14, 370}
}
, {{102, 30, 219, -119, -88, -301, -1, -106, 125, 6, -152, 34, -117, -316, 87, 242, -40, 122, -220, -104, -8, 125, -172, 169, -62, -14, 15, 105, 11, -139, 135, 12}
}
, {{-53, 92, 49, 5, -147, 212, 159, -265, -8, 157, 47, -97, -116, -147, -65, -84, -5, -44, 140, 10, -139, -15, -57, -70, 15, 112, -53, -19, 52, -179, 113, -110}
}
, {{-2, -49, -63, -169, 141, -168, -134, -72, 5, 48, -86, -97, 212, 175, 243, 41, -7, 15, 4, 56, -65, 70, 75, 3, 20, 128, 315, -114, 8, -134, -24, -152}
}
, {{135, -128, 2, 37, 76, -76, 87, -117, -96, 70, -100, -24, -87, -43, 104, 34, -61, -19, 91, 60, 66, 83, 64, -83, 167, -53, -41, 25, -35, -123, -113, -42}
}
, {{35, -194, 12, -172, -309, -23, 103, -87, 51, -123, -301, 11, 128, 29, 50, -110, 34, -105, -55, 377, 1, -89, 41, 77, 31, 260, 47, 414, -24, -1, -201, -1}
}
, {{22, -1, -69, -77, 6, 63, -2, -54, -81, -37, -96, -33, -164, 83, 2, -5, 25, 39, -2, 110, 61, -39, 58, -20, 20, 32, -105, 34, -62, 134, -32, -53}
}
, {{72, -118, -282, -40, -150, -101, 72, 75, 19, 299, -52, -347, -104, 48, 548, -2, 21, 130, 389, 111, -40, -28, 100, -77, 66, -215, -219, 59, 48, 57, -50, -88}
}
, {{-115, 21, -114, 97, 83, 148, 145, -128, -126, 164, -136, 0, 51, 121, 165, -166, 100, -27, -121, 226, -36, -83, -190, -48, -42, -21, -68, 31, 36, -51, 4, 44}
}
, {{-3, -9, 105, 128, -23, -410, -59, 110, 40, -166, -20, -60, -152, 331, 54, -91, 261, -108, -15, 156, -75, 105, -95, 325, -56, 80, -249, -97, 33, -27, 0, -128}
}
, {{-54, -242, -78, -7, -74, -98, 207, -3, -87, 62, -29, -125, -87, 37, 17, 29, -87, 202, 117, 122, 42, 46, -8, 15, -62, 109, -200, -59, -53, -88, 35, 117}
}
, {{-46, -84, 237, -55, -74, -268, -31, -5, -14, -43, -214, -19, 61, 286, -5, -198, 64, -88, 23, -11, 74, 51, 125, 222, -118, -182, -224, -76, -53, -20, 67, 235}
}
, {{46, -51, -21, 19, 68, 69, -84, -40, -118, 68, -80, 63, -86, 129, 100, -150, 70, -15, -46, 95, 57, -100, 78, 51, 91, -125, -59, 20, -39, 14, -64, 29}
}
, {{9, -64, -46, -9, -60, 28, -126, -4, -10, -3, -85, 89, -88, -61, 31, -13, 34, -77, 0, 70, 115, -10, 190, 29, -45, -209, 145, 24, 8, -224, 115, -45}
}
, {{19, 55, 167, 225, -91, -173, 115, 44, -6, -96, 93, 75, -47, -113, -195, 129, -129, -47, -55, 58, -32, -15, 42, -32, -70, -119, 160, -41, 67, 0, 19, -57}
}
, {{92, 17, -74, 78, -63, 31, 123, -232, -200, -313, -54, -41, -124, 40, -32, 89, -45, -90, 72, -45, 84, -31, 100, -61, 67, 89, 2, -119, 175, -91, 12, 27}
}
, {{-53, -253, -134, 210, -24, -10, 7, 51, 51, 82, -52, -170, -107, -34, -24, 37, -69, -87, 74, 2, -135, -177, -66, -101, -15, 5, -32, 24, 18, -109, -45, -143}
}
, {{-13, -56, -122, -96, 48, 120, 165, 91, -65, 103, -132, -30, -166, -133, 69, -76, 221, -86, 61, -32, -9, 56, -49, -176, 5, 189, -64, 110, 78, 29, -233, 5}
}
, {{59, 121, 21, -42, -186, -44, -291, 197, -129, -207, -90, 111, 14, -244, -160, 33, -92, -39, 265, -49, 70, -37, -378, -61, 37, -100, -252, -18, -246, -32, -62, 76}
}
, {{27, 27, 46, -10, -122, -101, 6, -131, 96, -55, 28, 62, 145, -26, -123, -124, 134, -276, 36, -101, -84, -92, -171, -62, -113, -105, 39, 1, -37, -257, 133, 112}
}
, {{6, 35, -108, 61, 7, 232, -121, -37, -7, 108, -65, 118, 39, 189, 233, 47, 110, -56, 41, 248, 147, 58, -161, 178, -1, -101, -58, -21, -153, 91, 181, 36}
}
, {{13, -12, 101, -96, -69, -6, -53, 55, 26, 49, 31, 1, -25, -48, 39, -166, 9, 3, -5, 37, 127, -19, 40, -15, -4, -150, 40, -38, -76, -134, 28, 190}
}
, {{44, -107, -208, -153, -21, -231, -406, 89, -54, -53, -16, 101, -112, 54, -60, 129, 161, -36, -212, 85, -151, 305, 83, 223, -7, -87, -32, 22, -222, -41, -213, -234}
}
, {{241, 42, -113, -179, -82, -59, -276, 49, -9, -17, 139, 308, -141, -210, -79, 91, -77, -81, -71, -117, -126, 83, -28, 159, 121, -215, -108, -70, -99, 90, 39, 51}
}
, {{-70, 60, 5, -1, -52, -116, -10, -69, -62, -6, -98, -160, -148, -17, 22, -97, 33, -43, 99, 60, 131, -11, 86, 94, -94, 26, -77, -94, -37, -158, 12, 112}
}
, {{147, 30, -42, -58, 25, -8, -75, -48, 139, -17, 17, -24, -52, 63, 247, -12, -140, 156, 111, -25, -264, -114, -146, 3, 212, -15, -64, -217, 59, 94, -83, -95}
}
, {{-27, -66, 51, 91, 1, 38, 115, -33, -163, -32, -23, -90, -40, 4, -71, 26, -73, 71, 0, -63, -22, -66, -48, -6, -30, 64, -24, -44, -25, -144, 49, -6}
}
, {{-15, -84, -17, -171, -134, 106, -285, 171, -161, 22, 137, 310, -29, 93, 149, 118, 106, -49, 126, 35, 95, -8, 104, 189, -143, 105, 11, -66, -270, -227, 242, 122}
}
, {{-38, -46, -15, -278, -31, 43, 0, 15, 205, 4, 20, -98, -115, 18, 106, 49, 114, 209, -30, 30, -12, 226, 90, 151, -28, 28, 172, 21, -69, 259, 55, 102}
}
, {{-254, 89, -338, -182, -71, 57, 28, 46, 12, -175, -131, -63, -39, -156, 51, 20, 81, 126, -326, 208, -112, 122, 97, 135, -372, -82, -120, 96, 78, 128, 88, 58}
}
, {{54, 38, 62, -164, 114, 268, 226, -29, -98, 313, -1, 54, -108, -186, -50, 146, -143, 1, 50, -19, -122, 101, 245, 157, -17, -132, 17, 24, -124, -23, 214, 162}
}
, {{-45, 45, -61, 122, 30, -36, 61, -72, -36, -79, -33, 14, 280, -111, 13, -96, 170, -103, 39, -31, 8, 13, -85, -55, 47, -10, 98, 13, 10, 24, -43, 43}
}
, {{191, -23, -145, -40, -177, 41, -251, 167, -10, 2, 11, 82, 243, 45, -129, -150, 138, 99, -157, -176, 18, 59, 462, 129, 6, -95, -200, 38, 79, 17, 193, -2}
}
, {{119, 163, -3, -183, -47, -11, 30, 74, -24, 71, -103, 159, 148, -57, 22, -137, -102, -99, -17, 69, -169, 100, -185, -9, 84, 88, -41, 39, 106, 62, -90, 1}
}
, {{-47, -65, 125, -171, -54, 23, -28, -22, -98, -62, 12, 13, 58, -41, 83, 14, 214, 138, -20, -43, -71, 210, -8, -86, -4, 73, -39, 161, -239, 101, -103, -301}
}
, {{-64, -52, 85, 61, 31, -7, 14, -2, 61, 42, 17, -57, -113, 70, 7, 33, 54, -88, -88, 73, 7, -22, 182, -30, -6, -79, 35, 92, 215, -163, -27, 89}
}
, {{-52, -51, -80, 54, -180, 128, -192, -60, 8, -40, -47, -13, 99, -127, 74, 55, 105, 72, 15, -121, -62, -105, 188, -9, 0, -54, 224, 31, -126, -155, 39, -143}
}
, {{98, -226, 2, -118, -95, 136, 91, 105, -179, 172, 56, 35, 200, 96, 371, -43, 49, -185, -137, 25, -339, 85, -196, -135, -39, 117, 211, 80, -147, 176, 140, 43}
}
, {{131, 6, -223, -166, 219, -205, -343, -3, -123, 3, -27, 147, -194, 217, -9, -128, 53, -186, -44, 83, 135, -98, -98, -31, 52, -266, 87, 124, -182, 69, 202, -91}
}
, {{-77, 91, 183, -75, -112, 34, -121, -11, -159, -78, 110, -47, 61, 48, -216, 163, 37, -197, 130, 70, -11, -33, 116, -63, -192, -112, 18, -90, 158, -150, -4, -230}
}
, {{71, -1, -245, 44, -32, -84, 0, -131, -225, 20, 156, -89, -44, 129, -263, 218, -31, -65, 194, 258, -55, -189, -114, -25, 130, 81, -73, -146, 11, 76, -156, -10}
}
, {{-91, -145, 29, 32, 34, -17, 99, -25, -78, -40, 19, -205, 56, -16, -130, -5, 16, -48, 16, -66, 62, 4, -18, -28, -90, 54, 39, 23, 26, -19, 90, -46}
}
, {{-76, 19, -18, 41, -25, 5, -110, 9, -59, -41, 63, -104, -13, 22, -4, 47, -71, -64, -36, -29, -65, 108, -80, -41, 1, 42, 64, 6, -108, -3, -68, -157}
}
, {{5, -61, -162, 79, 25, 185, 108, 98, -2, 107, -227, -70, -96, -277, -46, -433, 17, -377, 121, -25, -11, -26, -92, -138, 12, 100, -34, 131, -19, -107, -92, 135}
}
, {{133, -56, 49, 60, 108, -12, -17, 15, 42, -52, 215, 184, 53, -23, -173, 121, -197, -57, 28, -43, -163, 11, 125, -71, 50, -117, -21, -82, -158, -219, -53, -189}
}
, {{36, -32, -58, -3, 4, -75, -206, 22, -75, 59, 38, 97, 23, -167, 42, 2, 6, -40, -24, 22, 110, 20, 17, 37, -20, -87, 134, -18, -67, -131, -29, -73}
}
, {{26, 39, -25, -17, 142, 11, 19, -14, -98, -2, -88, -7, -51, 83, -27, 49, -112, 3, -42, 163, -12, -75, 70, -97, 24, -23, -16, 106, -5, -72, -92, -63}
}
, {{-87, -67, -3, 4, 82, 61, 227, -120, -166, -16, -95, -69, 70, 25, -142, -103, 19, 26, -56, -111, 28, -71, -128, -67, -133, 30, -128, 42, -7, 31, -30, 78}
}
, {{-48, -180, 89, 98, -74, 111, -137, 55, 56, -28, 39, -4, 84, -107, -92, 85, -7, -70, 37, -22, -88, 145, 99, -33, 41, -201, 255, -3, -77, -80, -77, -97}
}
, {{-85, -39, 98, 16, 44, 95, 149, -118, -30, 38, -79, -193, 141, 32, 133, 53, -126, 138, -89, -175, 173, 17, -23, -75, 47, -78, -86, -46, -22, -278, -120, -162}
}
, {{-49, -28, 51, 73, -61, -116, -46, 53, 39, 0, -5, -33, -119, -75, -6, 41, 57, -42, 3, 15, -66, 133, 44, 108, -11, -110, 89, -23, -50, -154, -74, 89}
}
, {{65, 97, 75, -47, 45, 146, 342, 17, 142, -72, 332, -82, 41, 4, 73, 101, -254, 151, 19, 199, -61, 30, 19, 41, 73, -162, -143, 60, 74, 98, 36, 218}
}
, {{-10, -61, -165, 47, 14, 416, 79, -117, -39, 35, 98, -55, -69, -233, 26, -284, 121, -135, 25, 133, -63, -212, -87, -288, -145, -189, 198, 37, 76, 92, 99, -148}
}
, {{-242, -164, -138, 145, -141, 68, -5, -189, 39, -239, 109, 24, -275, -64, 244, 32, 110, 57, 4, -63, 49, 284, -45, -227, -13, 101, 335, 95, 194, 351, 43, -52}
}
, {{109, -126, 247, -228, 139, 16, -376, -82, -123, 146, -98, 238, -32, -153, 201, 249, -76, -5, -197, -67, -28, -9, -214, -122, -33, -254, 38, -198, 113, -117, 71, 27}
}
, {{103, -319, -227, -39, -103, 99, -366, 139, -42, -88, 104, 249, -36, 24, -226, -252, -180, 127, -1, -139, 26, -60, 17, -15, -45, -65, 190, 155, -149, -179, 0, -33}
}
, {{6, -52, 13, -106, 12, -98, -74, 73, -6, 40, -8, 4, -59, -39, 38, -41, 38, 7, 4, 6, 78, 83, 43, 82, -26, -102, 40, -6, -75, -65, 35, 165}
}
, {{83, -104, 39, -144, -114, 177, 179, -267, -120, -116, 83, -192, -289, 69, 119, 135, -131, 168, 105, -82, -74, 49, -48, 22, -60, 11, 70, 41, -56, 65, 101, -59}
}
, {{-34, -39, 85, 80, -62, 153, 0, -109, 109, -160, 251, 17, 168, -5, -48, -35, -80, 108, 1, 101, 4, 125, 144, -43, 66, -81, -56, 131, -63, -67, -131, -136}
}
, {{-75, -96, -67, -42, -106, -2, 5, 11, 3, 12, 17, -96, 92, -17, 123, 49, 180, -6, 107, 37, -94, 1, 54, -1, -47, -83, 203, 105, -331, -267, -157, -183}
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

#ifndef _CNN_LAYERS_CONV3_DW_H_
#define _CNN_LAYERS_CONV3_DW_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      64
#define INPUT_SAMPLES       47
#define CONV_FILTERS        64
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         2

#define ZEROPADDING_LEFT    1
#define ZEROPADDING_RIGHT   1

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

typedef int16_t cnn_layers_conv3_dw_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void cnn_layers_conv3_dw(
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

#endif//_CNN_LAYERS_CONV3_DW_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "cnn_layers_conv3_dw.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#elif defined(WITH_NMSIS_NN)
#include "riscv_nnfunctions.h"
#endif

#define INPUT_CHANNELS      64
#define INPUT_SAMPLES       47
#define CONV_FILTERS        64
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         2
#define CONV_GROUPS         64
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


static inline void cnn_layers_conv3_dw(
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

#define INPUT_CHANNELS    64
#define CONV_FILTERS      64
#define CONV_KERNEL_SIZE  3
#define CONV_GROUPS       64


const int16_t  cnn_layers_conv3_dw_bias[CONV_FILTERS] = {-89, -261, -236, -69, -183, 557, -520, -23, -270, -12, 129, 579, 269, -174, -400, -24, -119, -49, 74, 779, 686, -186, 540, 473, -102, -65, -247, -149, 387, -348, -16, 554, 211, -371, -297, 231, -57, -209, -278, -388, -145, 472, -275, 487, -341, -866, -539, -63, -355, 189, 312, -515, -105, -420, -56, -293, -329, 19, 623, 732, -162, 528, 352, 132}
;

const int16_t  cnn_layers_conv3_dw_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{444}
, {-1109}
, {644}
}
, {{421}
, {605}
, {-243}
}
, {{173}
, {-74}
, {513}
}
, {{-685}
, {1664}
, {-1168}
}
, {{418}
, {-116}
, {378}
}
, {{-372}
, {-471}
, {-297}
}
, {{582}
, {611}
, {268}
}
, {{-475}
, {803}
, {-490}
}
, {{319}
, {1004}
, {972}
}
, {{526}
, {-813}
, {366}
}
, {{1720}
, {-496}
, {-1338}
}
, {{-281}
, {-480}
, {-337}
}
, {{-477}
, {365}
, {-741}
}
, {{484}
, {465}
, {-252}
}
, {{41}
, {1020}
, {628}
}
, {{-446}
, {344}
, {479}
}
, {{1194}
, {-34}
, {-834}
}
, {{-1267}
, {828}
, {772}
}
, {{-309}
, {-584}
, {903}
}
, {{-507}
, {-662}
, {-763}
}
, {{-495}
, {-619}
, {-469}
}
, {{122}
, {736}
, {438}
}
, {{-257}
, {-549}
, {-383}
}
, {{-453}
, {-626}
, {-292}
}
, {{-231}
, {518}
, {-392}
}
, {{-872}
, {-314}
, {1346}
}
, {{-407}
, {295}
, {764}
}
, {{551}
, {522}
, {-633}
}
, {{637}
, {-630}
, {-735}
}
, {{180}
, {526}
, {91}
}
, {{1903}
, {-152}
, {-1591}
}
, {{-475}
, {-439}
, {-162}
}
, {{399}
, {-736}
, {17}
}
, {{450}
, {509}
, {262}
}
, {{297}
, {592}
, {45}
}
, {{753}
, {-136}
, {-915}
}
, {{-637}
, {761}
, {-362}
}
, {{-79}
, {573}
, {-234}
}
, {{-671}
, {414}
, {807}
}
, {{586}
, {-136}
, {389}
}
, {{415}
, {-1076}
, {880}
}
, {{-339}
, {-432}
, {-391}
}
, {{380}
, {-249}
, {885}
}
, {{-546}
, {-158}
, {-377}
}
, {{411}
, {441}
, {246}
}
, {{992}
, {1120}
, {431}
}
, {{1033}
, {42}
, {388}
}
, {{1000}
, {-283}
, {-668}
}
, {{659}
, {579}
, {-360}
}
, {{-194}
, {-1163}
, {1049}
}
, {{-1283}
, {-277}
, {944}
}
, {{662}
, {641}
, {137}
}
, {{-489}
, {570}
, {533}
}
, {{625}
, {562}
, {9}
}
, {{1203}
, {303}
, {-1262}
}
, {{396}
, {225}
, {420}
}
, {{-318}
, {182}
, {858}
}
, {{235}
, {467}
, {-919}
}
, {{-513}
, {-597}
, {-282}
}
, {{-487}
, {-693}
, {-178}
}
, {{-936}
, {1921}
, {-1176}
}
, {{-694}
, {-329}
, {-195}
}
, {{-735}
, {526}
, {-916}
}
, {{-1015}
, {422}
, {324}
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

#ifndef _CNN_LAYERS_CONV3_PW_H_
#define _CNN_LAYERS_CONV3_PW_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      64
#define INPUT_SAMPLES       24
#define CONV_FILTERS        128
#define CONV_KERNEL_SIZE    1
#define CONV_STRIDE         1

#define ZEROPADDING_LEFT    0
#define ZEROPADDING_RIGHT   0

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

typedef int16_t cnn_layers_conv3_pw_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void cnn_layers_conv3_pw(
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

#endif//_CNN_LAYERS_CONV3_PW_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "cnn_layers_conv3_pw.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#elif defined(WITH_NMSIS_NN)
#include "riscv_nnfunctions.h"
#endif

#define INPUT_CHANNELS      64
#define INPUT_SAMPLES       24
#define CONV_FILTERS        128
#define CONV_KERNEL_SIZE    1
#define CONV_STRIDE         1
#define CONV_GROUPS         1
#define CHANNELS_PER_GROUP  (INPUT_CHANNELS / CONV_GROUPS)
#define FILTERS_PER_GROUP   (CONV_FILTERS / CONV_GROUPS)

#define ZEROPADDING_LEFT    0
#define ZEROPADDING_RIGHT   0

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


static inline void cnn_layers_conv3_pw(
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

#define INPUT_CHANNELS    64
#define CONV_FILTERS      128
#define CONV_KERNEL_SIZE  1
#define CONV_GROUPS       1


const int16_t  cnn_layers_conv3_pw_bias[CONV_FILTERS] = {796, 562, -286, 34, 332, 10, 215, -150, -84, 160, 312, 282, -345, 210, 104, 196, 148, -142, 89, 189, 224, 221, -288, 258, 66, -263, -421, 190, -50, 441, 277, 531, -18, -457, -54, 28, -53, 207, 472, -21, 737, -178, 132, -151, 568, -75, -9, -38, -355, 61, -228, -87, -2, -202, 16, -178, -276, 70, 257, -347, 1003, 124, 479, 167, 288, 42, -360, 119, 58, 385, 76, -130, 425, 211, 50, -270, 61, 193, -56, -330, 75, -283, -122, -35, -2, 247, -263, 107, 236, 87, 8, 230, -114, -247, 423, -285, 246, 568, 112, 22, 523, -203, 15, -354, -418, 7, 376, 131, 326, 185, -5, -308, 102, -356, 523, -122, -69, -117, -257, -89, -526, -316, -112, 472, 857, 148, 303, 123}
;

const int16_t  cnn_layers_conv3_pw_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{-129, -133, -26, 49, 65, -93, 23, 86, 80, -35, 27, -78, 106, -149, 11, -8, -107, -48, -141, -70, 72, -67, -106, -107, -133, 29, 60, -19, -89, -132, -67, -2, -64, -19, -133, -82, -84, 35, -150, 57, -183, 35, -64, -55, 139, -47, -5, 41, 61, -157, 83, -182, 112, -30, 28, -82, 70, -27, -60, -48, -151, 61, -7, -89}
}
, {{-49, -34, -98, -63, 20, -95, 13, -32, -81, 20, 52, -40, -2, -26, -206, 10, -163, -26, 86, 32, -145, -32, -19, -99, -1, 119, -50, -25, 36, -42, 14, -26, 139, -206, 24, -65, -36, 97, -3, 48, -7, 14, -243, -179, 41, 5, -45, 65, -21, 108, 19, -105, 69, -137, 240, -106, 9, 90, 9, -17, 90, -8, -108, -160}
}
, {{14, -11, -9, 53, 36, 44, 16, 7, 49, -7, 7, 3, -17, -3, 1, 50, 43, 11, -3, -10, -49, 17, 32, -56, -11, 22, 11, -47, 1, -75, 55, -13, -35, -25, -5, -10, -18, -65, -32, 12, -9, -32, 33, -20, -16, 27, 12, -6, 56, 11, 9, -17, 31, 32, 13, -8, 13, -16, -19, 20, 27, -45, -4, -14}
}
, {{40, 35, -44, -18, -5, -1, 30, -64, -37, 3, -37, 56, 35, 20, -24, -13, 13, -36, -9, -40, 35, 98, 81, -15, -74, -32, -83, -65, 29, -10, -30, 1, -38, -145, 12, -41, -95, -79, 9, 85, -20, 20, -111, 28, -39, 186, -39, 49, 37, -38, -15, -3, 5, 69, 26, -75, 10, 34, 18, 21, 5, 70, 10, 29}
}
, {{-59, -34, -19, 27, 48, -75, 30, -147, 20, 60, -133, 52, -18, 34, 29, 70, -185, -39, -109, -11, -24, -79, -25, -1, -163, 48, -22, 5, 23, 118, 89, -92, 19, -116, 25, -128, -119, 9, -47, -115, -47, -51, 20, -116, 70, 61, 44, 2, 50, 3, 83, -46, -127, -75, -78, 42, 65, 13, -95, 64, -64, 36, 1, -54}
}
, {{-31, -111, -212, -54, 1, 110, -79, -88, -115, -5, -85, -160, -28, 34, 235, 94, -52, -124, -35, -96, 208, 29, 69, 113, -66, -60, 140, -133, 198, -44, -53, 120, -161, 21, 44, 13, -81, -17, 4, 15, -42, -73, -71, -17, 133, -62, 51, -206, 243, -144, -61, 102, 116, 107, -16, 63, 2, 33, -67, -65, 53, 11, -158, -109}
}
, {{121, 57, 2, 53, 17, 100, -132, 47, 168, -21, -105, -165, 108, -2, -96, 63, 70, 56, 121, -132, 66, 20, -29, 28, 10, -178, -18, -48, -49, 68, -112, -82, 30, -42, -21, -132, 93, -119, -104, 39, 70, -78, 38, 106, 66, 13, 61, -99, -15, -131, 9, -45, -67, 103, -87, -49, -143, 155, -47, -23, 36, -26, -14, -67}
}
, {{-32, 43, 26, 52, -39, -8, 9, 52, 64, -24, 49, -181, 46, 66, 95, 103, 3, 10, 105, -75, 27, -110, 76, 83, 86, -91, 32, 53, -39, 9, 127, 77, -77, 131, -117, 152, 160, 43, 20, -3, 28, -46, -3, -68, -121, -48, -5, -8, -176, -93, 192, 18, 66, -132, -90, 87, 10, 119, -28, 20, -40, -140, -9, -25}
}
, {{-27, -41, -14, -1, 75, 18, 83, -54, 36, -34, -48, 30, 59, -8, 54, -94, 79, -78, -142, -12, 59, -86, -54, -49, 52, -208, -7, -5, -105, -12, -59, 45, -6, -73, 18, 67, 47, -42, 22, -27, -70, 78, -39, 12, -60, 68, -2, -2, -8, -25, -2, 87, 5, -10, 88, 60, 7, -77, 8, 39, -71, 83, 27, -4}
}
, {{-6, -28, -42, -34, 33, -3, 11, -22, -2, -55, 23, 126, -12, -102, -57, -128, 25, -42, 45, 88, -211, -47, -23, 30, 73, 16, -86, -82, 19, -39, 48, 149, 93, -129, 70, -54, -30, -29, -49, 10, -26, 78, -43, -47, -32, 60, -48, 50, 111, -2, 45, -36, 51, 129, -203, 80, 36, -83, -138, 99, 8, -54, -120, -54}
}
, {{-30, 7, -45, -89, -89, -59, 55, -52, -70, -76, -8, 134, -60, 45, -50, 36, -71, -2, -34, -113, -124, 30, 20, -47, -68, 65, -77, -72, -23, -39, 57, -36, 0, 8, -77, -17, 46, 115, 2, -148, -59, -1, -42, -80, 6, 22, 64, -23, 49, -39, -119, -62, -67, 67, -61, -117, -77, -34, 37, 181, -128, -13, 106, -27}
}
, {{6, 28, -32, -11, -6, -65, 1, -19, 19, 2, -39, 10, 22, 2, -9, -8, 1, -3, -3, 27, -21, -3, 61, 11, -16, 6, 17, -3, 30, -45, -5, -16, 2, -62, 58, -28, 5, -55, 19, 31, 41, -26, -8, -68, -78, 32, 71, -6, 38, 14, -2, -102, 34, 9, 47, -52, -6, 28, -39, -64, 20, -2, 15, -67}
}
, {{48, 103, 95, -38, -49, 15, -40, 2, 12, -67, -114, 1, 85, -44, -23, 74, 97, -20, 19, 35, 56, 12, -20, 144, 74, 119, 53, -27, -43, 84, 70, -84, 3, 58, -25, 57, -13, -125, -6, -73, -23, -21, 49, 49, 6, 6, 1, 74, -42, -6, 40, 12, -47, 106, -61, 25, -97, -11, -16, -95, 109, -26, -10, -9}
}
, {{125, -161, 35, 116, -12, -33, -18, -52, 147, 58, 6, -182, -6, -55, 10, 39, -60, -23, -97, 14, -26, -64, -135, 78, -132, 109, -11, -26, 42, 142, -121, 155, -260, 2, -61, -101, 65, -164, 45, -110, -92, -1, -80, 73, -7, -135, -208, 179, 214, 32, 341, -67, -101, 171, 35, -12, -52, -186, -62, 159, 53, -177, -113, 15}
}
, {{50, -4, -32, 3, 38, -53, -77, -26, 20, -23, 76, 54, -43, 60, -3, -38, -86, 90, -1, 51, -49, 18, -13, 57, -126, 114, -29, 36, -42, -25, 14, -12, -59, -139, 13, -14, -92, -91, -69, -12, -24, 38, -100, 90, -47, -16, -57, -28, 6, 116, -37, 28, -3, -90, -29, -47, -18, -44, -66, -38, 56, -44, -57, -27}
}
, {{-73, -3, -33, -9, -28, 15, 39, -86, -7, 77, 44, 14, 43, -93, -59, -10, -12, -141, -133, -6, -12, -159, 50, 91, -7, -169, 70, 95, -105, 33, 37, -84, 16, 109, -24, 113, 15, -50, 120, -11, -12, -54, 8, -101, 110, 17, 225, -71, -17, 35, -131, -225, 3, 139, -180, 77, -244, -92, 101, -64, -32, 74, 19, 81}
}
, {{-32, 22, -11, 15, -36, -100, -9, -6, -3, 34, 96, -17, 7, 60, -13, 80, 25, 39, -3, 55, 30, -13, 85, 18, -76, 10, 209, 41, -16, -43, 32, -171, -78, 7, 9, -10, -26, -61, 7, 30, 26, -69, 26, -18, -31, -107, 18, -1, 51, 12, -28, -55, 43, -25, 3, -37, 2, -6, 14, -77, 10, -26, -35, 64}
}
, {{-36, -47, 143, -32, 7, 48, -21, 120, -57, -55, -49, -78, -9, -143, -105, 90, -37, 82, 114, 139, 172, -108, 75, -1, 4, -2, 95, 73, 105, 134, -90, -50, 125, 121, 69, 30, 55, 58, 172, -47, 30, -20, 84, 8, -106, -140, 10, -165, -49, -47, -81, 100, 45, -4, 22, -54, -57, 196, -110, -3, -166, -60, 51, -34}
}
, {{-20, 83, -6, -92, 24, -63, 23, -11, -190, -67, 8, 81, 29, 115, -7, 60, -19, -86, -7, -64, 28, 19, -27, -85, 20, 89, 51, 63, 38, -54, 0, 69, -6, -128, 85, 47, 65, 31, 89, 0, -30, -65, -76, -66, -55, 109, -68, 33, 35, -22, -49, -93, 31, 5, 2, -161, 11, 33, -81, -74, 24, -66, 71, 6}
}
, {{62, -45, 43, -30, 26, -137, -38, -39, -53, 32, 9, -2, 83, 12, 64, 47, -8, -86, 13, 55, 192, -31, -98, -82, 39, 129, -20, 48, -65, -91, -106, -141, 53, -32, -10, 29, -6, 32, -32, 3, -101, 107, -60, 26, -39, -85, 34, -23, -42, 5, 27, -119, -58, 18, -9, -39, 54, 46, -56, 48, 157, 16, -81, -150}
}
, {{-1, -72, -64, 65, -28, -60, -107, -47, 34, -32, 11, -80, -11, 86, -123, 100, -42, 50, 101, 133, 31, 22, 58, 67, -73, -44, 155, 33, -150, -9, 84, -103, -89, -19, 47, -64, 64, 33, -97, 46, 98, -6, 8, -33, 28, -90, 112, -14, 12, -1, -37, -155, 71, 60, -19, -4, 50, 84, -72, -75, 67, -144, -8, -41}
}
, {{185, -20, 10, 68, -19, 158, 162, -57, -71, 76, -31, -133, -64, 23, 160, -135, 97, -134, -22, -102, -50, 58, -132, -236, 45, 140, -64, -36, -8, 81, -124, -87, -62, 82, -70, 13, 49, 13, -48, 73, -63, 39, 56, -81, 20, -18, 17, 115, -128, 22, 111, 66, -128, -250, 132, -76, 74, -15, 104, -165, 81, -19, 171, -75}
}
, {{68, -14, 106, 34, 14, -54, 104, -2, -46, -30, 85, -88, 94, -55, -81, 37, 35, 265, 106, 76, 50, -16, -88, 27, -23, 115, 70, 50, -4, 74, 143, -126, 156, 31, -21, -80, 23, 4, -32, 45, 5, -24, 91, 40, 26, -114, 85, -39, 53, 161, -128, -122, -115, 44, -43, 34, -14, 95, -154, 2, 230, 8, 110, -124}
}
, {{59, 28, 74, 0, 52, -13, 72, 164, 117, -140, -8, -17, 122, -60, -4, -114, -147, 119, -54, -24, -80, -185, 63, -15, 92, 21, -80, 127, -110, -74, -12, -82, 182, -152, -70, 83, 104, -35, -214, 165, -111, 88, -149, 127, -106, 8, -125, -123, 71, -194, 38, -13, 68, 11, -75, -69, -41, -31, 124, -38, -114, -119, 95, 156}
}
, {{71, 63, 14, 61, -99, 107, -2, -12, 98, 25, 63, -105, -77, 12, -22, 18, 7, -12, 41, -24, -64, 158, 86, 103, -75, -19, 39, 52, 11, -10, -28, 177, 28, 4, -77, 68, 20, -12, 16, 3, 20, -149, 77, -12, -136, -135, 70, 96, -59, -3, -108, 13, -67, -51, -69, -73, 84, 44, 67, -68, 65, -46, 2, -82}
}
, {{-18, -35, -93, -104, 49, 46, 29, 11, 2, -9, 2, 1, -25, 11, 49, -1, -23, 63, -62, -28, 89, -117, 66, 30, -92, -80, 134, 33, -84, 33, 77, -80, -59, 26, -26, 108, -74, -71, -20, 24, 33, -47, -32, 47, 17, -66, 27, -6, 22, 153, -36, 79, -18, 70, -57, 23, -5, -39, 9, 81, -27, 9, -117, 17}
}
, {{-90, 59, -5, 30, 29, 24, -28, -19, -134, 25, 5, 83, 112, 142, 66, -11, -152, 80, 48, 30, -115, -4, 50, 91, 13, -191, 73, 115, 82, 85, -49, 18, 43, 66, -100, 179, 82, 66, 15, -74, -79, 34, -36, 63, -69, 59, 33, -169, 29, 41, -212, -73, 81, -18, 26, 46, 110, 36, 114, 47, -74, -30, 154, -88}
}
, {{171, -27, 103, 18, 7, -79, -21, 111, -98, 22, -132, -95, 44, -4, -9, -122, 52, -71, 3, 18, -31, -54, 14, 93, 3, -8, -57, 22, 180, 205, -55, -169, 105, 70, -142, -58, -39, 50, 5, 90, 6, -34, -21, -53, 101, -32, -87, -33, -32, 61, 27, -46, -144, -54, 29, 2, -85, 54, -35, 173, 157, -76, 102, 58}
}
, {{-5, -60, 26, -71, 9, -2, -6, -40, -32, -65, -14, 57, 28, 152, 52, 132, 90, -129, -22, 63, -126, 0, 70, 98, 0, 137, -26, -181, 80, -29, -31, 87, -61, 87, 14, -47, -98, -201, 74, -95, 67, 24, -60, 118, 19, -104, 66, -124, -218, 31, 17, 104, -21, -129, -9, 26, 88, 67, -90, 154, 100, -73, -122, -61}
}
, {{-55, -57, -29, -26, 23, -48, 11, -79, -61, -42, 12, 4, 29, -24, -20, 27, -74, -82, 17, 1, 36, -10, -28, -11, -68, -7, 1, -26, 10, -5, 0, 43, -20, 88, -77, -5, 7, -42, -3, -40, -37, -9, -110, -62, -15, -9, 10, -11, 36, -16, -5, -29, -21, -33, -18, -30, -16, -12, 15, 10, -29, -16, 34, 24}
}
, {{26, 22, 14, 5, 68, -22, -49, 8, 33, -26, 3, -20, -18, 5, -22, 11, 0, 34, -16, 19, 36, 0, 60, -73, -33, 9, 4, -42, 10, -45, -17, -74, 14, -24, 29, 39, -31, -39, 3, 86, 14, -16, -22, -17, 19, -31, -109, 6, 8, 19, -19, -25, 0, -48, 42, -53, -14, 10, -37, -18, -28, 60, 23, 58}
}
, {{52, 24, 65, 88, -50, -201, -21, 98, 121, 153, -140, 51, -82, -66, -78, -158, -109, -3, -131, -123, -110, -80, -32, 71, 81, -6, -240, -29, 121, 8, 126, -98, 190, -13, -169, 30, -32, -24, -4, 6, 86, 40, 96, -18, 85, 111, -108, -28, -161, -69, 51, -20, -125, 34, 132, 57, 34, 132, 30, -67, -105, -238, 25, -28}
}
, {{29, 37, -35, -41, -42, 95, 89, -58, 64, 11, -270, -81, -16, 50, 21, 82, 83, -111, 6, 57, -58, -19, 55, -6, 19, 14, -61, -29, 53, -156, -166, 114, 63, -78, -115, -196, 101, 41, -51, 32, -25, 83, -5, -109, 34, -9, -106, -163, -29, -34, -36, 36, 59, 7, 52, 44, -28, 36, 60, 93, 136, 160, -36, -62}
}
, {{56, -27, -45, 100, 40, 137, -88, -27, 79, 86, 72, 52, -28, -56, -67, -15, 18, 75, -13, 59, 53, -70, 3, -124, -6, 1, 49, 56, -1, -67, 6, -46, 17, -27, -15, -21, -29, 26, -85, 75, 22, 14, 36, 85, -30, -8, -41, 29, 7, 139, 96, -12, 112, 15, 20, -17, -73, -81, 5, 12, 35, 79, -8, 54}
}
, {{53, -155, -82, 66, 7, -25, 1, -12, 99, -56, -37, 12, -12, 35, 22, -18, -30, -32, -36, 1, -47, -90, -7, 59, 77, -33, -16, 74, -53, 2, 1, 0, -52, 69, -75, 148, 0, 22, -79, -71, 1, 70, -34, -21, 81, -51, -13, -53, 6, -16, -8, -29, -84, 37, 14, 54, -60, -1, 40, 101, -49, 8, 48, -39}
}
, {{16, -21, 12, 47, 71, 9, -2, 1, -8, 15, 30, -96, -103, 20, 10, 68, 1, 28, 10, -28, 12, -31, 46, -34, -42, -33, 68, -50, -15, -56, 45, -40, -36, 64, 71, -1, -26, -81, 10, -19, -8, 51, -34, 88, 26, 27, -155, 38, -21, -17, 20, 41, -16, -11, 60, -16, 61, 52, 1, 51, 31, 13, 11, 81}
}
, {{95, 162, 68, -19, 125, -50, 128, -163, 15, 23, -100, -23, -6, -68, 140, -122, -44, 51, -204, -111, 74, -69, -92, -286, -7, -104, 281, -42, -93, 22, 194, -107, -53, 137, 43, 69, 9, -100, -120, -127, -115, 85, 60, 170, 136, 111, -146, -25, 186, 94, 39, 70, -63, -117, 47, 200, 57, -46, -175, 16, 28, 65, 50, -37}
}
, {{23, 15, 39, 19, 18, -91, 112, -45, -41, 26, 12, -85, 89, 25, 112, 66, 25, -31, 15, -73, -50, 52, -95, -22, -58, 14, -28, 28, -77, 67, -68, -12, 15, 15, 67, -35, 9, -1, 20, 61, -38, -30, 7, 20, -38, -4, 29, 47, -97, -51, 9, 80, -130, 26, 14, -32, -50, 42, -38, 11, 74, -6, 50, 57}
}
, {{-139, -154, -36, 52, -39, -54, 88, -32, -71, 32, -13, 0, 29, -20, 74, 59, -77, -90, -69, -45, 47, 9, -49, -58, 39, -50, -31, 42, -11, 13, 17, 11, -59, 7, -85, -161, 59, 63, 39, -101, 13, 22, 9, -7, 4, 51, -23, -3, -44, -44, 45, -31, -54, -102, -12, 37, -64, 79, 30, -31, 26, -72, 36, -50}
}
, {{-82, 29, 93, -72, -178, 63, -18, 109, -12, -86, -33, 84, 59, 183, 3, -145, -51, -43, 17, -131, -50, -179, -41, -27, -31, -90, -45, 59, 1, 33, 154, 41, 166, -23, 11, 62, 100, -70, 38, 122, -26, -97, -21, -46, 64, 64, 80, -36, 32, 107, 11, 125, -63, -9, 65, -1, -6, -43, -5, -23, -40, -225, -17, 49}
}
, {{42, -14, -34, 65, -43, -62, -105, 71, 9, -18, -12, -7, 19, -29, -139, -55, 15, 138, -16, -97, 31, -58, -45, -61, 6, 6, -43, 69, -66, -8, -68, -102, 64, -170, 25, -13, 0, -118, -4, 87, -65, 42, 72, 100, 23, -65, -60, -40, -84, -25, 28, -246, -61, 16, 24, -94, -30, 44, -129, -193, 71, 48, 82, -29}
}
, {{42, 9, 0, 44, 59, 93, 74, 27, 109, 32, -44, -48, -81, -3, -34, 14, 56, 2, 37, 27, -70, 65, 43, 7, -15, 64, 10, -19, 15, -75, 38, 51, 59, 32, 28, 12, 35, 36, 8, 76, 28, 48, 81, 10, 151, -67, -40, 36, 65, -13, -126, -38, -45, -11, -15, 52, 72, -34, -54, -37, 41, 29, -62, -11}
}
, {{-281, 44, -204, -236, -52, 119, -88, -52, 118, -38, 8, 54, 90, -136, 35, 83, -55, 76, 50, -100, -45, -119, 70, 5, -28, -184, -162, 10, -129, -8, -137, 323, -46, 127, -50, 71, 122, -44, -1, 99, -16, 12, -8, 134, 11, 46, -120, -238, 94, 25, -84, -11, 133, 21, 55, -1, -8, -93, 64, -47, -156, -2, -194, 128}
}
, {{-5, 95, 71, -13, 103, -2, -173, 80, 15, -142, -38, 34, 31, -93, 73, -47, 138, 36, -155, -12, 24, -27, 21, -2, -34, -8, -39, 62, 10, -63, -118, 52, -37, 129, 213, -33, -79, -88, 58, 32, -22, 76, 27, -43, 149, 196, -66, -33, 139, -61, -60, 8, 100, 162, 20, -41, -193, -126, 71, -47, -89, 9, 13, -18}
}
, {{-83, 8, -139, -64, 15, -13, 65, -2, -10, -87, 56, -111, 18, -80, 123, 85, -99, 62, -40, -78, -14, 86, -47, -24, -167, -99, 100, -13, -210, -32, -59, 11, -109, -63, 83, -45, -5, -54, -80, -78, -41, 44, -107, 29, -78, -38, 62, -33, 47, -90, 81, -104, 18, -21, -7, -60, -29, -77, 105, 69, -78, -45, -113, -40}
}
, {{0, 19, 8, -8, -85, 149, 115, -57, 79, 159, 88, -21, -30, 36, 50, 60, 37, 72, 16, 15, -76, -32, 109, 50, 40, -2, 87, 20, -85, 49, -49, -54, -49, -33, -57, 13, -5, 17, -140, 10, -77, -62, -60, -94, 88, -38, -92, 162, 50, -11, -87, 0, 15, -115, -162, 108, 31, 3, 92, -10, -13, -87, 23, -106}
}
, {{52, -14, 50, 15, 28, -14, 20, -60, 2, 15, 5, 4, 27, -72, -24, -87, -14, -71, 3, 46, -8, 40, -78, 30, 29, -142, -20, 82, 34, -68, 22, 17, 22, -36, 72, -113, -57, -51, 38, -8, 0, -66, 12, 0, -12, 33, -23, -12, -5, -32, 7, 8, 52, -15, 37, -60, 16, 22, -40, -15, 28, -13, -77, 25}
}
, {{-13, 6, 14, -8, 21, -135, -7, 71, 19, -58, 48, 0, -1, -52, 21, 51, 1, 13, 42, 30, 70, 13, -33, -62, -13, 31, 84, 71, 22, 10, -74, 33, 1, 22, 59, -17, 9, -8, -18, -17, 26, -67, 0, -45, -134, -29, 63, 70, 48, -44, -48, -9, 6, -6, 35, -58, -120, 64, -81, -13, -24, -40, -7, -114}
}
, {{-63, 43, -45, -62, -37, 75, -32, 64, 61, 9, -72, 5, 72, -35, -21, -25, -52, 94, 116, 93, -43, 155, 100, 80, 6, -209, 47, 29, -66, 63, 145, 38, -4, -48, 61, 71, -17, 31, -60, 54, 79, -10, -94, 62, 73, -55, 89, 38, 4, 43, -52, 16, 100, 43, -176, -8, -42, 34, -28, 48, -189, 6, -85, -52}
}
, {{-90, 109, -57, -51, -92, -60, -84, 2, 14, 10, 177, 136, -48, -89, 72, -164, -35, -5, 24, 179, 49, -36, 15, 74, -116, -48, 128, 52, -65, -126, 13, -111, 154, -53, -21, 51, -64, 60, 64, 35, -16, -119, -18, 3, -13, 8, -39, 57, -15, -35, 112, -8, 149, 64, 4, -17, 58, 70, -96, -186, -27, -1, 1, -18}
}
, {{31, -2, 72, 83, -39, -104, 135, 53, 160, -109, -44, -39, -35, -43, 2, -18, 45, -33, -153, 88, -42, 29, 71, 69, 78, 105, 89, 157, 6, -29, 14, -73, 102, 47, 36, 31, 16, 64, -9, 9, -122, -27, 121, -46, -21, -25, -55, 41, 90, 39, 32, -82, -75, 72, -43, -32, 48, 65, -129, 54, -70, -1, -1, 85}
}
, {{-105, 106, -66, -64, -99, -188, 6, 46, -49, -60, 67, 164, -18, 9, 2, 14, 136, 100, 9, 115, -106, 99, -130, 61, 127, 76, -48, 15, -42, -73, -63, -38, 137, -34, -149, 159, 34, -13, 57, -39, 44, -60, 135, 24, -292, 105, 85, 25, -9, -146, -38, -120, 99, 46, -22, -170, -84, 4, 41, -11, -93, 139, 121, -48}
}
, {{-30, -61, -63, -76, 49, -4, 20, 31, -30, 41, 2, -17, -106, -122, -57, 112, -13, -1, -104, 105, 107, -69, 40, 54, -129, -71, 49, 19, -55, -60, -91, -26, 17, -29, 15, -103, -34, -18, -7, -60, 40, 53, -7, -44, 22, -47, 32, -26, 33, -72, 49, 13, 34, -53, 9, -88, 57, 4, -76, 19, 77, 41, 9, -12}
}
, {{135, 75, -4, 26, 203, -190, 52, -126, -66, -100, -90, 50, -125, 4, -30, 43, -81, 10, 33, 161, 160, 190, 112, 74, -78, 169, 28, -40, 19, 21, 4, 47, -126, 125, 51, -32, -34, -111, 185, -22, -45, -101, -95, 87, -23, -17, -66, -87, 51, 40, -30, -108, -143, -108, -83, 139, 61, -5, 46, 37, -68, -115, 39, 86}
}
, {{18, 33, -83, -107, -24, 74, 119, 57, -3, -93, -136, 108, -9, 190, -3, 62, -28, 35, 75, -26, -8, 40, -22, 128, -85, -155, 143, -73, 124, 72, 90, -42, -93, 112, -33, -70, 6, -79, -22, -142, 21, -39, 4, -166, -65, -18, -51, -166, 59, 21, 78, 35, 31, -80, -113, 11, 15, 0, 22, -3, -93, -18, 106, 50}
}
, {{-7, -36, 62, 1, 15, 0, 67, 30, -12, -71, -98, -15, -4, -40, 30, 80, -133, 3, 19, 128, -57, 89, 28, -20, 48, 108, -37, -96, 40, 71, -35, -43, -127, 65, 27, -189, -154, -83, -53, -98, -12, -85, -1, 9, -5, -101, 83, -131, 82, 22, 203, -45, -43, 38, 72, 40, 88, 67, -1, 96, 76, -148, 138, 97}
}
, {{-44, 61, 38, -56, 128, 6, -36, 55, 66, -75, -18, 52, 105, -39, 162, -14, 65, 36, -29, 5, 109, -49, 154, 17, -33, -19, 25, 71, 103, -96, -114, -105, -54, 85, 147, -82, 38, -115, -29, -33, -21, 102, -71, -144, 187, 189, -6, -149, 106, -77, 1, -159, 193, 62, 58, 30, -135, -54, 10, -57, -86, -96, 35, -63}
}
, {{-31, 113, -48, -11, 38, 55, 28, -34, -27, -17, -32, -24, 25, 15, 51, 8, 48, -48, -21, -43, -37, 100, 116, -1, -8, -41, 56, -87, -42, -55, 25, 83, -6, -146, -46, -107, -51, 13, 0, 64, -64, 4, -82, 3, -69, 140, -5, 56, 33, -21, -12, 20, -78, 87, -25, -2, 43, 30, 67, 93, -37, -39, -122, -21}
}
, {{1, 56, 25, -25, -87, -70, 84, -52, -60, -4, 86, -8, -33, -38, -52, -68, -105, -48, 46, 58, -81, -8, -91, 26, -27, 52, 109, 99, 27, -17, 15, 38, 31, -63, -61, -22, -2, 56, 27, -151, -55, -77, -5, -13, -3, 116, 40, -35, 30, -38, -56, -40, 71, -18, -21, 17, 24, 48, -71, -96, 88, -3, 39, 18}
}
, {{-11, 121, 61, 91, 109, 39, -2, 81, -1, -95, 226, 53, 16, 33, -18, 3, -18, -62, 124, -23, -2, -20, 6, -29, 93, -89, 11, 119, 31, 99, -44, 115, 8, 71, -136, 7, 76, 44, 75, -72, -15, -91, 8, -41, -1, -59, -41, 104, -105, -5, -95, 22, 149, -140, 38, -9, 62, 0, -7, 22, -33, -35, -30, 136}
}
, {{101, 37, 4, 145, -142, -165, -35, -195, 89, 14, -84, -7, 61, -69, -149, -43, 137, -230, -172, -156, -58, -31, 41, -60, 69, -66, -94, -103, 41, -156, 45, -195, 65, 0, -63, -4, -94, -159, 65, 79, -117, 127, 27, -39, -52, -58, -31, 23, -37, -221, -44, -208, -152, -32, -34, -135, 21, 112, -76, 49, 147, -151, 125, -76}
}
, {{-61, 6, -27, 77, -68, -66, 15, -153, -35, -23, 55, 100, 50, 12, -55, 109, 40, 6, 56, 114, 11, 23, -15, 33, -129, -157, 143, 100, -159, 4, -87, -152, -19, -30, -16, 93, -107, 113, -128, 23, -135, -2, 76, -201, 79, 4, 88, -17, 25, -68, 64, 18, -14, -20, -143, 1, -216, -84, -46, -144, 159, 135, 113, -88}
}
, {{142, 58, -36, 186, 102, -76, 63, -73, 41, 88, 14, -236, 46, -157, -88, 254, -113, -121, 89, -139, -43, 151, 80, -25, -38, 114, -96, -90, -147, -81, 33, -1, -35, 71, -112, 1, 70, 160, -35, 37, -133, -33, 49, -75, -217, -9, -6, -245, 166, 112, 12, 15, -142, 132, -260, -63, -8, -3, 15, -23, 66, -63, 5, -166}
}
, {{7, 41, -132, 19, 157, 56, -92, -91, 33, -133, 82, 73, -63, 101, -7, 194, -162, 60, -22, 43, 74, 17, 3, -92, -31, -13, 109, 143, -86, 27, 112, -65, -237, 107, -259, 99, 22, -63, -98, 99, -7, 83, -103, -138, 76, 72, -134, 31, -82, 32, -153, -36, -183, -108, -25, 55, 20, 71, -21, -26, 14, 3, -181, 19}
}
, {{18, 40, 2, 171, -56, 73, 45, 64, 77, 45, -8, -144, -37, -28, -56, -54, 4, 10, -11, -1, -73, 232, -27, -62, -32, -59, -33, 65, -55, -5, -21, 32, 40, -5, -57, 14, -70, 21, 10, -40, -9, 34, 101, 101, -43, -26, -17, 59, 0, -56, -39, -13, -123, -52, 26, -62, -9, 42, 60, -74, -91, -127, -30, 17}
}
, {{-105, -20, -31, -166, -105, -6, -71, 32, 35, -178, 10, 108, 36, 48, 48, -63, 25, 143, 92, 34, 53, 14, 52, 46, -106, -152, 3, -26, -112, -14, -19, -71, -194, 153, 73, -109, -14, -4, -23, 21, -60, 86, -42, -1, 7, -22, -26, -10, -191, -170, 17, 19, 83, 36, 14, -48, -1, -67, 262, 13, -131, -63, 51, -73}
}
, {{48, -38, 37, -20, -43, -32, -134, -13, -27, -37, 16, 49, -91, -44, -7, -67, 144, 122, -60, -14, 229, 2, -127, 131, 47, 16, 71, -171, 161, -4, 70, -5, 97, 94, -90, -60, 84, 6, 11, -112, 84, 87, -17, 5, 77, -287, 52, -48, 111, -3, 80, 120, 61, 4, -136, -81, -69, -6, 58, -13, -126, 182, -39, 98}
}
, {{98, 26, 7, 230, 4, -143, 53, 167, 85, 205, 110, 20, -26, -125, -45, -140, 22, -300, -6, -99, 17, 104, -167, -94, -12, -6, -80, 10, 31, 30, 119, 125, -32, 92, -50, -8, -52, 73, -29, -175, -179, 104, -11, 46, -41, -85, 39, 240, -66, -11, -36, -5, 17, 34, -45, 30, -60, -117, -145, 73, 74, -171, 127, 99}
}
, {{-32, 55, 29, -47, -37, 27, -37, 65, -34, 30, 51, -35, -78, -71, -27, -9, -42, 91, 6, 6, -6, -55, -52, -22, -62, 90, -56, 37, 42, 20, 52, 70, 33, -2, -33, -44, 37, 4, 22, 40, 45, -31, -5, 20, -16, -57, 22, -17, 36, 62, -42, 33, 44, 14, -213, -4, -25, 32, -6, -118, -68, -52, -78, 47}
}
, {{-149, 53, -208, -52, 38, 102, -37, 64, -43, -40, -52, 33, -7, -13, -21, -15, 21, -33, -18, -146, 43, -175, -28, -164, 19, -56, 12, 190, -124, -108, 24, -99, 123, -133, -5, 61, 132, -101, -52, -16, 73, 111, 81, 83, -10, -79, -39, -33, 144, -20, -13, 20, -70, -107, -12, -267, 82, 64, 52, -38, 10, 4, 74, 27}
}
, {{-3, 47, 7, -197, -47, -5, 9, -48, 27, -162, -138, 9, 192, 63, -16, -47, 140, -124, 74, -56, -45, 49, -43, -6, 44, 160, 41, 62, 45, -77, 42, 87, 4, -100, 103, 22, 143, 12, -45, 65, -168, -20, -84, -89, -83, -31, -129, 118, -2, 23, -4, 61, -92, -55, -27, 87, 65, -10, -131, 4, 66, -127, -61, -84}
}
, {{72, 94, -119, -29, 85, 60, -99, 95, 12, 8, 44, 39, -43, -139, -2, -94, -70, 37, 32, -38, 44, 30, -23, -82, -53, -117, 42, -32, 89, 78, 51, 111, 25, 6, 104, 13, -25, -11, 42, 44, -29, -204, 14, 23, 65, -24, -90, -9, 114, 112, 82, 27, 129, 2, -26, 26, 40, 8, -63, 5, -77, 1, -12, 165}
}
, {{87, -32, -15, 40, -38, -81, 16, -6, 18, 27, -3, -55, 92, 130, 21, -32, 34, 12, -43, 14, -33, 1, -86, -42, 53, -47, 52, 80, -32, 83, -10, -103, 22, -59, 74, -22, 76, 2, 44, 51, -15, -126, 24, 52, -18, -65, -21, 10, -102, 26, 67, -73, -80, -158, -49, -47, -18, 18, -18, 23, 9, -52, -30, -11}
}
, {{-80, -109, -48, -76, -125, -5, -48, -125, -43, -117, 85, 57, 60, -119, -5, 53, -140, 33, 33, 16, -134, -49, -12, 44, -3, 1, 17, 130, -86, 65, -50, 99, -4, 52, -84, 13, 27, 54, -38, -83, -33, 68, -96, 62, -14, -55, -63, -17, 28, 30, -84, 57, 92, 9, -46, -10, -10, -35, 40, 96, -71, -4, -12, 54}
}
, {{-31, -21, -27, 16, -177, 102, 148, 85, 24, 80, -47, -125, 170, -96, -151, 20, -41, -66, 168, 52, -137, -23, -28, -15, -60, -27, -109, 74, 9, -35, -1, 18, 32, 118, 98, 7, 62, 12, -53, 85, 85, -119, 6, -36, 98, -163, -36, 94, 86, -65, -50, 104, 96, -17, -121, -129, -2, 118, 18, -7, -127, -79, 46, 28}
}
, {{-94, -40, -3, 186, -51, -9, 162, 157, -10, -41, -164, 32, 259, 68, 26, 42, 98, -190, 73, 99, -6, -80, -108, -99, 199, -43, 92, 113, -104, -32, -107, 50, 149, 47, 21, 95, 129, 83, -92, -93, -58, 20, 5, 36, 91, -44, -24, 227, -56, -84, 99, 67, -168, 45, 119, 21, -47, 18, -151, 4, 138, 155, 30, -18}
}
, {{120, 136, -6, 107, -71, -42, 43, -116, 12, 119, 88, -163, -144, 65, 149, -178, 24, -135, 23, -140, 97, -21, -13, -86, 8, 63, -35, 4, -89, -172, 61, 78, 142, -14, 76, 207, -222, 88, 68, 72, -211, -54, 9, 28, -308, 61, 139, 169, -7, 4, 155, 11, -207, 80, -183, -121, 45, -37, -114, 186, 26, 47, -78, -96}
}
, {{-20, -91, 7, -7, -45, 87, 147, 4, 49, 16, 76, -176, 31, -70, -21, -67, 5, 88, -9, -40, -103, 92, 116, -45, -64, 60, -47, 52, -154, 1, 92, -56, 6, 50, 1, -16, 20, 60, -43, 6, 2, -123, -13, -59, 37, -7, -124, -20, 60, 23, 58, -39, -81, 37, -158, -25, 4, -38, -30, 53, -73, -23, 4, -7}
}
, {{-7, 39, -3, 72, -39, 220, 28, -8, 45, 65, -64, -137, 24, 8, 14, 84, 90, 3, -13, -66, -40, 143, 16, 3, 1, -66, 22, -53, -14, -72, -20, 118, 40, 85, 72, 27, -4, 38, 0, 83, -92, -179, 50, 2, 35, -181, -93, 17, -10, -73, -8, -64, -17, 27, -31, 90, 4, 20, 47, 43, 79, 76, -85, 16}
}
, {{-24, 53, 27, 19, -10, 4, 100, 77, 22, 14, 115, 119, -56, -19, -72, -32, -55, 21, -40, 35, 47, -103, -90, -7, -75, 68, 77, 6, -5, 13, -7, -2, -47, 14, -77, -117, 17, 19, 32, 44, 56, -122, 5, 32, 29, 57, 36, -48, -16, 13, 54, 71, 99, -60, 93, -93, 80, 33, -81, 4, 42, -51, 84, 86}
}
, {{-42, 13, 33, 24, -4, -109, -18, 0, 38, 0, 21, 3, 45, 17, -100, -30, -16, 35, -25, 92, -112, -7, 27, 8, 23, 37, 39, 95, 89, 21, 18, -2, 7, -70, 48, -24, 40, 53, 20, 49, 89, -25, 5, -26, -62, -50, 102, 43, 41, 32, -25, -77, 71, -27, 115, -15, 14, 57, -23, -98, 64, -60, -75, -33}
}
, {{-25, -13, 71, 65, -136, -258, 92, 84, -52, -16, 89, 241, -53, -145, 44, -42, 144, 182, -18, 110, 38, -26, -118, 83, 2, 168, -15, -178, -36, -71, -23, 56, 12, -68, 59, -73, -152, -245, 55, -112, -57, 170, 242, 105, -1, -43, -181, 160, 143, 63, -88, 28, 1, -5, -284, 52, -55, 80, -121, 294, 228, 91, -55, 84}
}
, {{3, 4, 21, -40, -80, 86, -85, 253, 67, 58, -62, 56, 171, -201, 121, -116, -109, 42, 80, -83, 14, -265, 93, -21, 193, 65, -55, -32, 126, 218, 52, 13, -121, 3, 33, -81, 117, -51, -83, 49, 3, -288, 36, 6, 62, 56, -29, -1, -89, -98, -91, 56, 290, 1, -156, 24, 45, 12, 4, 23, 55, 143, 157, 29}
}
, {{-163, -29, -108, -36, 72, -109, -99, -36, -51, 27, 145, 178, -105, 40, 99, 57, -29, 61, 13, -112, -40, 10, -9, 14, -70, 62, 164, 48, -89, 50, -77, -64, -48, -87, -80, -29, 9, 42, -142, 47, -65, 153, 23, 82, 52, 21, -24, -85, 69, 78, 47, -35, -163, 58, 93, 5, 46, -65, 9, 74, 176, -66, -51, 88}
}
, {{56, -195, -64, -88, -52, 99, 40, 16, -18, 64, -23, -136, 3, 197, 33, 14, 47, -72, 2, 64, -113, -4, -33, 93, -10, -53, -68, -60, -25, 49, -42, 2, 149, 13, 121, 26, 5, -127, -90, -38, 119, 81, -6, 54, -31, -92, -131, -41, -24, 77, 58, -104, -19, 45, 81, 15, 29, 23, -67, 104, 84, 71, 98, 8}
}
, {{-59, 28, 15, 2, -196, 64, -124, 101, 64, 21, 108, -249, 77, -8, -13, -44, 120, 169, 53, -74, 133, -228, -17, -61, 32, -86, -129, -129, -162, -29, 23, 88, 51, -117, 73, -58, 72, -50, -156, 15, 177, -210, -20, 177, -191, 158, -83, 110, -164, -99, 203, -98, 7, 200, -74, 24, -43, 108, -245, 247, -100, 54, 102, 92}
}
, {{1, 52, -21, -68, 28, -16, 218, -17, -18, -25, 9, -54, -135, 54, 144, -155, 133, 11, -73, -87, -243, 95, 100, -7, 234, -193, 115, 105, 29, -46, 95, 49, 167, -107, 18, -2, -111, 84, -86, -97, 62, 216, 6, -1, -38, 19, -45, 3, -45, 95, 23, 59, -11, -53, -59, -108, 53, -58, 81, 177, -173, -79, 72, -22}
}
, {{90, -109, -93, 25, 76, -120, 157, -46, 106, 7, -99, 29, -15, 30, 18, -48, 71, -8, 53, -272, -77, -20, -144, -26, -19, 21, 116, -7, 105, -16, 137, -13, -57, -103, 74, -141, 36, 54, -18, 24, 286, 25, -61, -69, 101, 87, -109, 44, -62, 136, 37, 89, 58, -58, 105, -35, 21, -70, -180, 88, 8, 49, -141, 24}
}
, {{0, 77, -5, 76, -41, 71, -26, 28, 53, 37, -34, -8, -11, -5, -78, -16, -4, -9, -13, 10, -35, 136, 4, -60, 27, -3, -4, 37, -25, -6, -19, 54, 35, -9, -111, -45, -9, -15, 33, -23, -38, -20, 59, 40, -54, 28, -39, 47, -19, -28, -36, -14, 0, -41, 2, -33, 46, 25, 31, 8, -52, -13, -51, 26}
}
, {{-14, 70, -64, -19, 128, -48, -105, 9, 28, 33, -91, -74, 6, 51, 218, -58, -8, -40, -91, 112, -114, 162, -51, 26, -21, 62, 119, 75, -57, -81, 61, 88, -6, 33, -148, -139, 87, 260, 89, -44, -67, -210, 110, -36, 49, 55, 43, -71, -29, 56, -30, -84, -72, -198, 102, 48, 179, -104, -92, -92, -108, 102, 146, -130}
}
, {{40, 87, -11, 72, 151, 12, 48, -41, -21, -23, 33, -38, 101, -41, 37, 66, 46, -5, 13, -2, 25, 106, 110, 48, 7, 22, -4, -21, -96, -71, -75, 26, -16, 19, 88, 21, -3, -24, -43, 28, -94, -83, 100, 14, -116, -59, 42, 7, 28, -125, 6, -42, -146, 77, -43, 75, 20, 36, -61, 60, 112, -4, -147, -80}
}
, {{-106, -159, -46, 33, 43, 117, -43, -63, 127, 27, 28, 117, 140, 119, 203, -23, 39, -64, -10, 91, 99, -80, -39, 63, -19, -40, -170, -40, -112, 30, -119, -98, -104, 144, -66, -5, 40, -109, -33, -154, -178, -153, 74, 81, -22, 31, -34, 9, -19, -66, 37, -22, 15, 41, -276, -47, -5, -160, 99, -130, 13, -76, 78, -6}
}
, {{-52, -38, -77, 27, 15, 19, 77, 12, -51, -20, 56, 17, 68, 34, -11, -58, 120, 1, -48, -23, -81, -39, -144, 23, -81, -76, 37, 12, 3, 65, -24, -93, 0, -1, 54, 191, -75, 25, -40, 28, 12, 15, -13, -83, 72, 14, 71, 100, 12, 88, -33, 43, -99, -38, 116, -33, -215, -110, 46, -84, 82, -7, 4, -13}
}
, {{-28, 18, 83, -53, -9, -30, 56, 129, 77, -25, -37, -82, 56, 17, 69, -56, 83, 37, 85, 46, -14, -150, -25, 67, 59, 13, -73, 81, 105, 60, -141, -15, 81, -43, -4, 82, 61, 61, 64, -23, 60, -62, -47, 67, -55, 7, 35, -51, -149, 76, 33, -7, 57, -58, 114, 13, -69, 88, -98, 22, 37, -68, 1, 44}
}
, {{31, 52, 15, -47, 2, -42, 92, -106, 32, -43, -111, -56, -103, -100, 17, -122, 7, -119, -91, -69, -55, -3, -1, 41, -79, -91, -13, -54, -109, -26, -2, 62, 25, -61, 63, 38, -49, -90, -19, 87, -62, 83, -40, 20, 46, 26, 10, -21, -49, -96, 27, -42, -61, 29, 23, 32, -29, -15, 45, -71, -150, 28, -20, -9}
}
, {{68, 27, 75, -186, 45, -49, 18, -92, -89, 6, -174, -46, -73, 23, 62, -49, -9, -76, -20, 172, -74, -93, 61, 11, 105, -92, 55, 87, 72, 100, -243, 218, -19, 209, -39, 8, 33, -60, 34, 92, -75, -59, 28, 220, -72, -51, 58, -148, -16, 20, -61, 85, 25, -52, 41, -120, 152, -73, 94, 61, 72, 56, -23, 6}
}
, {{-25, 1, -96, 3, 9, -139, -2, -86, -18, -27, 40, 35, -60, 20, 86, -14, -67, -40, -30, 12, 91, 74, -40, -1, -144, -26, -14, -52, -8, -75, 67, -13, -131, 42, -33, -27, -75, -52, 28, 11, -25, 50, -108, -3, -11, 7, 33, 4, -3, -18, -47, 41, -51, 57, -114, -82, -72, -72, 77, 97, -46, 19, 35, 85}
}
, {{-35, 17, -59, 87, -52, -150, -132, 39, 87, 34, 37, 15, -86, -7, -125, -8, -51, 153, -44, 33, 47, 5, 39, -12, -107, -11, 8, 113, -96, -38, -31, -219, 41, -182, -18, -84, 9, -77, -112, 49, 65, 93, 80, 45, 79, -127, 44, -22, -51, 77, 74, -180, -4, 17, 43, -77, -85, 39, -286, -187, 76, -56, 85, 59}
}
, {{-205, -10, -85, -203, -70, -35, 33, -52, -49, -105, 43, 140, 32, -22, 11, -51, -117, 21, 24, 7, -8, 38, -46, 5, -98, -13, 30, 69, 49, -32, 18, 40, 41, 45, 3, -45, 61, -93, 59, -44, 44, 1, -36, 88, 11, -26, 31, -70, 40, -7, -122, 13, 35, 54, -41, 36, -72, 44, -55, 4, -115, -37, -117, 55}
}
, {{27, 97, 48, -61, 79, -72, -68, 98, 8, -66, 64, -16, -63, -20, 6, 32, -46, 44, 29, -22, -59, -51, -47, -39, 4, 59, -72, 0, -31, -37, -85, -1, 81, -8, -43, 122, -50, 36, -59, 70, -43, -73, -14, -5, -52, 59, 20, -5, 84, -118, 3, 102, 9, 54, 41, -116, -58, 21, -172, -31, 10, 4, 92, 32}
}
, {{45, 160, 52, -87, -16, -79, -43, -28, 124, -35, -92, 102, -57, 188, 9, -100, -57, 79, -45, -29, -36, -43, 98, -164, -28, 37, 74, -17, -86, -163, -8, -253, -91, 110, 91, -43, -73, -70, 101, -77, 73, 5, -17, -197, 21, -105, -134, -34, -35, -132, 47, -196, 199, -176, 5, 50, 55, 89, 3, -80, 47, 43, 1, -46}
}
, {{-53, 95, -113, 25, -33, 83, -28, -65, -45, -16, 74, 154, -38, 68, -192, -12, -75, 107, -118, 32, 65, 72, -185, -84, 16, 92, -19, 112, -24, -49, 23, 78, 83, 39, 13, 103, -8, -35, -7, -39, -34, -78, 99, 69, 142, -131, 117, 162, 161, 117, 70, 29, 111, 79, -187, -34, 28, 33, -257, -137, 18, 79, -127, 72}
}
, {{-20, 36, -55, -7, 32, 71, 42, -111, -55, 63, 52, -79, -20, -88, 37, -43, 202, 23, -26, -34, 68, 16, 24, -8, -52, -55, -49, -28, -3, 17, -25, 3, 70, -61, 38, -36, 36, 13, 97, 149, 230, -30, -101, 4, 16, -139, -50, 67, -61, 8, 138, 77, 112, -7, 119, -52, -101, 33, 12, -17, -129, -73, -87, -69}
}
, {{-40, 26, -53, -38, 13, 122, 10, 27, -61, 5, -11, 39, 2, -29, 19, -18, 76, -25, 8, -30, 0, 11, -41, -48, -3, -77, -91, 18, 19, -12, -70, 74, 57, 40, 15, 81, 20, -18, 29, 43, -45, -40, -5, -10, 31, 106, 10, 41, -12, -65, 71, -34, -11, 42, 81, 53, -74, 38, 4, -60, -11, 86, 7, -18}
}
, {{8, 46, -134, 24, -6, 42, -20, 45, 141, -37, 68, 60, 1, 348, 39, 141, -21, 24, -15, 277, -105, -35, 190, 70, 52, -61, -104, 72, 45, -50, -50, 36, -32, -113, -76, 15, 74, -120, 10, -34, 24, 86, -5, 84, -121, -52, 122, -13, -85, -182, 61, -132, -157, -97, 86, -13, -107, -48, -25, 73, -32, 218, 32, 96}
}
, {{41, 40, 80, -15, 6, 19, 29, 60, -65, -38, -65, 124, 39, -19, 12, 22, -174, 36, -1, 48, -21, -86, -35, 65, 60, 26, 107, -50, 10, 178, 19, -73, -30, 105, -31, 31, 95, 61, 98, -74, 59, -190, -51, 5, -54, -69, 187, -31, 44, 13, -46, -77, 5, 28, -40, 80, -112, 144, -64, -121, 58, 6, -19, -100}
}
, {{-32, -16, -85, 13, -118, -41, -23, -32, -31, -12, -10, 32, 36, -64, -11, -79, -15, -43, -18, 87, 34, 4, -18, -14, -27, -8, -6, -29, 9, -4, -18, -14, 37, -82, -39, -18, -52, -24, 54, -32, -2, -69, -19, -17, 50, -9, 124, -37, 56, -26, -30, -51, 8, 28, 34, 32, -48, 8, -8, -62, -38, 19, -75, -35}
}
, {{33, 3, 39, 148, -48, -121, -141, -25, 96, 88, -109, 34, 41, -8, 63, 84, -65, 60, -30, -55, -85, 61, 66, -175, -33, 53, 8, 92, 155, -179, -12, -62, -82, -36, -11, 94, 46, -71, -50, -32, 9, -73, 9, 52, -78, -263, 0, -35, 190, -47, 223, -14, -3, 27, -38, 24, 69, 34, -44, -90, 36, -39, -74, -76}
}
, {{25, 39, 21, -22, -107, -50, -89, 9, 74, 1, -39, 43, 9, 76, -30, -201, -128, -52, -44, -17, -16, 3, -3, 31, 1, -116, -14, -166, -7, -4, -15, -2, 62, -21, -76, 25, 3, -59, -30, -29, 26, -53, 11, -4, 18, -28, 77, 75, -19, -83, 90, -57, 12, 16, 54, -19, 55, 19, 74, -27, -30, -1, 4, -94}
}
, {{-36, -47, 19, -30, 30, 79, -27, -29, 47, 22, 10, -120, -47, -14, 82, 27, 90, 33, -12, -19, 27, 44, 0, 9, 11, -32, 18, -64, 10, 7, 0, -96, 13, 60, -39, -25, -5, 5, -4, 18, -19, -90, 59, -6, 106, -31, -23, -8, 52, -46, 11, -16, -38, 22, 14, 53, 7, 39, 37, -14, -17, 21, -70, 33}
}
, {{62, -4, -26, 48, -1, -16, 44, 42, 103, 53, 112, 14, 54, -26, 18, -46, -16, -7, -30, -19, -181, -10, -43, -5, -109, 8, -25, -13, -13, 49, 148, -100, 37, -24, 7, -8, -40, -27, 107, 71, -15, -146, 23, -62, -143, -10, -8, 172, 83, 36, 158, -227, 10, 24, 63, -71, -73, -63, -97, 42, 5, 0, -22, 43}
}
, {{95, -43, 98, -140, 215, 82, -21, 49, -22, -130, 94, -83, -98, -209, 98, -59, 11, 141, -75, 15, 52, 46, 75, 66, 28, 50, 133, 34, 123, 10, 105, 41, -167, 125, 6, 64, -136, -181, 105, -14, 42, 35, 50, 81, 87, -169, -26, -11, 57, 98, -251, 96, 49, -27, -32, -119, -59, -12, -152, 54, 158, -38, -97, 203}
}
, {{-22, 78, -66, 35, -54, -24, 30, -18, 48, -32, 82, 84, 37, 52, -51, -70, -5, -9, -6, 122, -104, 70, -29, -12, -48, -207, 82, 79, 50, 14, -18, -54, -102, 29, -53, -49, 18, -64, -80, -57, 0, -68, -18, -32, 15, -36, 53, 56, 13, -47, -108, 12, 64, -93, 60, -80, 59, 26, -18, -35, -70, -13, -53, 23}
}
, {{-75, -140, -54, 72, 131, -21, 187, -15, 10, -65, -51, 38, -132, -248, -87, 28, 41, -123, 59, 151, -55, 179, -21, 139, 171, 179, -201, 153, 129, -7, -129, 42, -6, 137, 39, 201, 247, 144, -114, -1, 69, 135, 17, 23, 28, -194, 125, -102, -47, -48, -154, -2, 77, 156, -45, -142, -12, -61, 20, -102, 147, 54, 96, -112}
}
, {{-42, 15, 37, -197, 4, 37, 164, -71, -47, -12, -151, 19, -81, -85, 67, 23, -10, -2, -29, 39, -106, -45, -28, 10, 3, 2, -211, -106, -8, -40, 48, -21, 22, -31, -14, -218, -91, -14, -75, 79, 60, -97, -38, 1, -38, 7, 67, -133, 49, 83, -158, -77, -10, -71, -206, -41, 26, -55, -45, 86, -91, 65, 65, -31}
}
, {{-6, 117, -20, 27, -155, 75, -30, -80, 46, 5, 45, -39, -32, 58, -58, -60, -9, 45, 74, 93, 3, 109, 106, 37, -148, 60, 68, -69, -70, 86, -35, 7, -55, 100, -182, -20, -61, -81, -14, -15, 50, -59, 44, -6, -85, 67, 40, 59, -29, 16, -52, 63, 100, -85, -61, -36, 26, 32, -26, 151, 4, 20, 76, -52}
}
, {{18, -6, 45, -9, -82, -20, 24, 61, 57, -41, 61, -137, -11, -24, 13, 60, 208, 81, 112, -27, -41, -34, 150, -12, 6, 10, 33, -124, -50, 42, -50, -112, 5, 69, 6, -44, 48, -117, 3, 72, 37, -153, 121, 38, 76, -45, -6, 53, -41, -66, 30, -95, -47, 27, 26, 81, 19, 32, 48, 5, 17, -68, 46, -33}
}
, {{-54, 68, -70, -57, -16, 6, 25, 19, 46, -64, -78, -66, 9, 93, 87, -65, -29, 62, 104, 74, 3, -18, 29, 183, -175, -57, 174, -50, -58, -74, 80, -51, -132, 71, 22, -82, -32, -14, -31, 94, 57, -9, -94, 6, -76, 57, 51, -17, -60, -19, 72, 32, 86, 60, -38, 15, 77, 5, 114, 127, -104, -133, -3, -40}
}
, {{101, -19, 88, 1, 66, 21, 117, 109, -32, -1, 4, -258, 25, 41, 92, 34, 8, -142, -81, 19, 43, 38, 4, -79, 47, 201, 42, -37, 2, 23, -230, 121, 5, -54, 81, 0, -39, -47, 81, -72, 17, 10, 21, -68, -83, -49, 38, -22, 143, 60, -83, 3, -22, 90, 138, -23, 1, 21, -77, 181, 205, -44, -19, -50}
}
, {{-129, -133, -70, -67, -7, 35, 18, -12, 87, -5, 51, 30, 72, 83, 119, 46, 65, -1, 49, 38, 38, -131, 151, 31, -85, -83, -3, 58, 51, -108, -9, -135, -96, 9, 86, -69, 199, -65, -109, -98, 20, 116, -58, -29, 164, 14, 68, -72, -29, -121, -49, -118, 108, 48, -13, -40, -64, -73, -60, 34, -33, -24, 88, 29}
}
, {{-26, 84, -17, 9, -78, 45, -72, 49, -58, -12, 181, 17, -28, 20, 6, -222, 109, -80, -90, 58, 118, -55, -68, -34, -81, -6, -47, -70, 55, 70, -91, -3, 29, 72, 128, -59, -33, -111, 30, 50, 176, -21, -9, 34, 25, 104, 14, -8, 126, 243, 59, 8, 179, 63, 99, -41, 27, -62, -34, 80, -91, -84, 158, 252}
}
, {{38, 27, 4, -14, -31, 41, 113, 100, -59, -38, 100, -135, 23, 86, 212, 142, -81, -150, -129, 176, -86, -37, 156, -99, 142, 219, -151, 47, 53, 59, -13, 189, 104, 46, -25, -85, 36, 97, -113, -138, -138, -97, 16, -17, 44, -58, -2, 21, 165, 9, -175, 67, 182, 56, 135, 56, -123, 19, -31, 21, 20, -16, -147, -130}
}
, {{63, 28, -30, -63, 142, 29, 153, -27, 43, -11, -58, -288, -267, -89, -1, 111, 126, 58, -141, 111, -49, 24, 122, -92, 3, 158, -12, -53, 136, -24, 10, 32, -7, -11, 43, -21, -13, -23, -101, 136, 39, 34, 71, -42, 3, -34, -2, 45, 19, -97, -18, -64, -5, 22, 8, -8, 50, 39, -92, 83, 77, -29, 43, 7}
}
, {{67, 6, 39, 15, -17, -7, 77, 49, -45, 35, 11, -109, -100, -27, 21, -26, -6, -47, -67, 6, 46, 79, -27, -189, -14, 16, -60, -61, -67, -77, -56, -61, 22, -107, -18, -12, -12, -19, -23, -23, -4, 71, 8, 28, -132, 4, -166, 25, 21, -6, -52, 103, 2, -9, 9, -110, 18, -19, -44, 2, 85, 88, 34, 35}
}
, {{239, 85, 121, 12, 1, -94, -83, -48, -60, 51, -111, -85, 94, -35, -112, -51, -61, -99, -94, 69, 48, 41, 83, -218, -119, 156, -41, 14, -95, 52, -104, -187, 94, -137, -151, -209, -61, -70, -17, 70, -144, -115, 138, -9, -106, -80, 144, -161, -47, 23, 87, -110, -133, -107, -127, -174, -3, 75, -140, 40, 240, 25, 84, -127}
}
, {{-123, -49, -68, -80, 41, 94, -25, -58, -124, -34, 7, 12, 27, 135, -137, 89, 15, -45, -27, 52, -140, -30, -106, -6, -5, -92, -96, 30, 96, -56, -63, -27, -32, -8, -98, -132, -106, -11, 37, 34, -35, 23, 4, -5, 27, -27, 69, -50, -66, -64, -49, 27, 69, -54, 18, -99, -16, -3, 74, 39, 61, 54, 8, -11}
}
, {{-52, -7, -45, -35, -16, 29, 54, -14, -1, -57, 63, -63, 42, -144, -64, -66, 86, 56, 8, -88, 48, -75, 53, -89, -153, -41, -75, -10, 15, -101, 59, 24, -75, -28, 138, -44, 91, -81, -122, 11, 53, -30, -37, -94, 41, -153, 50, -202, 68, 13, 238, -67, 41, 69, 48, -56, -75, 83, -21, -46, 36, -96, -132, -27}
}
, {{6, -61, -88, 112, -35, -82, -28, -5, 59, 14, 243, -82, 1, 29, 132, -107, -35, 72, 30, -70, -81, 95, -44, 45, -72, 41, -38, 35, -212, 76, 84, 4, -70, 27, -13, -17, -98, -51, 19, -69, 10, -179, -19, 119, 9, -126, -9, 54, 64, 71, 98, -91, -18, 61, -123, -19, 46, -37, -89, 105, -136, -69, -91, 120}
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

#ifndef _CNN_LAYERS_CONV4_DW_H_
#define _CNN_LAYERS_CONV4_DW_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       24
#define CONV_FILTERS        128
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         2

#define ZEROPADDING_LEFT    1
#define ZEROPADDING_RIGHT   1

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

typedef int16_t cnn_layers_conv4_dw_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void cnn_layers_conv4_dw(
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

#endif//_CNN_LAYERS_CONV4_DW_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "cnn_layers_conv4_dw.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#elif defined(WITH_NMSIS_NN)
#include "riscv_nnfunctions.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       24
#define CONV_FILTERS        128
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         2
#define CONV_GROUPS         128
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


static inline void cnn_layers_conv4_dw(
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

#define INPUT_CHANNELS    128
#define CONV_FILTERS      128
#define CONV_KERNEL_SIZE  3
#define CONV_GROUPS       128


const int16_t  cnn_layers_conv4_dw_bias[CONV_FILTERS] = {496, -487, -268, 86, -447, 413, -207, 480, -472, 434, 474, -25, -266, -482, -317, 419, 299, 422, -544, -336, 93, 523, 596, 523, -13, -35, -529, 509, 550, 6, 15, 639, -413, -387, -10, -61, 99, -28, -15, 582, -50, 220, 521, -101, 462, 341, -36, -408, -640, 459, 215, 485, -82, 12, 678, -397, -61, -23, -39, 423, 655, -406, 633, -25, -71, -209, -460, -479, -449, -646, -382, -23, 436, 450, 404, 337, -465, -533, 193, -172, 190, 490, 229, 563, 3, -311, 568, 278, -3, 390, 42, 432, -456, -378, 569, 366, -12, -3, -480, -43, 206, 582, 19, -380, -7, 511, 211, -470, -731, 54, -401, -441, -443, 325, -575, 8, -16, -85, -217, -77, 33, 439, -33, -49, -527, -513, 496, -485}
;

const int16_t  cnn_layers_conv4_dw_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{-7}
, {-540}
, {-697}
}
, {{324}
, {535}
, {217}
}
, {{1695}
, {3327}
, {1696}
}
, {{915}
, {-741}
, {-523}
}
, {{845}
, {764}
, {150}
}
, {{-333}
, {-67}
, {-554}
}
, {{984}
, {928}
, {-247}
}
, {{-403}
, {-671}
, {-219}
}
, {{559}
, {1070}
, {586}
}
, {{-446}
, {-202}
, {-453}
}
, {{31}
, {-634}
, {-717}
}
, {{1561}
, {-477}
, {-1241}
}
, {{791}
, {1186}
, {742}
}
, {{441}
, {428}
, {322}
}
, {{-52}
, {1207}
, {1195}
}
, {{-245}
, {-275}
, {-364}
}
, {{407}
, {-424}
, {-706}
}
, {{-348}
, {-498}
, {-216}
}
, {{421}
, {758}
, {413}
}
, {{41}
, {483}
, {593}
}
, {{-378}
, {518}
, {-614}
}
, {{-360}
, {-456}
, {-297}
}
, {{-404}
, {-522}
, {-381}
}
, {{-464}
, {-639}
, {-318}
}
, {{744}
, {100}
, {-882}
}
, {{-1218}
, {986}
, {443}
}
, {{456}
, {492}
, {292}
}
, {{-310}
, {-504}
, {-460}
}
, {{-381}
, {-534}
, {-266}
}
, {{1854}
, {-846}
, {-1083}
}
, {{1549}
, {-334}
, {-1363}
}
, {{-395}
, {-626}
, {-299}
}
, {{518}
, {848}
, {502}
}
, {{596}
, {763}
, {354}
}
, {{587}
, {600}
, {-1420}
}
, {{-1267}
, {260}
, {734}
}
, {{402}
, {-104}
, {-528}
}
, {{773}
, {-925}
, {-209}
}
, {{-370}
, {608}
, {-476}
}
, {{-447}
, {-753}
, {-408}
}
, {{-813}
, {840}
, {-410}
}
, {{497}
, {-572}
, {-580}
}
, {{13}
, {-499}
, {-548}
}
, {{363}
, {-859}
, {338}
}
, {{307}
, {-464}
, {-954}
}
, {{-589}
, {-376}
, {-88}
}
, {{1690}
, {1414}
, {-3115}
}
, {{1401}
, {1151}
, {203}
}
, {{405}
, {778}
, {589}
}
, {{-269}
, {-495}
, {-299}
}
, {{-365}
, {-541}
, {349}
}
, {{-351}
, {-379}
, {-363}
}
, {{1252}
, {1132}
, {-1760}
}
, {{335}
, {-702}
, {88}
}
, {{-501}
, {-596}
, {-524}
}
, {{855}
, {440}
, {-655}
}
, {{-536}
, {772}
, {-427}
}
, {{-766}
, {1029}
, {-620}
}
, {{668}
, {438}
, {-1250}
}
, {{-371}
, {-603}
, {-379}
}
, {{-342}
, {-534}
, {-494}
}
, {{378}
, {653}
, {469}
}
, {{-339}
, {-481}
, {-296}
}
, {{216}
, {-705}
, {337}
}
, {{-784}
, {798}
, {-213}
}
, {{-217}
, {859}
, {-588}
}
, {{474}
, {352}
, {471}
}
, {{452}
, {515}
, {252}
}
, {{1015}
, {1092}
, {341}
}
, {{430}
, {623}
, {421}
}
, {{604}
, {1072}
, {576}
}
, {{-255}
, {455}
, {-636}
}
, {{-833}
, {-494}
, {364}
}
, {{-766}
, {-621}
, {173}
}
, {{-179}
, {-720}
, {-662}
}
, {{-315}
, {-495}
, {196}
}
, {{245}
, {487}
, {386}
}
, {{397}
, {572}
, {502}
}
, {{-426}
, {-553}
, {530}
}
, {{-339}
, {981}
, {957}
}
, {{-793}
, {-430}
, {727}
}
, {{-451}
, {-496}
, {-155}
}
, {{305}
, {-383}
, {-332}
}
, {{-434}
, {-495}
, {-394}
}
, {{73}
, {-752}
, {353}
}
, {{594}
, {174}
, {-12}
}
, {{-292}
, {-612}
, {-447}
}
, {{-597}
, {-294}
, {322}
}
, {{59}
, {-1155}
, {987}
}
, {{-294}
, {-399}
, {-263}
}
, {{-424}
, {654}
, {-683}
}
, {{-291}
, {-506}
, {-276}
}
, {{577}
, {1102}
, {703}
}
, {{721}
, {652}
, {165}
}
, {{-495}
, {-913}
, {-248}
}
, {{-480}
, {-496}
, {1}
}
, {{-1483}
, {480}
, {746}
}
, {{442}
, {-933}
, {320}
}
, {{694}
, {632}
, {-177}
}
, {{-848}
, {593}
, {751}
}
, {{-813}
, {142}
, {286}
}
, {{-352}
, {-555}
, {-326}
}
, {{-489}
, {438}
, {-422}
}
, {{1463}
, {1630}
, {131}
}
, {{-551}
, {470}
, {-274}
}
, {{-374}
, {-450}
, {-285}
}
, {{-1537}
, {213}
, {1035}
}
, {{479}
, {721}
, {364}
}
, {{462}
, {732}
, {528}
}
, {{975}
, {106}
, {-1146}
}
, {{856}
, {920}
, {313}
}
, {{260}
, {447}
, {247}
}
, {{584}
, {761}
, {500}
}
, {{38}
, {-493}
, {-145}
}
, {{314}
, {604}
, {451}
}
, {{-920}
, {591}
, {133}
}
, {{-1107}
, {372}
, {617}
}
, {{429}
, {-920}
, {378}
}
, {{571}
, {499}
, {-352}
}
, {{421}
, {-933}
, {378}
}
, {{-367}
, {-349}
, {323}
}
, {{-427}
, {-515}
, {-304}
}
, {{-675}
, {-20}
, {439}
}
, {{-1148}
, {188}
, {815}
}
, {{375}
, {555}
, {300}
}
, {{942}
, {1100}
, {678}
}
, {{-453}
, {-631}
, {-309}
}
, {{656}
, {794}
, {337}
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

#ifndef _CNN_LAYERS_CONV4_PW_H_
#define _CNN_LAYERS_CONV4_PW_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       12
#define CONV_FILTERS        128
#define CONV_KERNEL_SIZE    1
#define CONV_STRIDE         1

#define ZEROPADDING_LEFT    0
#define ZEROPADDING_RIGHT   0

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

typedef int16_t cnn_layers_conv4_pw_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void cnn_layers_conv4_pw(
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

#endif//_CNN_LAYERS_CONV4_PW_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "cnn_layers_conv4_pw.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#elif defined(WITH_NMSIS_NN)
#include "riscv_nnfunctions.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       12
#define CONV_FILTERS        128
#define CONV_KERNEL_SIZE    1
#define CONV_STRIDE         1
#define CONV_GROUPS         1
#define CHANNELS_PER_GROUP  (INPUT_CHANNELS / CONV_GROUPS)
#define FILTERS_PER_GROUP   (CONV_FILTERS / CONV_GROUPS)

#define ZEROPADDING_LEFT    0
#define ZEROPADDING_RIGHT   0

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


static inline void cnn_layers_conv4_pw(
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

#define INPUT_CHANNELS    128
#define CONV_FILTERS      128
#define CONV_KERNEL_SIZE  1
#define CONV_GROUPS       1


const int16_t  cnn_layers_conv4_pw_bias[CONV_FILTERS] = {-1098, -122, 381, -236, -325, -160, 567, 593, -254, 179, -491, 497, 369, 589, -145, -411, -473, -462, -389, -102, -119, -538, 62, -380, 59, -141, 482, 901, 622, -227, -519, 383, -78, -92, -454, -100, -214, -654, -710, 273, -281, -882, 643, -163, 219, 56, -983, -482, -91, 313, -101, 100, 171, -397, -176, 240, 855, -395, 67, -552, 728, -798, -331, 143, 70, -314, -206, -199, 255, -588, 156, 712, 14, -259, -456, -300, 214, -200, -226, -559, -648, -71, -168, 350, 123, -486, 121, 763, -678, -546, 68, -866, 255, -572, 454, -340, 75, -848, 37, 372, 241, -398, -237, 32, -233, 569, 672, 475, -659, -511, 340, 648, 298, 479, 524, -765, 505, -668, -163, -430, -298, -16, 521, -431, 321, -166, -528, 13}
;

const int16_t  cnn_layers_conv4_pw_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{93, 20, 23, -123, 190, 7, 72, 138, 28, 49, 148, -46, -133, 44, 114, -17, -23, -17, 79, 86, -81, -52, 27, -9, -191, 22, 0, -35, -12, 105, 282, -68, 8, 87, -99, 87, -202, 27, -206, 42, -33, 48, 123, -31, 209, 225, -33, 161, -234, 208, 163, 221, 110, 21, -64, 4, -126, -30, 37, 63, 110, -32, -91, -85, 44, -76, -101, 3, 166, -78, -118, -55, 36, 119, 10, 181, -59, -2, 70, 27, 161, -28, -86, -156, -64, -46, 76, -31, -132, 245, 26, 5, 109, -9, 24, -77, -32, -147, 151, 18, -74, 199, -36, -71, -238, 73, 253, -140, -11, -187, 231, 6, -160, -3, 149, -152, -29, 6, -45, -164, 101, 199, 140, -2, 177, 34, 159, -24}
}
, {{-8, -223, 55, -157, -21, 18, 130, 146, 49, -85, -59, -64, -59, 1, -61, -47, 176, 124, -252, 107, 18, 62, 85, -167, -158, -187, 74, -84, -171, 79, -92, 50, 35, 220, -136, -10, 4, 61, -1, -171, -55, 174, -102, -85, 44, 80, -38, -186, 217, -73, -87, -84, -131, 181, 144, 136, -125, -92, 209, 15, 6, -189, 213, -164, -49, -1, -14, -99, 129, -123, 219, -107, -27, -221, 37, -54, 126, 27, 230, 163, 174, -173, -333, -8, 63, 42, -84, -44, 55, 301, 264, -177, -125, -117, -14, -161, -89, -91, 107, 71, 101, 55, 154, 55, 2, -63, 176, 220, 94, -34, 16, -28, 148, -21, 15, -19, 37, -144, 100, -67, 233, 71, 100, -20, -58, -43, 86, 11}
}
, {{33, -43, 37, 59, -108, 28, -135, -116, 138, 69, 8, 6, -87, -17, 46, 66, -27, -13, 161, 16, 376, -4, -342, 11, -61, 182, 104, 103, 105, 133, -35, 135, 78, 14, 76, -232, 189, -139, 72, -12, 78, -104, -161, 215, 26, -122, 100, 40, 43, -141, -19, 26, 185, -130, 60, -44, 383, 253, -186, 106, -20, -167, 131, 181, -92, 8, -85, -68, 97, 256, -36, 21, -193, 106, -36, 18, 3, 129, 30, -138, -15, 110, -134, 32, -140, 33, -72, -161, 146, -152, -218, -16, -183, -92, -170, -146, 102, 282, 206, -120, -164, -116, 41, -62, 94, 123, -413, -2, -52, 172, -182, -158, -43, -11, -98, 220, 18, 133, 36, 253, -125, 170, -61, 172, -20, -83, -231, 98}
}
, {{-107, -183, 132, -92, -120, 56, 24, -59, 9, 68, 98, -23, -2, 35, -49, -8, 105, 198, -59, -62, 63, -98, -93, 18, 5, -62, 110, -107, 9, -68, 251, 36, 136, 37, -239, 15, -2, 75, -22, -107, -99, -29, 140, -80, 4, -54, 74, -30, 114, 88, -91, 93, -23, -47, 274, 77, -35, -80, -58, 60, 0, 64, 58, -140, 116, -50, -33, 167, 128, 19, 84, 49, -73, 69, -52, -226, 96, 26, 104, -49, 79, 18, -40, 51, -200, 34, 24, -158, 36, -51, 131, 106, -44, -175, -36, 49, -84, -4, -23, -142, -193, 118, -67, 141, -23, 16, 113, 5, 35, 18, 121, 11, -29, 69, -45, 25, 31, -23, 129, -56, 108, 12, 47, 46, -22, 40, -123, -170}
}
, {{117, 96, 125, -96, -74, 65, -257, 7, -32, -207, -109, -278, 81, 174, -84, -221, 263, -30, 53, -31, 215, -121, -14, -6, 14, 117, -52, -96, -181, -81, 239, -116, 89, 89, 17, 133, 135, 325, 69, -185, -100, 149, 117, 103, -2, -236, 123, -191, -67, -81, 35, 22, 14, -88, -190, 155, 171, 121, 87, -177, -18, 57, -115, 84, 195, 1, 24, -89, -127, -10, 104, 230, -31, -220, 34, 139, 39, -49, 176, -70, -31, 23, 186, -11, 160, -110, 64, 107, 147, 175, 200, -136, 127, 102, -71, -14, -5, 85, 11, 190, 118, 56, 6, 97, 138, 14, -69, 24, -77, 151, 175, -54, -64, 143, -34, -70, 176, 122, 208, 76, 75, -224, 48, -65, -38, 140, 80, -30}
}
, {{66, -5, -49, -110, -108, -129, 184, -21, 57, 93, -3, 266, 64, 46, -63, -152, -49, -124, 203, 11, -22, 76, 97, -1, -60, -30, -37, -30, -111, -22, 95, 194, -87, -30, 18, 56, -37, -96, -160, 57, 39, 12, 38, -50, 115, 206, 31, -5, -220, 14, -63, -153, -26, -21, 33, 9, 13, -69, 171, -47, 66, -128, 114, -65, -185, 72, 1, -109, -125, 118, -27, 19, 6, 210, 101, 10, -65, 102, -125, 175, 103, 74, -27, -126, -5, 80, 114, 24, -37, -59, -137, -135, -120, -37, 94, -44, 171, -83, 40, 54, -135, -88, -88, 40, -28, -318, 188, 3, 37, 78, -68, 140, -2, -21, 36, 127, 50, -120, -29, -59, 89, -26, 46, -44, -51, -29, 180, 75}
}
, {{128, 56, -59, 9, -144, 21, -59, -42, 49, -122, -40, 99, 105, -138, 84, -189, -65, 15, 73, 125, 86, -43, 2, 53, 156, 183, -105, -77, -26, -207, 57, 71, 68, 51, -180, 3, 81, -7, -183, 37, -84, -300, 43, -41, -37, -199, 172, 79, -36, -19, -104, 23, 93, -153, -92, -107, 34, 96, 63, -179, -10, -46, -65, 40, 4, -79, -28, 27, 85, 116, 76, 25, 6, 52, -167, -147, 128, -30, -114, -25, 86, -29, -29, 128, 87, -25, -117, 122, 22, -2, -132, -97, 79, 30, -73, -269, -82, 7, 51, 25, -46, -77, -123, -56, 68, -121, 87, 43, -116, -48, 14, -40, 71, -64, -178, 115, -177, 46, 263, 34, -84, -99, 86, 163, 84, -128, -19, -68}
}
, {{76, -44, 83, -97, 75, 63, 175, -7, 27, -71, -42, -30, 86, 171, -59, 0, -49, 85, -226, -250, 50, -420, 24, 43, 59, -18, -91, 12, 193, 426, -74, -100, -104, 1, 35, -39, 199, 130, 31, -43, 26, -58, 119, -39, -21, -116, 27, -11, 4, -10, 134, -320, -164, 204, -57, 263, 52, 21, 45, 126, 185, -62, -109, -88, 82, 38, 4, -187, -136, -110, 76, 101, -122, 114, -75, -120, -124, -19, 30, -94, -91, 17, -144, 161, 27, 112, -48, -68, 152, -8, 59, -90, 52, -1, 24, -29, -41, 61, -92, -180, -287, -71, -45, -97, -65, -128, -146, -7, 166, 182, 194, -326, 133, 104, 34, 57, 281, -32, -31, 89, 27, -19, 17, 30, -105, 96, -19, 91}
}
, {{51, 50, 176, -129, -124, 215, -11, 181, 18, 19, -33, 71, 87, -48, 50, -151, -119, -80, -117, 161, -53, 95, 97, -79, -34, -36, -142, -76, -95, -66, 123, 12, 134, 0, -17, -82, -145, 30, 14, 102, -75, -183, 225, 18, -45, -34, 123, 159, 114, -171, 92, -115, 8, 35, -109, 19, -104, 25, 78, 17, -306, 90, 295, -59, 36, -8, -47, 14, 173, 112, -86, 70, -143, -32, 36, -35, -11, -10, 85, -50, -98, 201, 127, -106, -15, 87, 39, 82, 157, 187, -55, 137, 89, 75, 163, -117, -69, -130, 16, -164, -226, 16, 130, -153, -34, -36, -272, 180, -40, -87, -42, -60, 12, 108, -21, -57, -5, -27, 298, -70, 9, -7, 183, 112, 281, 12, -41, 109}
}
, {{43, -56, 45, -222, 77, -27, -96, -172, -7, -25, -100, 106, -99, 32, -5, 24, 153, 140, 140, -16, 148, 105, 22, -180, -76, 69, 36, 36, -207, 117, -179, -60, 85, 52, 20, -211, 22, 74, -12, -219, -66, -8, -13, 190, 60, -55, -211, -74, -211, -257, 49, -35, 27, 228, 119, -138, 106, -81, 66, 63, 54, -58, 103, 128, -109, -95, 64, -34, 42, -60, 148, 193, -9, -23, 164, -4, 16, -149, 2, 300, 247, -48, -26, -4, 279, 62, -161, 135, -19, 15, 92, -32, -3, 134, 18, 29, 109, 44, 74, 207, 6, -249, 255, 30, 150, 179, -324, 39, 101, -164, -233, 68, -41, -52, -184, 37, -59, 60, 52, 33, 106, -124, 43, 24, -140, 45, -190, 104}
}
, {{-2, -62, 148, -134, 57, 4, 85, 94, 34, 182, -5, -56, 42, 15, 83, -5, 77, -120, -77, -86, 162, -143, -304, 21, -108, 104, 12, -113, 42, 128, 298, -101, 156, 118, -10, 111, 40, 112, -88, -126, 24, 269, 54, -146, -91, -95, -238, -158, -61, 105, 81, 105, 17, 194, 84, 171, -21, -134, -71, 126, 64, -76, -54, 41, -140, 82, -10, 6, -100, -214, 81, -123, -167, 115, 90, 61, -96, -122, 97, -59, -3, -101, 9, 20, 31, -8, 57, -178, -182, 7, 210, 27, 116, -155, 49, 50, 204, -7, 23, -39, 29, 121, -94, 192, 35, 57, 147, 50, -3, 74, 169, -45, 137, 190, 120, -116, 65, 71, -60, 134, -127, 20, 107, -46, 149, 76, -57, -11}
}
, {{-168, 142, -22, 6, -81, 102, -92, 85, -92, -105, -67, -131, 137, -88, -57, -40, 69, -25, 220, 81, 180, 212, -90, -8, 70, -22, -57, 154, -164, -75, 73, 16, -31, -31, -105, -19, -16, 35, -186, 59, 157, 54, -17, 161, -38, -41, 111, 77, -186, -78, -128, -29, -34, 62, -23, 92, -36, -7, -80, -93, -161, 35, 150, -103, 8, -56, -40, -2, -23, 87, 48, -9, -61, -75, -78, 5, 33, 195, 17, 30, 171, -71, 35, -331, 0, -94, 46, -15, 17, -129, -146, -46, -17, -74, -57, -68, -199, 20, -57, 10, -94, -110, 20, -1, -46, -55, 366, -44, -80, -196, 32, 105, -46, -93, -214, -18, -150, 0, 93, -84, -28, 31, 44, -61, 78, -18, 77, -105}
}
, {{-12, -91, 114, -206, -102, 113, -27, -159, -7, -60, -36, 60, 82, -35, 113, -268, 70, 163, 159, -66, 148, 88, -149, -97, -248, 70, 101, 161, -119, 17, -186, -47, 189, -60, -132, 50, -13, -73, -35, -332, 47, -177, -6, 3, 158, -166, -224, -45, -162, -98, 3, -12, 38, 186, 51, -140, 13, -30, 37, -72, 225, -113, 27, 57, -160, -58, 104, -100, 25, 74, 96, 112, 44, -70, -55, 20, 80, 7, 26, 303, 145, 69, -194, 45, 220, 37, 10, -100, -77, -26, 4, 11, -48, 19, 197, -41, -31, 90, 49, 5, -41, -260, 274, 177, 161, 81, -97, 106, 56, -246, -74, 114, 190, -73, -172, 3, -64, -75, -70, 84, -147, -54, 23, -74, -150, 33, -24, 88}
}
, {{48, -79, 15, 394, 152, 203, 181, -163, 279, -219, 2, -114, 191, -69, -50, 93, -157, 36, 74, 4, 68, -122, -469, 171, 14, 63, -30, 64, 151, 211, -152, 57, -246, -46, 67, -5, 241, 50, 84, 119, 308, -54, 100, -139, 180, -32, 97, -76, -56, 195, -78, -266, -83, 7, -34, 124, -17, 115, -7, 102, -256, -115, -7, -159, 31, -74, -9, -286, 106, 32, 69, 87, -51, 258, -10, -139, -32, -144, -147, 64, -97, -199, -45, -83, -64, -2, -27, 20, 160, -111, -97, -209, -194, -129, 36, 308, 11, 177, -205, -140, -388, -120, -35, -116, -108, -265, -35, 163, 108, 110, 182, -158, -59, 79, 75, 273, 158, 66, -106, 60, -62, 202, -104, 16, -108, 116, 66, -252}
}
, {{126, -27, -16, 47, -147, 99, -151, -7, 97, -238, -136, 146, -76, -59, 154, -108, 350, 138, 95, -220, 330, -92, -125, 152, 25, 75, 1, 228, -234, -106, -126, -147, 63, 82, 142, -176, 170, 69, 120, -91, 144, 38, -9, 20, 122, -111, 10, -144, -157, -91, -88, 59, -131, 21, -193, -121, 164, 121, 284, -18, 88, 147, -328, 166, -31, -33, 94, -50, -293, -56, 231, 192, 170, 1, 257, 175, 56, -49, 91, 147, 138, -158, 313, -140, 362, -39, -135, -2, 139, -261, 27, -94, -4, 81, 43, 247, 69, 247, -91, 80, 174, -9, 3, 92, 308, -17, -112, -101, 53, -317, 105, 75, 72, -12, 70, 22, -52, 104, -45, 182, -142, -238, -158, -176, -248, 33, -10, -83}
}
, {{174, 66, 46, -3, -162, -98, -89, 272, 60, -221, 44, 189, 93, 18, -85, -228, -15, -12, 150, -19, 133, -69, -6, -199, -103, 41, -81, -14, -125, -18, 331, 146, 125, -17, -31, 198, 97, 17, -111, -69, -23, -42, -189, 47, 16, -82, 9, 2, 11, -66, -179, 89, 105, -52, -46, 27, 4, 90, 71, 39, -1, -85, 67, 168, -31, -59, -81, 134, 132, 92, -34, 34, 143, -65, 37, 26, -60, -186, -130, -27, 247, 139, -102, -195, 53, -82, -5, 102, -27, 212, 27, 114, 46, 28, 23, -201, -153, 31, 34, 8, 173, 25, 45, -162, 143, 84, 183, 195, -246, -158, 145, -16, -134, 109, -74, 13, -38, -11, 262, 124, 126, 38, 300, -77, 62, -9, -141, -158}
}
, {{-3, 110, -22, 163, 77, 32, 66, 102, -68, 152, 70, 11, -29, 174, 111, -121, -89, -85, 2, -5, -113, -112, -310, 17, -252, -16, 167, 37, 155, 99, 119, 53, 173, 0, -14, 146, -157, -31, -61, 75, 28, -86, 157, -108, -67, -83, -35, 171, 1, 244, -157, 208, 136, -43, 90, 18, -93, -141, 89, 248, -101, 93, -111, -99, -144, 27, -57, -48, 38, -84, -45, 29, 77, 150, -35, 217, -168, 77, -52, -120, 86, 34, 102, -59, -134, -12, -47, -39, -30, 74, 27, 270, 164, -43, 31, -15, -77, 44, 32, -93, 47, 2, -155, 35, -123, 8, 60, -97, -103, -11, -162, -89, 62, -20, 72, -65, -101, -29, 49, -99, -49, 145, 162, 91, 283, -84, -131, 22}
}
, {{1, -204, 121, -139, 219, 106, 268, 96, -69, 41, -145, -55, 33, 114, -82, 251, 13, 167, 55, -100, -37, -4, 54, 13, 41, -25, 19, -121, 127, 173, -87, -67, -58, 206, 91, -114, -60, 83, 135, -227, -48, 38, 34, -79, 76, 32, 72, -163, 254, 9, -47, -26, -176, 15, 218, 158, -106, -41, 6, 70, 59, -49, -58, -159, -18, 153, 72, -178, -25, -89, 130, -220, -78, -90, 69, -4, 78, 118, 190, -6, 23, -73, -302, 36, 62, 176, -161, -172, -32, 272, 202, -44, -1, -178, 61, -28, -130, -70, -120, -77, 62, 211, 177, 107, 1, 107, 15, 75, 104, 43, 72, -115, 165, 141, 90, -241, 230, -174, -66, 89, 79, -38, -90, -182, -66, 47, 65, 80}
}
, {{-2, -95, 216, 184, 67, 40, -86, -58, 210, -107, -10, 151, -11, 129, -105, 25, 16, 125, 13, 42, 20, -210, 70, 49, -148, -163, -71, 170, 23, 39, -151, 14, -189, 120, 94, 19, 77, 116, 49, 59, -16, 28, -9, 44, 97, 16, 311, -54, 59, 209, 100, 172, -108, 34, -67, 114, 76, 131, 292, 38, 157, -42, 177, -28, 143, -83, 67, -116, 115, 130, 95, -140, -132, -29, -180, -348, 51, -56, 201, 166, 25, -110, 15, 20, 42, -27, -57, 17, 185, -137, 2, -243, -197, -221, -189, -169, -95, -11, 93, 19, -177, -106, 71, -25, 11, -169, 239, 96, 8, -5, 33, -113, 205, 129, -114, 103, 192, 129, 157, 61, -26, -1, 4, 83, -106, 114, -121, -29}
}
, {{75, 11, -32, -127, 56, -24, 91, 22, 15, -69, 85, -95, -12, 8, 41, 201, 28, -252, 12, -203, -41, 161, -143, 178, -93, -128, 45, 45, -242, 226, 32, 15, 51, -83, -12, 89, 2, -110, -127, 177, 27, 82, 142, -88, 337, 238, -96, -14, -193, 16, -78, -125, -22, 35, 22, 86, -127, -52, -54, -102, -37, 69, -38, -148, -96, 71, 137, 3, -61, -70, 5, -61, 98, 119, 220, 96, -188, -19, 13, 78, -58, -347, 6, -97, -103, 39, 188, -118, -109, 49, -44, -169, 78, -33, 238, 36, 110, -21, -83, 10, -134, 107, -71, -44, -81, -135, 93, -196, 103, -14, 179, 128, 119, -112, 179, -33, 101, -60, -96, -50, 64, 87, 82, -191, -4, 37, 125, 60}
}
, {{155, -161, -6, -72, 104, 220, 218, 226, -1, 134, -34, -101, 38, -306, 45, 60, 436, 79, -113, -17, 148, 55, -159, 41, -8, 34, -87, -205, -89, 58, 54, -131, 187, -59, -107, 92, -29, -190, 115, -173, 308, 3, -28, -191, 168, 173, -107, -133, -34, 23, -2, -57, 116, 73, 182, -2, -50, -51, -24, -95, -280, -328, 161, -108, -321, -17, 180, -28, 269, -135, 81, 103, 4, 115, 161, 52, -50, -132, 82, 67, 1, -243, -26, 48, 87, 180, 20, -171, -105, 37, 65, -107, 8, -109, -24, 34, 165, 125, 43, 104, 8, -177, 164, 79, -142, 143, -201, 100, 157, 43, -137, 104, -128, -173, -59, 29, 29, -117, -114, -103, 170, 110, 87, -185, -84, 12, 148, 70}
}
, {{-59, -121, -26, -31, -58, -145, -21, 5, 36, -71, 110, 220, -83, 120, 40, -72, -103, 131, 149, -43, -216, 138, 53, -49, -135, -107, -6, 160, -49, 6, 45, 115, -155, 8, -97, 182, -102, -72, -157, 134, 20, -48, 127, -118, 44, -4, 11, 107, 61, -29, 105, 14, 59, -36, -62, 92, -88, -63, 61, -90, 211, -145, 105, -9, 23, -72, 86, -249, 112, 76, -96, -42, 126, -42, -131, -52, -67, 82, 65, 102, 48, -43, -101, -149, -116, -56, 36, 11, 88, 119, 51, -139, 29, -209, 90, -18, -91, -124, 65, 68, 246, -40, 11, 32, -11, -133, 290, 215, -297, 129, 101, 188, 48, 64, 78, -158, 69, -36, 7, -99, 151, -11, 172, 177, -44, -87, 246, 24}
}
, {{-75, -124, -57, -70, -143, -82, 145, -18, -38, 64, -73, 95, 52, -73, -14, -31, -80, 59, 184, 9, -135, 76, 126, -138, 26, -132, -141, 161, -55, -146, 17, 89, -140, -50, -15, 13, -25, -88, -66, 110, 57, -45, 44, 0, 42, -50, 62, 74, -44, -175, -49, -118, -102, 1, 30, -51, -57, -125, 106, -307, -7, -154, 126, -26, -5, 9, 47, -41, 149, 131, -111, 80, 20, 97, -39, -53, -40, 243, -158, 153, 109, -81, 14, -119, -23, 35, 161, 24, 75, -95, -27, -44, -52, -58, -24, 34, -86, -63, 89, 110, 144, -177, 134, -4, -20, -67, 211, 76, -196, 174, -6, 209, -80, -29, -69, 9, 62, -197, 45, -95, 229, 61, 74, 83, -35, -131, 174, 67}
}
, {{-121, -4, 101, -34, 71, -26, 144, 81, 27, 73, 72, -40, -56, -3, -20, 168, -20, -51, -67, -50, -60, -159, -183, 15, -53, 32, 11, -235, 39, 114, 352, -25, 102, 19, -61, 200, -123, 65, 133, -40, -42, 153, 83, -144, 45, -98, -186, -13, -32, 72, -136, 186, 94, -68, 178, 129, -176, -207, 45, -39, -28, 2, -26, -144, -75, 128, -133, -38, 38, -104, -90, -54, -106, 125, 30, 92, 24, 43, -31, -54, -4, 68, -166, -59, 47, -33, 43, -115, -197, 105, 139, -77, 210, -204, -46, 21, 244, -121, 67, -79, -54, 267, 48, 147, -197, -120, 92, 6, -70, 25, 135, 13, 160, 136, 130, -153, 49, 44, 38, 65, -21, 42, 124, 7, 260, 36, 122, -34}
}
, {{126, 61, 172, -158, 56, -32, -22, 48, -130, 37, 155, -131, -212, -106, 250, 356, -163, -17, -171, -10, 35, -135, -86, 24, -63, -22, 81, 180, -148, 29, -265, -80, -149, -11, -112, -17, 6, -100, -208, 71, -94, -111, 310, -111, -165, -160, -76, -122, -10, 67, 198, 168, -86, 32, 133, -32, 29, -51, -102, 95, 205, 160, -240, 89, 75, -41, 61, -12, -209, -116, 32, 79, -303, -5, -42, 67, 68, -55, 136, 7, 127, -11, -1, -27, 39, 53, 90, -127, 67, -165, 183, 1, -51, -75, 67, 103, -210, 35, -36, -31, -62, -16, -40, 57, 80, 301, 109, 73, 78, -184, 160, -92, 99, 2, 52, -61, 29, -38, 4, 29, 88, -90, 105, -58, 39, 62, 57, -114}
}
, {{108, -220, -22, -261, -174, 69, 29, -101, -56, 46, 17, -1, -43, 19, -19, 218, 181, 150, -89, 54, 13, 72, 82, -486, -194, -59, -109, -109, -277, 121, -368, 102, 82, 83, -210, -201, 74, 247, -35, 40, -91, -188, -25, 70, 78, 89, -157, -164, 250, -203, 4, -121, -81, 375, 41, 77, -132, -170, 75, -33, -51, -154, 183, 115, -110, -45, -61, -76, 132, -43, 229, 131, 57, -28, -5, -138, -123, -52, 79, 202, 98, -68, -135, 198, 107, 68, -36, 146, -7, 147, 248, 137, -34, 61, -59, 83, 50, -14, 75, 164, -23, -106, 205, 41, 191, 73, -92, 164, 123, -281, 69, -74, 3, 115, 81, 116, 75, -6, 224, -121, 285, 110, 47, 9, -14, -23, 50, -62}
}
, {{68, 39, -10, -118, 86, -102, 199, -140, 15, 29, 38, 62, 137, -26, 94, 34, -171, -279, 173, -27, -81, 128, -87, -13, -21, -26, -60, -13, -226, 75, 8, 39, -190, -273, -79, 249, 16, -2, -121, 97, 86, 107, 70, -44, 198, 151, 32, -48, -310, -63, -131, -242, 20, -3, 14, 134, -80, -26, 93, -123, -86, -117, 8, -184, -37, 91, 157, -217, -115, -214, -151, 36, -54, 65, 289, -92, -1, 163, -86, 22, 27, -82, 47, -107, -39, 193, 230, -122, -88, 108, -18, -216, 63, -145, 144, -40, 191, -30, -24, 31, -155, 27, 62, 141, 32, -353, 296, -168, 49, -43, 78, 88, 23, 26, 133, -10, 47, -152, -61, -110, 10, -27, 72, -15, -21, -10, 155, -47}
}
, {{73, -25, 28, -102, -90, -227, 234, -245, 186, 72, 18, 173, 137, -122, -166, -104, 95, -268, -87, -13, 86, -122, -188, -231, -136, 169, -180, -64, -337, 65, 21, 2, 5, -30, -70, -184, 44, -93, -105, 153, 169, -89, -44, 110, -57, 115, -291, -66, 34, -43, -37, -47, -100, 84, -198, -194, 181, 26, 78, -7, -231, -159, 71, -11, -218, 17, 114, -43, 89, 136, -34, 259, 16, 55, -63, 14, -43, 123, -208, 3, 192, 104, 109, 154, 130, 83, -160, 96, -22, 126, -82, 14, -47, 151, -162, -98, 271, 92, 105, 147, 100, -215, 185, -250, 161, -56, -175, 155, -135, 73, -40, 85, -24, -84, -101, 56, 151, 66, 42, 148, 44, -105, -28, -34, -70, -52, -324, 12}
}
, {{48, -19, -21, -78, -140, -116, 60, -67, 146, -14, 116, 281, 77, -102, -88, -66, -77, -140, 97, 117, 203, 26, -249, -88, -145, 116, -181, 40, -111, -102, 111, 133, 42, 12, -82, 67, -16, -146, -137, -41, 95, -109, -91, 87, -23, 5, -24, 13, 75, -125, -186, -98, 71, -86, -7, -24, 133, 63, 80, 11, -99, -124, 76, 79, -48, 34, -87, 47, 152, 203, -46, -4, -92, -74, -38, -22, 5, 54, -182, -22, 150, 63, -116, -64, 42, 59, -167, 132, 8, 26, -171, -22, -193, 40, -54, -314, -99, 54, 110, -89, 133, 9, 48, -185, 31, -97, 91, 129, -150, 38, 63, 44, 22, -129, -172, 79, -152, 124, 172, 173, -4, -25, 113, 212, 64, -132, -108, -107}
}
, {{-50, 43, 96, -42, -205, -25, 37, -99, 179, 59, 180, 207, -119, 101, -118, 209, 27, 85, 66, -32, 170, -178, -50, -51, 40, -240, -216, -54, 3, -53, 5, -57, -81, 35, 90, 18, -12, -260, -80, -33, -163, 263, -83, 121, -122, -135, 207, -76, 118, 278, 14, 154, -112, -193, 141, 140, 160, -11, 13, -189, 157, -11, 100, -85, -19, 86, -204, -128, -15, 334, -77, -311, -128, 43, -33, -28, -26, 27, 3, 138, 42, 115, -61, -167, -102, -7, 156, -43, -28, 18, 107, -214, -90, -147, -279, -99, 74, -28, 55, 53, 11, 176, -98, -55, 77, -6, 146, 107, 79, 268, -3, -246, 276, 214, -87, 0, 261, 56, 9, 290, -3, -68, -14, -8, -155, 139, -39, -6}
}
, {{44, 95, 98, 27, 305, 98, 170, -181, 149, 39, 153, -289, -77, 161, -75, 146, -100, -104, 161, -221, 12, 24, -115, 9, 130, 57, -35, -69, 180, 201, -56, -10, -236, -181, 76, 248, 23, -128, 7, -23, -40, 115, 91, 76, 62, 35, -61, 128, 125, 305, -29, -141, -51, -85, -157, 98, 20, 88, -195, -80, -141, 35, -265, -32, 90, 267, 85, 101, -91, -23, -81, -121, -17, 84, 26, 52, 27, 183, 49, -135, -170, -62, 12, 100, -229, -26, 86, -20, -81, 306, -10, -14, 17, -29, -23, -9, 179, -49, 88, -63, 177, 295, -59, -103, -160, -78, -91, -79, 70, 123, 134, 37, 54, -103, 192, -173, 221, 174, -103, 153, -136, 34, 52, 68, -126, 219, 72, 26}
}
, {{-18, 90, 185, 38, 75, -33, 176, -78, -166, 52, 12, -193, -151, 230, 14, -27, -273, 63, -84, 49, 15, -174, 79, 55, 30, 127, 71, -401, 170, 51, 118, -50, -78, -135, -124, -46, -61, 125, 74, -160, -180, -86, 160, -75, -34, -218, 134, 1, -4, 57, 110, -16, -76, 15, 35, 34, 86, -47, -159, 126, -13, 63, -78, 90, 214, 145, -138, -114, 15, -1, 65, -82, -257, 190, -218, -89, 90, 197, 71, 2, -35, 149, -69, -2, -56, 59, -31, -59, 31, 21, 173, -180, 197, -208, 10, -147, -61, -87, -56, -165, -75, 177, -48, -39, -83, -32, -92, 205, 120, 70, 50, -139, 51, 98, 36, 39, 141, 64, 93, 165, -33, -28, 75, -32, -88, 135, -48, 111}
}
, {{126, -52, 71, -31, -27, 0, 29, 184, -87, -25, -207, 215, -129, -90, 137, -34, 205, 140, -5, -218, -104, 84, 110, -13, 60, -17, -42, 32, -9, 32, 111, -134, 107, 195, 55, 137, -105, -35, 113, -49, 162, 220, -129, 80, 66, 123, -137, 46, 29, -228, -226, -45, 47, -37, 101, 96, -11, -4, 58, 79, 154, 107, 103, -12, -27, -9, 130, 20, 40, -99, -5, -60, 80, -72, 195, -56, -140, -70, -35, 94, -182, -154, -22, -98, -5, 52, -18, -30, -104, -150, -306, 39, 162, 114, -122, 54, 161, 81, -55, 78, -49, -234, 159, 112, -90, 36, -337, 20, 130, 153, -41, -121, 92, 94, -210, 66, -31, -100, -19, -77, 128, 58, -126, -66, -245, 105, 44, 158}
}
, {{-110, 95, -35, 55, -108, 55, -143, -42, 78, 107, -15, 2, -129, -2, 262, 103, 13, 58, 53, -71, 158, 202, -405, 108, -110, 62, 105, 172, 50, 37, -34, 9, 27, -23, 29, 6, -138, -230, 29, -201, 350, 33, -256, 54, 138, -89, -162, -134, 52, -37, 48, 99, 189, 91, 233, 18, -33, 117, 58, 123, 116, -73, 123, 77, -66, -45, -88, -59, 125, 44, 80, -204, -114, -171, 51, 48, 36, 157, 159, 82, 17, -35, -62, -111, -165, -80, -9, -202, 64, -41, -45, -62, -82, 17, -150, -22, -3, 426, 154, -230, -64, -140, -69, 92, 16, 193, 31, 41, -61, 62, -145, 6, 189, -102, -203, 71, 19, 180, -60, 187, -151, 179, 20, -58, -252, 214, -62, -8}
}
, {{-145, -16, -92, 35, 42, 47, 103, -97, 117, -7, 136, -34, -19, 47, -220, -56, -229, -19, 250, 240, 64, 88, -30, -149, -56, 61, -138, 178, -79, -6, 52, 45, -135, -29, 140, 198, 129, -226, -155, 60, -144, -32, -149, 298, 91, -52, 99, 162, 139, -192, -210, 72, -109, 121, -190, 29, 300, 187, 7, -249, -77, -51, 21, -77, 4, 143, 56, 120, -150, 175, -147, 121, 45, 73, -59, 22, 17, 221, -77, -214, 28, 183, -70, -247, -190, 1, 198, 173, 201, 117, 63, -98, -138, 122, -108, -32, -68, -91, 37, 9, 197, 61, -43, -228, -134, 51, 94, -98, -162, 224, -37, 120, -120, 70, -25, 44, 163, 151, 22, 153, -59, -4, 11, 167, -11, 226, 221, 196}
}
, {{-11, 19, -44, -77, -19, 199, -69, 203, -210, -16, -216, -37, 285, 21, -63, -158, 138, 149, 141, 140, -32, 173, 126, -32, 41, 68, -168, 82, -355, 14, 137, -62, 112, 207, 138, -297, 105, 157, -94, 43, 130, 92, 113, 153, 100, -97, 134, -83, -77, -269, -76, -77, -66, 169, -177, -168, -75, 153, 143, 147, -303, 36, 375, -95, 68, -117, -73, -63, 13, 82, 101, 268, -193, -7, -61, -56, -100, 183, 49, 33, 71, 42, 104, -340, 91, -59, 166, 245, 92, -10, 73, 158, 31, 224, -124, 231, -29, 168, -128, 108, -104, -203, 117, -103, -82, -124, -26, -146, -174, -127, -12, 120, -355, -18, -234, 87, 32, -160, 176, -279, 108, 118, 4, -124, -50, -2, 129, -121}
}
, {{-6, -212, 160, -203, -50, 173, 109, -132, -4, -6, -162, -62, -56, 121, -51, 65, 153, 184, -210, -170, -55, 56, 136, 21, -11, -182, 162, -65, -42, 300, 7, -89, 89, 203, -116, 122, -125, 101, 34, -64, -111, 186, -89, 24, 7, 2, -76, -21, 233, -51, -89, -49, -70, 71, 187, 177, 70, -161, 215, 80, -2, 89, -7, -138, -42, -12, 90, -42, -110, -204, 150, -95, 21, -18, 13, 37, 84, -101, 186, 139, -96, -239, -70, 68, 166, 107, 130, -118, -17, 3, 260, 12, -75, 60, 141, 1, -122, -104, 46, 77, -137, 80, 242, 140, 72, 36, -12, 96, 150, 96, -40, -224, 188, 280, -149, -85, 49, -228, -23, -60, 131, -185, -99, 7, -152, -30, -16, -60}
}
, {{-8, 60, 104, 87, -16, 46, -81, 163, 225, -63, 189, -58, 0, -72, 69, -89, -203, -68, 56, 101, -31, 59, -345, -5, -50, -47, 30, -84, 274, 101, 205, 147, 105, -62, -77, 228, -119, 3, 109, 71, 8, -306, 124, -179, -100, 23, 37, 84, 216, 93, 135, 134, 159, -21, 38, 117, -12, 12, -34, 161, -22, -46, 311, -51, -10, -42, -131, 40, 319, 171, -289, -78, 56, 65, 22, -9, -59, -126, 43, -19, -145, 75, -196, -206, -90, 118, -119, -119, 185, 104, -154, 93, -91, -406, 77, -84, -47, -63, 82, -234, 100, 162, -140, 69, -7, -64, -63, 152, -118, -4, 66, -226, 86, 167, -6, 110, -100, 141, 233, 14, -218, 95, 229, 253, 216, -32, -50, -55}
}
, {{81, 2, 3, -282, -201, 81, 139, 261, -112, -257, 235, -15, 79, 21, 100, 106, 22, -33, 2, -129, -91, 33, 103, 170, -387, -35, -204, 138, 37, 51, 219, 147, 219, -24, -163, 98, -138, -278, -64, 67, -171, -34, 70, -95, 96, -7, -64, 45, 19, 73, -50, 49, 104, -121, 258, 42, -214, -235, 75, -117, 204, 33, -109, -94, -164, -4, 92, -35, -129, -31, -77, -301, 219, -119, -112, 178, -12, 35, -30, 204, 98, -174, 75, -262, -180, -205, 222, -150, -94, 133, -111, 93, -57, 21, 184, -150, -104, -245, 141, -76, 26, 294, -69, -55, -131, 279, 149, 10, -55, -57, 9, 28, 245, -133, 56, -85, 63, -96, -51, -26, 145, 276, 114, -423, 144, 147, 143, 8}
}
, {{17, 88, 40, 61, 46, 39, -202, 179, -101, 176, -32, -17, -89, 192, 126, -195, 42, -69, -40, -130, 34, -103, -332, 201, 91, 33, 99, 8, 109, -98, 142, -216, 40, 97, -172, 44, 27, 182, -12, -16, 194, 63, -109, 8, 285, -152, 35, 128, -150, -24, 86, -16, 150, -46, -60, 45, -19, 36, 22, 255, -131, 80, 58, -51, 164, 13, -20, 12, 38, -166, -44, 11, -41, 50, -220, -32, -181, -13, 123, -167, -112, -29, 6, 115, 40, -90, -31, -14, 117, -68, -147, 101, 265, 81, -270, 60, 74, 258, -148, -124, -114, -185, -150, 153, 4, -132, 71, -5, 14, 133, 146, -23, -150, -42, -72, 54, -162, 44, 24, -37, -9, 156, 160, 17, -65, 11, -79, 105}
}
, {{-66, -20, 71, -60, -49, 79, -136, 52, -76, 208, 82, -15, -6, 169, 150, -28, 259, -114, 131, -49, 160, -118, -230, 6, -137, 44, 182, 24, 206, -144, 63, -67, 51, 133, 11, -171, 71, 94, -49, -25, 58, 253, 47, 43, 19, -265, -76, -150, -83, -40, 11, 75, 138, 126, -15, 81, 56, 65, -30, 208, -79, -56, -127, 105, -49, 44, -81, 87, -184, 118, 5, -212, -2, -54, -78, 24, -239, -56, 116, -123, 152, 27, 106, 62, -60, -118, -207, 35, -16, -33, -62, 222, 98, 51, -103, 12, 45, 135, 87, -131, 56, -69, -122, 251, 80, 194, -65, 26, 34, 229, -62, -1, -60, -11, 68, 39, 13, 142, 138, 208, -89, 10, 14, 92, 110, 42, -262, 37}
}
, {{53, 33, -38, -44, -15, 342, 0, -57, -50, -156, 64, 112, 17, -107, 46, 136, 76, 98, -33, -87, 96, 57, 98, 69, -194, -51, 7, 128, -108, -195, -55, 25, 96, -70, 24, 164, -4, -268, 25, -8, -67, -122, -44, 31, 100, -40, 53, 27, -192, 83, -11, -20, -5, -13, 318, -28, 33, -163, 157, -94, 175, 5, -199, -148, -69, -153, 231, 170, -174, 19, 179, -61, 24, 4, -63, 102, 255, 124, 152, 242, 150, -171, 19, 129, 41, 8, 172, -299, -15, 110, -145, 98, -34, 2, 248, -66, -29, -129, 3, 16, -111, 47, 124, 154, 24, 258, 24, 129, 52, -51, -100, 61, 271, -67, -13, 51, -35, -43, -115, -19, 137, -9, 130, -219, -146, 76, 163, -179}
}
, {{13, 21, -61, 96, 118, -55, -109, 115, 137, 98, 88, 209, -164, -17, 63, 66, -164, 10, 151, 195, -26, 87, -425, 62, -23, -24, 113, 56, -130, 55, 47, 0, -129, -48, -81, 105, -150, -169, -61, -41, 238, -32, -132, -12, -15, -52, 52, 303, -5, -60, 24, 60, 159, -132, -73, 90, 53, 135, 20, 117, -6, -128, 439, -96, 9, -2, -230, 1, 209, 95, -166, -147, -126, 65, 1, -286, 17, 160, 73, -216, -107, -27, -28, -351, -160, -69, 69, -134, 144, -100, -293, 38, -163, -167, -309, -238, -11, 160, 34, -154, -32, -199, -117, -138, -165, -15, -48, 114, -90, 93, 63, 26, -14, 109, -92, 37, -53, 61, 98, 9, -67, 196, 156, 117, -81, -45, 94, 111}
}
, {{-21, -199, 13, -117, 53, 23, 61, 11, 48, -86, 86, -204, 119, -36, -30, 6, 44, 100, -147, 61, -84, -137, 60, -262, -64, -163, 3, 27, 107, 186, -133, 45, 102, -11, -182, -30, 234, 163, -15, -113, 9, -109, -35, -120, 182, 93, 93, -194, 271, -22, -24, -97, -124, 106, 219, 85, -126, -113, -15, 40, 64, -157, 50, -75, -1, -37, 87, 126, 51, -3, 153, -12, -30, -81, -169, -10, -5, -56, 65, 159, 47, -54, -165, 207, -54, 109, 1, -19, 26, 75, 93, -4, -200, -150, 2, -98, -126, -24, 115, -13, 154, -112, 135, 154, 6, -94, 183, 117, -52, -85, 7, 99, 84, 37, -56, -76, 10, -133, 145, -156, 228, 26, 126, 194, -16, -133, -24, -46}
}
, {{6, -35, 82, 144, -99, 39, 81, -182, 207, -52, 75, 16, 113, 4, -145, 103, -67, -147, 132, -17, 208, -231, -235, -165, -21, 178, 29, 187, 68, 195, -76, -19, 80, 106, 166, -90, 170, -77, -87, 225, 126, -67, -163, 162, 47, 92, -103, -104, -16, -91, 41, -38, 89, -65, 76, -48, 316, 81, -65, 162, -51, -222, -37, 48, -276, 7, -127, -233, -6, 143, -164, -22, -114, 311, 112, 82, -18, 104, -128, -43, 283, -23, -42, 208, -5, 352, -52, -69, -54, -49, -173, -72, -196, -48, -186, -48, 221, 129, 14, 48, -39, -110, 3, -77, 9, -47, -159, -14, -88, 155, -66, 18, -185, -210, -10, -82, 207, 95, -74, 256, 30, 154, -97, 1, -121, -84, -49, -52}
}
, {{13, -124, 153, -104, -95, 19, 172, -155, 40, 125, -11, -237, -29, 73, -89, 149, -24, -3, -131, -4, -6, -195, 165, -66, -8, -63, -132, 86, 80, 253, -117, -40, 57, 55, -162, -10, -12, 183, 16, -47, -214, 113, 95, -101, 165, -67, 67, -64, 11, 164, 83, 57, -205, 88, 37, 138, -65, -92, -11, 39, -41, 5, -9, -18, 51, -72, 43, -269, -111, 62, 193, -115, 56, -13, -97, 96, -5, 13, 159, -43, -163, -93, -243, -1, -88, -61, -46, 78, 20, -44, 207, 5, -142, 75, -57, -74, -92, -51, 45, -96, -185, 33, 134, 77, -61, 115, 80, 235, 12, 8, 178, -87, 104, 203, -91, -69, 173, -146, -5, 25, -127, 40, 63, 87, -28, 68, -109, -115}
}
, {{40, -215, 94, -93, 52, -102, 145, 115, 10, 97, 35, -103, 7, 72, -2, -59, 107, 202, -55, 126, -58, -92, 26, -127, -43, -193, 81, -66, 190, 151, -19, -136, 28, 164, -70, -57, 124, 179, -63, -48, -105, 172, -83, -179, 59, 37, -10, -164, 139, 156, -100, 130, -149, 61, 0, 189, -161, 8, -11, 88, 94, 85, 40, -152, 66, -36, -28, 54, 148, 81, 161, -74, -111, -103, -136, 17, -157, -16, 266, 148, 101, -28, -259, 175, -55, 46, -81, 37, 9, 107, 248, -14, -55, -126, 31, -43, -81, -54, 136, 34, 318, -33, 119, 228, -2, -17, 208, 136, 23, 66, 59, -76, 89, -35, 71, -38, 1, -190, 133, -174, 192, 117, 127, 5, 107, -107, 55, -12}
}
, {{179, -57, 24, -174, 64, 26, 95, -13, -138, -69, 124, 164, 36, -145, 1, 62, -26, -95, 163, -160, 19, 219, -60, 224, -104, -25, -74, -24, -54, -18, 182, 21, 27, -238, 74, 306, -27, -156, -80, -144, -118, 129, -83, -42, 161, -151, 4, 87, -320, -66, 23, 69, 94, 13, 126, 101, -80, -104, 184, -35, 153, 72, -5, -66, -196, 118, 227, 33, -220, -107, -82, -45, 126, -81, 141, 107, -12, 98, 24, 164, 12, -230, 27, -28, -148, -13, 294, -116, -175, 45, -138, -40, 20, -73, 139, -157, 202, -124, 33, 8, -87, 106, -29, 290, -12, 91, 77, -49, 1, 66, -41, 91, 191, -181, 132, 48, 63, 35, -14, -24, 122, 40, 200, -187, -180, 172, 78, -54}
}
, {{25, 80, 136, -195, -64, -29, -16, 132, -139, -68, 85, -281, -157, -38, 145, 201, -170, 92, -209, 15, 13, -71, 132, 32, -80, 0, -40, 195, -175, -15, -175, 49, -58, -12, -104, 52, -27, 36, -86, 24, -214, -18, 232, -163, -20, -178, -134, -63, -32, -74, 103, 136, 40, 216, 56, 44, -72, -96, -132, 142, 9, 224, -237, 66, -9, 88, -27, -72, -224, -181, 65, 82, -175, 1, 66, 128, 40, -129, 165, 53, 40, -99, -47, -103, 160, -16, 138, -66, 102, -32, 305, 61, 16, 25, 128, 193, -261, -43, 25, 91, -81, 51, -25, 22, 66, 213, -29, 99, 38, -231, 159, -29, 126, 174, 2, -48, -12, 57, 88, 78, -63, 10, 193, -51, 100, -47, 20, -127}
}
, {{102, 25, -22, 198, -83, -23, -105, -65, 1, -170, -51, 202, 24, -37, 53, -181, -55, -13, 30, 196, 222, 21, -96, 122, 73, 156, -80, -15, -53, -260, 37, 180, -5, 115, -135, -57, 118, 30, -52, 167, 101, -179, 78, -95, 32, -57, 261, -52, -98, 7, -97, -94, 26, -111, -88, -112, 176, 232, 178, -57, -75, 68, -143, 6, 36, -114, -185, -48, 6, 157, 141, 41, -11, 34, -57, -17, 119, 5, -67, 7, 37, 42, 194, 68, 145, -36, -113, 78, 48, -49, -153, -182, -121, -46, -68, -133, -66, 143, 62, 72, 121, -105, -190, 3, 105, -213, 17, -67, -118, 48, -18, 29, -74, -95, -215, 115, -198, 10, 179, 51, -170, -119, 37, 67, 19, -152, 28, -41}
}
, {{-257, 73, 9, -97, -57, 148, -75, 32, -88, 101, 36, 43, -224, 151, 27, -6, 80, 107, -64, 84, -39, -189, -209, -100, -43, -62, -23, -61, 143, 12, -137, -68, -70, 61, 9, -9, 17, 146, 59, -57, 72, 237, 12, -65, 22, 4, -46, -105, 248, 12, -87, 56, -31, -18, -43, 125, 95, -62, 58, 26, -79, -137, -3, -89, 50, 3, 22, 19, 147, 160, 141, -81, -96, -117, -303, -118, 8, 132, 204, 86, 124, 9, -140, 177, -160, 59, -22, -157, 67, 135, 182, -144, -43, -96, -36, 21, -143, 57, 216, 39, 225, -56, -37, 187, -86, -62, 197, 119, -69, 99, -39, 9, 146, 6, 8, -82, -27, 6, 54, 6, 223, 212, 63, 76, -97, -187, -191, 133}
}
, {{112, 74, 84, -59, 70, -63, 66, -206, 56, 88, -139, -127, -259, 45, 58, -73, -61, 11, -131, 28, -44, 149, 387, -85, 258, 202, -23, -183, -42, -75, -244, -104, -107, -298, -10, -124, 84, 53, -34, 104, -277, 19, 226, -41, -176, -63, -48, 30, 138, -25, -120, -36, -87, 156, 80, -215, 209, -31, -120, 47, -188, 22, -149, 221, 28, 96, -46, -116, -71, -128, 67, 439, -151, 200, -16, -45, 307, 143, -106, 172, 99, -119, -3, 98, 20, 84, 47, -186, 75, 171, 19, 37, -6, -101, 126, 153, -141, -30, -82, 341, 9, 129, 141, 31, -2, 94, -208, 16, 28, 78, -149, -69, -13, 171, -210, 150, -39, 126, 50, 129, 76, -223, -162, 254, -52, -214, -102, 99}
}
, {{-124, -20, 93, 182, 68, 22, -15, -81, 52, 61, -212, -138, 98, 70, 34, 58, 90, -43, -12, -97, 187, -123, -89, 90, 31, 71, -31, 169, 138, 183, -124, -18, -151, 173, 78, -80, 38, 244, 99, -244, 170, 191, -122, -7, -29, -14, -67, -155, -174, 98, 95, -14, -54, 6, -113, 158, 46, 98, -85, 281, 27, -77, -2, -41, 78, 11, -44, -207, -54, 62, 108, 11, -79, -119, 73, 114, 13, -83, 86, -42, 23, -269, 133, -61, 83, -42, -144, 101, 118, -154, 162, -163, -12, -70, -234, 150, 29, 199, 58, -230, -165, -40, 79, 136, 16, -97, -100, -15, -71, 211, 132, -92, 5, 144, 45, 24, 84, 55, -54, 135, -285, 82, -19, 50, -10, 47, 31, -12}
}
, {{188, 204, 31, 153, -157, 101, -49, -110, -47, -116, 26, 178, -174, -52, 110, -20, 193, 145, 182, -38, 177, -7, 30, 106, 56, 113, 3, 89, -122, -186, 11, -114, -54, -58, 8, -79, 209, 58, -98, 44, -28, -38, -26, -11, 90, -118, 211, 111, -232, 125, -128, 66, 20, -141, -39, -33, 152, -9, 209, -129, -57, 6, -162, -107, 77, -116, 130, -25, -210, -14, 175, 107, -3, 34, -72, -134, 291, 167, 40, 136, 143, -34, -12, 117, 73, -100, 40, 8, 103, -184, -63, -102, -35, 89, -45, -134, 79, -83, -3, 144, 106, -12, -175, 81, 161, 69, -50, -74, 152, -149, -8, 18, 179, 53, 76, 99, -196, 101, 94, -5, -7, -166, -34, -57, -131, 56, -74, -59}
}
, {{-108, 21, 159, -90, 109, 82, -8, 61, -88, -99, -146, -224, 93, 270, -107, -252, 9, -134, -87, 90, 40, 59, -53, -25, -58, -132, -263, -64, -130, -11, 82, -60, 159, 197, 91, -242, 50, 216, -96, -38, -307, 228, 337, 58, 37, -158, 144, -57, 8, -161, -74, 70, -20, 172, -46, 119, -23, -87, 145, 110, -91, 163, 51, -115, 40, -19, -77, -63, -164, -24, 29, 44, -383, 60, -21, 11, -127, -97, 116, 37, 9, 15, -20, -182, 147, 3, 350, 184, -2, 120, 240, -131, 179, 168, 167, 189, -27, -214, -113, 162, 4, 104, -9, 41, 11, 48, 7, -14, -89, 99, 161, -54, -37, 69, 121, -208, 118, -51, 117, -45, -21, -102, -68, -116, 184, 59, 200, -90}
}
, {{98, 172, 113, 28, 68, 141, 49, -206, 17, -2, 276, -84, -69, -13, -20, 33, -77, -131, 55, -171, 71, -93, -178, -47, 303, -84, 78, 91, 104, 65, -25, -119, -259, -262, 46, 323, -71, -202, -102, -55, -61, -100, -2, 250, 30, -243, -63, 90, -43, 43, -18, -61, -121, 62, 23, 45, -32, -4, 62, -222, -101, -37, -116, 2, 281, 113, -192, -8, -272, 0, 10, -157, -3, 123, -34, 121, 18, 222, -176, 90, 101, -17, -167, -33, -103, 107, 195, -172, 81, 17, 8, -71, -111, -51, -46, -137, 17, -22, -55, -90, -105, 175, -91, 12, -72, 1, 113, -161, 74, 179, 149, -73, 97, 118, 153, 60, 177, 153, -185, 172, -79, -68, 40, 167, -81, 205, -34, 43}
}
, {{-33, -145, 61, -34, -163, -96, 0, 54, 174, -284, 187, 48, 89, -187, -50, -83, 154, 14, 8, 51, 85, -106, -41, -80, 89, 80, 7, -127, -34, -153, 249, 344, 93, -45, -217, 141, 40, 8, -85, -71, 6, -346, -80, -63, -53, -307, 33, -3, 6, 70, -216, 76, 56, -88, 75, -50, -30, 67, -73, 31, -31, -116, 313, 45, -88, -15, -95, 137, 106, 210, 18, -54, -125, 19, -288, -297, 54, -59, -272, -46, 75, 106, -57, 73, 172, -73, -191, 226, 159, -17, -202, 22, -88, -251, -164, -166, -38, -10, 42, -157, -365, -246, -124, -112, 255, -20, 173, 111, -90, 12, 44, 37, -28, -4, -216, -75, -29, 117, 196, 115, -46, -50, 245, 289, 68, -134, -118, -131}
}
, {{-98, 103, -96, -8, -125, 19, 200, -13, 110, -3, 132, -13, 120, 27, -45, -1, -170, -171, 21, 25, 77, -1, -47, -119, 113, 42, -166, 51, 73, 54, -187, -18, -116, -90, 101, -227, 181, 29, 66, 74, 5, 83, -43, 160, 39, 40, -51, 200, 88, -74, 6, -44, -183, 97, -75, 3, 252, 90, -31, -269, 7, -102, -82, -9, 28, 243, 137, 78, -75, 33, -13, 130, 28, -111, -137, 89, -26, 89, -3, -228, 120, 192, -106, -38, -80, 108, 93, -47, 177, 163, 186, 86, -83, 54, -32, -136, 55, 69, -55, -77, 173, 133, -101, -198, 28, -54, -117, 5, -1, 289, -104, 33, 36, 15, -50, -29, 275, 232, -23, 272, -80, -178, -140, 202, 112, 151, -14, 215}
}
, {{170, -73, 88, -103, -59, -97, 123, -123, 0, -55, 22, 68, 181, -64, 37, -79, 162, -212, 29, 6, 184, 3, -202, 62, -94, 78, -59, -146, -267, 68, 66, -61, 95, -38, 34, 108, 140, 4, -39, -125, -2, 76, -42, 40, 77, -13, -173, 36, -114, 13, 10, 39, -24, 70, 23, 90, 41, -34, -36, -52, 65, 58, -215, 92, -218, 54, 263, 191, -1, -96, 21, 37, 115, -38, 152, 69, -31, -117, -44, 73, 91, -25, -30, 84, 112, 76, -9, 77, -74, 74, 128, -79, 60, 18, 80, -122, 113, 51, 123, 36, 54, 87, 114, 42, 188, -114, -61, -56, 7, -73, -18, 178, 4, -250, 75, -74, 94, 97, -8, 184, -72, -96, 99, -105, -93, 82, 23, -138}
}
, {{117, 63, 145, 112, -162, 224, -208, -200, -2, -74, 236, 14, 69, -141, -12, 295, 199, -7, -76, -168, 275, -221, 76, 81, 254, -23, 105, 90, -30, -45, -250, -258, 75, -106, -29, 37, 86, 45, -87, -18, -113, -96, -19, 177, 140, -287, -102, 68, 101, -89, -10, 15, 1, 194, 52, 56, 299, 110, 227, -90, 230, 102, -137, 155, -5, -1, -217, -164, -118, 57, 47, 52, 116, 23, 34, 236, 115, 98, -83, 40, 181, 10, 134, 93, 99, 71, 11, -91, 45, -145, -67, 36, 28, 133, -171, -243, 101, -33, -22, 129, 29, 172, -118, 80, 193, 114, -41, -344, 185, 71, -104, -70, 160, 170, 23, 16, 100, 182, -164, 199, -110, -263, -153, 136, -40, 220, -51, -34}
}
, {{223, -57, 41, 95, 94, -60, 167, -95, -19, -32, 127, -63, 74, -223, 91, 60, 91, -98, -137, -8, -20, -89, 25, 15, 40, -3, -155, 15, -99, -60, 49, -162, -95, -101, -85, -2, 117, 84, -33, 173, 180, -178, 157, -52, 14, -139, 229, -24, -180, -32, -91, -321, -52, 208, -40, -79, -134, -106, -1, -125, -337, -70, -40, 26, 115, -81, 115, -139, 30, -220, -32, 251, -68, 298, -30, -95, 66, -43, -30, 51, -140, -133, 151, 51, 198, 108, 8, -111, 131, 124, 62, -44, -91, -53, -132, 164, 162, -1, -83, 55, -271, -157, 3, -177, 110, -267, 50, 45, 153, 38, 71, -71, -25, 125, 81, 137, 109, -89, 17, 6, 35, 77, -90, -4, -38, -133, 16, -48}
}
, {{75, -72, 141, -107, -40, 151, -24, 44, 31, -21, -44, 115, 22, -141, -73, 38, 84, -20, 4, 2, 206, 23, 160, -337, -205, -27, -72, -35, -134, -53, 426, 109, 108, 21, -43, 80, 19, 81, -5, 159, -211, 78, -28, 144, -75, 121, -52, 7, -68, -13, -264, 144, 104, 143, -2, 179, 168, -143, 48, -62, -78, 45, -70, 96, -199, 75, -170, 132, -140, 35, -68, 12, 106, 132, 95, 169, -117, 20, 31, 6, 46, 83, 9, 177, 48, 48, -22, -48, -273, 125, 110, 184, 97, 23, 9, -175, 21, -294, 72, 100, -54, 120, 57, -57, 70, 183, -87, -50, -77, -43, -129, 109, -82, 21, 121, -14, 111, 133, 82, 129, 228, 137, 173, -71, 253, 93, -65, -12}
}
, {{-7, 99, 23, -121, -146, 186, -11, -131, -13, -128, -46, 4, 118, -84, -29, -127, 105, -61, 282, -127, 362, 72, -155, 12, -88, -50, 106, 260, -117, -62, -17, -184, 153, 93, -172, -82, -76, -66, -48, -104, -64, -75, -93, 284, 153, -149, 60, 18, -223, -317, -40, 102, 15, 95, 42, -53, 218, -59, 38, 11, 153, 47, 116, 138, -19, 0, -130, -49, -216, 130, 42, -7, 116, -135, 169, 165, 76, 51, -68, 143, 286, -79, -20, 3, 171, 74, 97, 108, -119, -146, -17, 15, -94, 218, -37, -74, 139, 217, -184, 38, 10, 20, 90, -15, 177, 153, -48, -141, 79, -79, -73, 98, 86, -74, 55, -42, 65, 147, -23, 156, -90, -124, -99, -31, -91, 216, -141, 30}
}
, {{-13, 30, -7, -153, -10, 12, 170, -124, -160, 46, -17, 289, 55, -156, 33, -184, 33, 184, 176, -194, -46, 256, 65, -31, -132, -1, 31, 68, -272, 13, -15, -32, 22, -72, -14, 25, -8, -16, -162, -140, -26, -132, 22, -36, 134, -59, -66, -35, -243, -160, 28, -42, -29, 94, 167, -127, -138, -150, 176, -18, 126, -97, 133, -99, -145, -15, 58, -189, -51, 100, 93, 78, -8, 23, 70, 50, -120, 125, -97, 296, 121, 81, -180, -85, 60, 96, 22, 39, -105, -65, -8, -34, -22, 57, 131, -18, 121, -41, 9, 115, -101, -146, 122, 194, 55, -110, -51, 8, -46, -130, -49, -5, 30, 130, -149, 134, 66, -119, 13, -75, 166, -21, 38, -192, -65, 76, 61, 133}
}
, {{92, 5, -64, -19, -142, -121, -101, -47, 202, -16, 7, 57, 17, -118, 53, -102, -28, -22, 70, 52, 132, -10, -2, 41, 103, 230, -30, -63, -51, -191, 152, 51, 35, -80, -50, 54, 217, -29, -23, 110, -81, -380, 21, -8, -2, -69, 53, 117, -143, 80, -70, -66, 105, -91, 35, 21, -70, 154, 30, -202, -126, -106, 59, -13, -17, -18, -26, 26, 74, 192, 66, 69, -136, 63, -126, -135, 178, 39, -63, 10, -55, 175, 0, 144, -24, -83, -171, 37, 99, 34, -219, 4, 84, -52, 23, -208, -65, 29, 85, 52, -126, 30, -108, -147, -2, -115, -97, 102, -62, 180, -95, -58, 167, 48, -21, 84, -206, 14, 267, 5, 106, -54, 136, 228, 98, -210, 69, -63}
}
, {{-33, 27, 69, -12, -43, 52, 55, 23, -89, -34, -128, 237, 95, -263, -38, 67, -24, 146, 236, -63, -147, 230, 121, 105, 119, -33, -187, 49, -139, 22, -54, -96, 1, 201, 26, -239, -52, -101, -17, 50, 47, 53, 187, 44, -75, -122, 168, 19, -33, -244, 26, 9, 9, 19, -56, -100, -216, 173, 239, 83, 51, 139, 206, 54, 86, -78, -150, -63, 27, 136, -172, 23, -73, 150, 94, 11, -197, 115, 34, 136, -3, -55, 227, -226, 56, -123, 152, 204, -7, -128, -196, -187, 102, 211, -34, 13, -26, 11, -128, -28, -86, -65, -12, 90, -6, 69, -112, 44, 64, 40, 103, -41, -64, 103, -141, 98, 6, -62, -82, -191, 135, 242, -182, -285, -70, 66, 169, 67}
}
, {{-74, 177, -36, 21, -36, 120, -38, 7, -68, -95, 44, -385, -78, -7, 156, -105, 127, 50, 240, -73, 357, 55, -235, -70, 142, 30, 4, 208, -103, -94, -96, -61, 35, -14, 32, -48, 106, -21, -14, 65, 164, 30, -151, 253, 201, 1, 47, 30, -147, -132, -208, -29, 44, 32, 93, -18, 173, 21, -113, 109, -28, -25, 27, 68, -5, -33, -35, -149, -209, -13, 166, 72, 10, 22, 6, 252, -15, 70, -60, 26, 88, -3, -85, -114, 53, -14, 219, 10, 112, -211, 5, 23, -46, 122, -72, 31, -125, 281, -94, 95, -92, -51, -45, 1, 86, 128, -31, -214, 102, 52, -141, 134, 19, -84, -145, 139, -22, 99, 46, 102, -34, 59, -75, -33, 31, 144, -84, 101}
}
, {{3, 86, -11, 134, 3, -58, 33, -46, -74, 89, 196, -21, -104, -13, 36, -72, -106, -4, 106, 15, 283, -83, -221, -5, 157, 102, 41, -54, 83, -48, -345, 1, -94, 30, 106, -174, 122, -94, 13, -108, 62, -18, -150, 259, 129, -79, -74, 67, 70, -231, 91, 93, 46, -67, -20, -127, 339, 173, 25, 20, -16, 2, -170, 101, 3, 155, 48, 118, 45, -49, 33, 23, -44, -136, -103, 247, -17, 93, 120, -259, 63, 120, 51, 19, -259, 148, 84, -21, 211, -18, -29, -33, -124, 7, -176, -137, -108, 76, -9, -54, 81, 96, -81, 134, 56, 135, -99, -94, -72, 333, -172, -18, -43, 20, -188, 164, 127, 169, -90, 201, 12, 48, -74, 224, 52, 99, 8, 95}
}
, {{85, -56, 126, -158, -42, 121, 103, -34, -49, -96, 112, -227, 29, 17, -94, 102, -166, 71, -170, -38, -20, -230, -80, 105, 37, -92, -67, 31, 156, 227, -131, -3, -91, 12, -118, 48, 137, 191, -5, 30, -110, -28, 243, -86, 127, -74, 184, -115, 19, -42, 17, -216, -164, 143, 78, 215, 12, 31, -54, 167, 164, 50, -164, 22, 99, -68, 54, 91, -31, 10, 149, -52, -179, 95, -222, -163, 45, -75, 48, -24, -59, 12, 12, 279, 37, -2, -51, -22, 99, 85, 126, -78, -126, -47, -43, -27, -137, -32, -131, -182, -196, 66, 111, -36, -14, -166, 57, 194, 121, -48, 101, -111, -16, 21, 24, 42, 132, -49, 55, 74, 35, -3, 122, 88, -62, 8, -84, -86}
}
, {{184, -1, 71, -186, 1, 87, 17, 25, -52, 129, 145, -169, -196, 74, 158, -115, 55, 46, -119, -92, -76, 26, -107, 84, 72, 5, 45, -79, 24, 12, 327, -180, 176, 39, -248, 76, -15, 195, -69, -154, 22, -18, 51, -114, 131, -102, -129, 87, -62, 56, -3, 193, 115, 35, 174, 80, -82, -179, -57, 10, -112, 34, -62, -102, 171, -46, 17, 91, 27, -87, 18, 25, -33, 45, -102, 59, 63, 30, 129, -124, -45, 69, -57, 38, 9, -38, 124, -115, 81, 149, 185, 52, 60, 24, 76, 64, 6, -5, -108, -33, -20, 90, -132, 142, -106, -2, 168, 80, 9, -55, 168, -89, -85, 71, -13, 17, -135, 4, 205, -104, 82, -8, 271, 46, 138, -47, -54, 83}
}
, {{-59, 1, 117, 5, 129, 40, -77, 138, -54, 90, 162, 4, -180, 167, 8, -156, 29, -45, 70, -95, 154, -160, -139, 24, -38, 16, 94, 2, 78, -46, -88, -171, -85, 150, 151, -53, 69, 250, -39, -52, 134, 148, -126, 84, -4, -10, -6, 11, 114, 18, -148, 146, -87, -143, -90, 22, 267, 30, -40, 193, -35, -24, -83, -82, 39, 38, -67, 76, 20, -96, 92, -91, -96, -121, -205, -93, -156, -34, 280, 34, -89, 160, -78, 74, -80, 92, 56, -200, 141, -87, -12, 86, 99, 22, -25, 37, -193, -59, 54, -86, 64, -213, 11, 37, -69, -3, -60, 60, 37, 355, -102, -27, -34, -22, -40, 53, 45, 49, 18, 49, 15, -2, -22, 59, -95, -90, -63, 142}
}
, {{78, 37, 59, 51, -39, 11, 82, 19, 99, -249, -102, 168, 290, 69, -97, -336, 95, -59, 213, 72, 320, -95, -222, 40, -43, 124, -64, 90, -259, -18, 100, -89, 16, 151, 168, -279, 217, 99, -134, -33, 129, -19, -53, 199, 42, -97, 54, -218, -62, 2, -135, -110, -184, 78, -251, 16, 131, 112, 208, 38, -184, 132, -301, 72, -106, -104, -64, -10, -272, 30, 180, 161, 73, -71, 22, -84, -14, -39, -61, 80, 102, -35, 102, -104, 257, -37, -64, 18, 74, 89, 69, -114, -80, 127, -136, -39, 112, 170, 16, 154, 32, -138, 45, 0, 103, -244, 11, -118, -216, -189, 56, -73, -143, -100, -74, -6, -45, 52, 84, 35, -114, -223, -97, -95, -55, 64, 166, -45}
}
, {{25, 6, -12, -112, 48, 42, 84, -65, 1, -84, -127, 336, 1, -85, 23, 3, 63, 124, 53, -33, -21, 260, 3, 21, -184, 115, -146, 52, -231, -55, -92, -94, 91, 111, 61, 48, 14, -82, -35, -193, 100, -49, -118, 51, -11, -185, 45, 33, -132, -143, -67, 47, -17, 103, 74, -86, -40, -87, 185, -94, -17, -140, 239, -8, -2, -44, -49, -117, 55, 254, -76, 85, -103, -14, 89, -22, -96, 74, -22, 338, 278, 14, 31, -85, 124, -68, 37, 36, -9, -160, 1, -190, -8, 104, -109, -53, 8, 13, 110, 79, 36, -69, 113, 80, 70, -3, 77, 144, -17, -153, 18, 39, 102, -47, -212, -13, 132, -1, 78, -77, 112, 96, 47, -114, -177, 124, 24, 44}
}
, {{116, 116, -10, -54, 41, 270, 13, 202, -33, -26, -218, -183, 152, 42, -107, -190, -137, 11, -29, 163, -83, 152, -15, -106, 100, -49, -131, -78, -74, -78, 264, -176, 106, 125, 70, -323, 67, 225, -54, 45, -66, 118, 168, 140, 127, 42, 106, 121, -30, -188, -112, 22, -2, 272, -104, -59, -95, 106, 42, -66, -281, 81, 121, -80, 81, -157, -141, -122, 46, 51, -34, 166, -287, 217, -26, -24, -101, 116, 88, 30, -108, 173, 114, -147, -48, 57, 233, 212, 37, 294, 139, 72, 137, 128, -11, 179, -118, -47, -214, 59, -96, -58, 24, 53, -268, -174, 5, -121, -123, -1, 44, 64, -214, 101, -32, -110, -7, -200, 67, -183, 43, 3, -60, -193, 230, 43, 179, 72}
}
, {{-31, 77, 57, -108, -129, -35, 202, -124, 116, 167, -49, 14, -37, 152, -217, -53, 66, -69, 210, 31, 102, 54, -38, 75, 207, -120, -31, 17, -34, -74, 20, 116, -166, 23, 131, 42, -58, -323, -156, 56, -150, 176, 56, 109, 98, 65, 148, 23, 90, 355, -55, 37, -167, -177, -113, 74, 160, 85, 3, -211, 45, 20, -43, -103, 184, 94, -42, -69, -84, 216, -272, -228, -39, 33, -27, 108, 50, 38, -198, -63, 88, 104, 1, -181, -284, -35, 227, 62, -57, 180, -170, -227, -40, -39, -9, -172, 63, -22, 18, 13, 198, 103, -148, -133, -68, -171, 177, -12, 34, 210, 101, -69, 148, 75, 41, -95, 96, 75, -74, 111, -60, 115, -69, -106, 68, 89, 97, 128}
}
, {{148, 46, 132, 26, -259, 44, -59, -132, 221, -217, -13, 288, 45, 189, -78, -84, -9, -166, 103, 4, 407, -112, -38, -29, -155, 31, -83, 225, -109, -13, 20, 219, 177, 69, -60, -8, 77, -96, -116, 106, 42, -75, 18, 164, -76, -6, 112, -42, -76, -75, -144, 39, 25, 12, -96, -71, 367, 103, 299, 24, 93, 0, -77, 31, -82, -23, -237, -191, -70, 300, -18, -99, 5, -41, -5, 96, 48, 18, -24, 101, 341, 86, 78, 167, -9, 60, -14, -68, -29, -110, -50, -90, -235, -154, -161, -206, 90, -101, 77, -2, 8, 17, 12, -39, 104, -98, 110, -122, -27, 64, -55, -67, 85, 38, -103, 85, 20, 198, -50, 234, -92, 60, 9, 71, 50, 56, -101, -25}
}
, {{9, -214, -16, -32, -53, -57, 238, 293, -111, 30, -49, -82, -63, -90, -51, 187, 84, 76, -39, 30, -57, -180, 11, -5, -133, -54, 21, 8, -21, 22, 178, 51, 232, 10, -236, -33, -82, 6, -58, -68, -173, 171, 77, -173, 153, 24, -111, 127, -73, 76, -133, 65, 104, -101, 77, -148, -131, -148, 183, 106, 45, 64, -3, -53, -163, 5, -94, -102, 60, -178, -171, -23, -76, -27, 19, 188, -16, -4, -105, -34, 110, 4, 51, -113, -26, -11, -10, -72, -216, -25, -157, -29, -83, -36, -169, -111, 292, -205, -183, -3, -10, 116, -20, 86, -223, 46, 152, -98, -238, 70, 69, 82, 48, -115, 97, -214, 9, -44, -95, 20, 142, -10, 180, -199, 160, 77, 107, -20}
}
, {{132, -17, 82, -55, -33, -87, -42, 99, -17, -188, 15, 66, -9, -31, 15, -57, 166, -36, 96, -30, 140, 29, -14, 57, -33, 137, -91, -157, -126, -75, 279, 60, 115, 22, -113, 241, 119, 13, -13, -66, -69, -181, -65, -40, 22, -166, 78, 110, -146, 0, -194, 82, 207, -170, -60, 31, 42, 77, 131, -3, -36, 67, -176, 35, 18, 30, -21, 26, -42, -56, 2, 58, 85, -66, 104, -14, 52, -22, -58, 49, -59, -3, -1, -113, 5, -21, 68, 82, 78, 108, 76, -55, 45, 66, 97, -224, -19, 21, 57, 13, 100, 112, -10, 37, 106, 83, 35, 15, -59, -87, 3, 25, -65, 76, 2, 54, 17, 86, 308, 41, 20, -273, 192, -5, 85, -69, -57, -59}
}
, {{84, 198, 1, 166, 136, 299, -43, -13, -2, -59, -119, 120, 11, 109, -40, -123, -111, 289, 174, -209, -87, -18, 122, 49, -108, -142, -113, 51, 37, -159, -138, -130, 36, 144, 235, -94, 165, 294, -123, -65, -49, 212, 172, 86, 182, -273, 169, 111, 94, -313, 21, -122, -105, 34, -264, 17, -117, -24, 356, 24, -46, 109, 106, 66, -14, -176, -42, -88, -123, 116, 28, 24, -202, 246, 122, -302, -96, 175, 196, 142, -128, 200, 145, 54, -46, -37, 168, 280, 139, -137, 235, -256, -83, 117, -72, 212, -11, -194, -258, -29, -19, 61, 231, -33, 88, -62, -219, -83, 113, -46, 211, -101, -66, 93, 19, -42, 89, -71, 121, -31, 124, -106, -122, -261, -86, 210, -60, -34}
}
, {{-103, 27, -129, 106, 111, 209, -15, -229, 6, 159, 35, -245, -42, -66, 311, 50, -166, 112, 134, 24, 123, 7, -95, 236, 166, 12, 38, 254, 102, -140, -60, -54, -1, 34, 40, 102, -99, -20, -102, -63, 241, 141, -77, 76, 112, -131, 161, 177, -70, 163, -112, 46, 86, 20, 333, -197, -96, 184, -93, 46, 95, -65, 111, -2, 117, -131, -159, -104, -281, 57, 68, -15, 99, 55, 94, 93, 181, 140, -104, 156, -72, -75, 11, 40, -10, -127, 45, -260, -15, -307, -161, -76, -155, 2, 6, 208, 59, 387, -122, -33, -204, -47, -220, -94, -4, 117, -26, 6, 262, 36, 58, 87, 120, -75, 9, 206, 13, 76, -102, 125, -119, -127, -41, -84, -213, 262, -23, 7}
}
, {{-58, 50, -13, -36, 53, -93, -47, 24, 158, -75, -59, 420, 46, 254, -40, 84, 170, 100, 157, 31, 96, 154, 84, -11, -43, -53, -233, 265, 51, -131, -81, 183, -104, 102, 20, 49, 43, -233, -179, 193, -153, 44, -91, 6, -87, 122, 269, 97, -97, 123, 101, 29, -99, -200, -154, 181, 91, 223, 300, -30, 265, -24, 88, -123, 124, -171, -145, -369, -10, 263, -145, -113, -157, -30, -82, -46, -1, 144, -124, -12, 248, 12, 129, -183, 10, -370, 101, 163, 93, -86, -339, -348, -7, 15, -164, -127, -42, -134, 78, 115, 164, 122, -138, 98, -7, -114, 258, 81, -255, 69, 79, -260, 39, 140, 19, 1, -84, 173, 66, -60, 3, 210, 126, 55, -120, 117, 93, -202}
}
, {{-4, -184, 204, -205, -163, -126, 57, -30, -151, 16, 34, -32, -88, 8, 7, 70, -151, 123, 57, 53, 35, -96, -51, -38, -239, -150, 153, 1, -62, 105, -195, 73, 114, 222, -38, -67, -13, 24, -44, -39, -195, 172, 162, -119, -278, -207, -33, -154, 210, 208, -17, 212, -98, 132, 66, 60, 39, -137, 26, 217, 12, 116, 50, 133, -57, -44, -108, -144, -7, -31, 198, -140, -85, -1, -12, -16, -170, 52, 70, 67, -2, 210, 81, 104, -54, 90, -17, -209, -153, -44, 139, -33, 91, 37, -147, -13, -119, -140, 40, -35, -47, 129, 246, 180, 68, 84, -13, 141, 60, 214, -74, -162, 158, -82, 43, -76, 9, -201, 57, 40, 64, 50, -30, -66, -34, 1, 38, -75}
}
, {{43, -121, 99, -56, -111, 84, 215, -33, 102, 211, 82, -99, 18, -10, -37, 234, -15, -48, -76, -49, -131, -426, -37, -325, -55, -58, 0, 109, 6, 289, -45, -165, -115, 120, 35, -239, 37, 120, -103, 211, -84, 207, 143, 72, -85, 75, -34, 17, 215, -169, -20, 46, -60, 260, -116, 133, 50, -99, 9, 87, 29, 247, 284, 47, -201, 94, 35, -149, 102, 4, 210, 79, -21, 194, -14, -152, -245, 42, 102, -33, -115, 178, -212, 259, 8, 199, -29, -12, -271, 84, 179, 83, 147, 79, 108, -11, 46, -178, 47, -19, -184, -9, 14, 91, -126, 93, -273, 76, 113, 49, -183, -62, 26, 79, 71, -191, 293, -113, 83, -67, -82, -19, -70, -125, 164, -12, -201, -62}
}
, {{-20, 22, -23, 155, 67, -22, 78, -171, 161, 135, 122, 0, 119, -75, 43, 25, -167, -8, 95, 55, 175, 131, -163, -47, -43, 79, 205, 11, 220, 164, -50, -251, -114, -19, 145, 191, 13, 30, 46, -219, 147, -114, -311, 101, 30, 159, -179, -155, 70, 32, 22, 58, 137, 19, 54, -25, 258, 264, -53, 20, -13, -128, -287, 33, -121, 55, -142, -80, 52, -6, -70, -26, -42, 30, 65, 226, 56, -62, -64, -274, -26, 25, 45, -29, -233, 26, -82, -81, -3, 20, -26, 101, 29, -75, -253, 79, 21, 197, 31, -153, -111, 108, -32, 182, -71, -87, -110, -189, -55, 88, -60, 44, -104, 141, 0, -108, 117, 188, -130, 91, 69, 35, -120, -145, -73, -4, -26, 136}
}
, {{-96, -24, 28, 122, -28, 167, 216, -8, 302, -22, 80, -149, 89, -100, -136, 151, 70, 32, -10, -126, 6, -188, -71, -114, -10, -109, -78, -120, 173, 122, 12, 80, 49, 31, -187, -57, 97, -104, 28, -132, 87, 96, -13, -105, -72, -123, 38, -249, 176, 151, -14, -3, -90, -41, 141, 4, -41, -78, -150, 6, 43, -48, 78, -66, -96, -33, 41, -172, -6, 70, 35, -433, -137, 101, 28, -141, 51, -26, -96, 133, 3, -42, -132, -44, -99, -19, -169, 85, -280, 17, 127, -117, -133, -202, -191, 40, 266, -43, -114, -43, 3, 241, -119, -100, 177, -124, 124, 133, -61, 122, 136, 57, 140, 374, 61, -144, 120, -12, 48, 106, -176, 92, -11, -80, -11, 44, 64, -129}
}
, {{-81, -63, 161, -202, -58, 45, -37, -19, -98, -37, 175, -183, -10, 166, 66, 60, 180, 42, -149, 8, 65, -74, -130, 171, -31, -82, -231, -25, -9, 178, 60, 36, -65, 19, -83, 168, 31, 167, 29, -108, -28, 60, -2, -113, -135, -106, -58, 27, -67, 120, 18, 299, 105, 210, 188, 148, -120, -175, 113, 160, 17, 161, 57, -52, 87, 62, 2, -50, -108, -68, -69, -70, -203, -181, -37, -2, 21, -61, 109, 35, 16, -85, 5, -145, 4, 15, 119, -105, 153, 2, 189, 21, 87, -146, -173, -116, -33, -19, 94, -206, 181, 97, -121, 165, -15, 29, 210, 108, -20, 93, 126, -68, 98, 77, 5, -55, 40, -100, 86, -41, -2, 102, 134, 192, 70, 117, 26, -115}
}
, {{18, -40, -81, -5, 61, -114, 164, -63, 33, 36, 96, -41, 58, 120, -74, -65, -180, -145, -3, -36, 70, 30, -137, 20, 27, 88, -11, -39, 29, 72, -42, 89, -142, -186, -8, 198, 153, 61, -17, -9, 63, 4, -32, 41, 3, 51, -146, 112, 180, 74, -90, -186, -27, -114, -188, 27, 140, 145, 37, 43, -10, -86, -114, 15, -119, 139, 138, 67, 25, 95, -99, -15, 117, -82, 67, 106, -3, 86, -83, -173, 97, 87, -8, 3, -132, 8, 2, 1, -26, 91, 130, -79, -104, -95, 40, -149, -2, 46, 88, -93, 167, 37, 121, -142, 90, -329, -59, -9, -39, 165, 110, 146, 62, -77, 70, -88, 138, 140, -5, 218, -165, -73, 26, 53, -29, 0, 45, 10}
}
, {{65, 94, 33, 82, 53, -32, 30, -209, -11, 176, 144, -150, -216, -103, 80, -109, -243, -36, 191, -47, 92, -10, -177, 190, 350, 29, 73, 10, -56, -40, -106, 90, -168, -217, 35, 93, 123, -107, -15, 56, 98, -79, -92, 169, 128, -150, 30, 265, -33, -33, -129, 19, 134, -50, 33, -106, 103, 168, -393, -112, -278, 5, 87, 11, 220, 47, 63, -48, 19, -32, 44, 96, -184, 80, -181, -80, 121, 228, -152, -96, -92, -41, -28, -162, -166, 0, 68, -84, 263, -79, -156, -103, -63, -49, -180, -1, -168, 97, -100, -29, -161, -109, -330, 47, -79, -61, -74, -84, -148, 145, 24, -11, -106, 128, -12, 238, -11, 129, 0, 98, 59, 110, -55, 173, 7, -71, -13, 129}
}
, {{328, -10, 118, -58, 10, -146, -82, 245, -79, 39, 251, -57, -256, -111, 148, -37, 234, 36, 98, -162, 30, 104, -205, 88, 44, 12, 78, -4, -28, -207, 389, -241, 87, 3, -81, 221, 83, 107, 106, -93, 172, -4, -144, -130, 280, -83, -183, -1, -106, 138, -49, 352, 213, -54, 111, 188, -120, -108, -111, 37, -197, -73, -71, -94, 177, 46, -150, 129, 198, -140, -181, -129, 142, -65, 112, -192, -78, -14, 120, -166, -124, -141, -39, -18, -15, 135, 20, -197, 2, 241, 24, 238, 244, -93, -98, 163, -11, -8, 81, 121, 206, -142, -287, 275, -205, -15, 231, 109, 146, -102, -4, -50, -138, 22, -35, 59, -34, 17, 64, -181, 95, 217, 322, -131, 8, -11, -72, 140}
}
, {{9, -119, 124, 81, -63, 147, 76, -337, 180, -93, 170, -172, -101, 110, -198, 148, -182, 137, 51, 77, 108, 50, 44, -198, -6, -112, 40, 113, 115, 45, -129, 234, -88, -45, -4, -157, 191, 139, -39, 294, -280, 118, 124, 170, 10, -7, 257, -65, 300, 97, 158, -82, -256, 32, -59, -130, 193, 4, 4, 223, 189, -52, 152, 122, 36, 84, -83, 19, 59, 135, 295, 101, -120, -5, -260, -241, 157, 197, 110, 46, -20, -171, 82, 39, 33, 79, -69, 149, 111, 101, 75, 68, -131, 36, -220, 89, -105, -219, 74, 15, -206, 118, 137, -155, 2, -37, -4, 121, -57, 156, -83, -68, -22, 56, -210, 51, 185, 22, 191, 211, 141, -86, -108, 6, -62, 76, -372, 11}
}
, {{-37, -104, 59, 103, -23, -10, -79, -12, 45, -57, -20, 0, -21, -121, 84, 6, 72, 178, -8, 0, -35, -124, -64, 66, 130, 37, 43, -57, -25, -39, 32, 100, -135, -56, -177, 81, 109, 15, 21, -59, 49, -90, 60, -10, -50, -74, 203, 64, 0, 172, 1, 44, 19, -28, 209, -6, 142, 47, 61, -46, -59, -70, 65, -64, 102, -129, 72, 163, 105, 24, 163, 27, -142, -19, -128, -255, 198, 9, 37, 15, 45, 59, 24, 201, -47, 25, -139, -53, 147, -62, -173, -3, 15, -99, -6, -78, -104, 51, 19, 56, -137, -74, -108, 50, 13, -84, 31, 11, 17, -26, -29, 43, -33, 13, -97, 50, -134, -26, 117, -58, 8, -81, 19, 295, -14, -220, -93, -91}
}
, {{66, 59, -48, 55, -35, 45, 2, 54, 29, -84, -70, 326, 127, -145, -37, -208, 160, 26, 229, 121, 230, 127, 35, -45, -94, -3, 10, 41, -111, 13, 78, 11, 42, 36, -43, -53, -18, 2, 24, -273, -81, -95, -90, 197, 36, 129, 178, 11, -195, 110, -75, 98, -10, 138, -110, -13, 268, 67, 143, -60, 137, -8, 119, 73, 26, -72, 9, 19, -240, 252, -25, -34, 159, -116, -14, -28, 119, 25, 17, 214, 386, 192, -79, -46, 145, -85, -121, 248, -52, -28, -66, -164, -132, 48, 33, -111, 61, -231, 97, 118, 109, -143, 136, 23, 134, -48, -12, -82, 112, -133, -162, 196, 4, -97, 202, 48, -122, 32, 157, 193, 45, -108, -58, 195, 25, 145, -48, 14}
}
, {{80, -29, -61, 195, 56, 120, -69, -181, 10, -144, -63, 140, 72, -18, 97, -109, -77, 134, 114, 28, 87, -4, -433, 72, -226, 31, 23, 111, -54, 53, -135, 114, -130, 14, -76, 24, 167, 19, -14, -40, 220, -191, 45, -91, 137, -52, 77, -22, -102, 87, -137, -36, -8, 61, -97, -3, 41, 276, 162, 58, -45, 111, 7, 35, 64, -108, -124, 43, 73, 139, 113, -152, -83, -113, 59, -154, -71, -25, -42, 51, 174, -197, 305, 14, 66, 0, -215, -29, 199, -41, -146, -66, -144, -313, -108, 59, -9, 222, 174, -246, 60, -226, -126, 187, 213, 9, 358, 113, -70, -72, 28, 34, -7, -74, -125, 110, -39, 116, 111, 107, -256, 7, -120, 79, -30, -61, -76, -64}
}
, {{129, -133, -27, -131, 67, 130, 298, 7, -15, -34, 19, -181, -5, 48, -120, 144, 212, 45, -15, -72, -41, 51, 39, -128, 188, 58, -50, -130, 24, 144, 76, -15, -119, 44, 0, 96, 71, -52, 64, -173, 114, 154, 21, -71, 113, 124, 27, -168, 144, -121, 26, -337, -164, 59, 109, 152, -97, -57, -12, 14, -79, -159, -142, -28, 48, 129, 26, -27, 39, -119, 45, 83, -97, 180, 74, 74, -23, 67, -97, 39, 7, -156, -56, 204, 89, 262, 107, -99, -21, 309, 90, 79, -57, -73, -15, 89, 186, -111, -18, 110, 93, 30, 145, -20, 54, -5, -40, 83, 181, 172, 17, -135, 2, 32, 9, -45, 252, -164, 26, 105, 197, -60, -81, 54, -102, -45, 89, 191}
}
, {{-16, -214, 229, 44, 18, 88, 103, -224, 138, -163, -318, -38, 66, 79, -48, 33, 179, 167, -4, -118, 163, -339, -205, -41, -6, 16, -76, -45, -257, 128, -203, 24, -34, 182, -25, -242, 299, 132, 204, -82, 74, -12, -21, 16, -73, -109, -159, -346, 141, 182, 96, -87, -230, 102, -26, 184, 36, -12, 50, 87, 95, 34, -24, 67, -73, -169, 91, -301, -4, -16, 242, -96, 78, -47, 153, -172, 49, -229, 74, 97, 193, -100, 69, -75, 211, 25, -205, 93, -74, -89, 87, -132, -16, 53, -112, -6, 102, 144, -94, -104, 22, 40, 194, -37, 335, -18, -8, 137, 202, -92, 88, -26, 123, 104, 7, 29, 114, -67, 32, 186, -61, -75, -16, 84, -140, 39, -96, -208}
}
, {{-126, 412, 67, -1, -76, 10, 59, 419, -252, -212, 13, -417, 91, -62, 45, 62, 158, 146, 24, -124, -118, 240, 64, -35, -35, -63, -306, 229, -172, 73, 3, 124, 20, -77, 12, -154, -147, 97, 100, 153, -53, 267, 59, 36, 61, 87, -46, 51, -129, -327, 37, 170, 13, 220, 143, 67, -181, -91, -140, 87, 157, 92, -116, -248, 33, -113, -58, -206, -98, -59, -7, 83, -169, -24, 201, 166, -163, 76, 106, -39, 4, -27, 7, -308, -47, -30, 318, -86, 54, -230, 73, -47, 134, 267, 96, -42, -219, -45, 59, 19, -208, -8, -89, -9, -153, 396, -24, -246, -5, -44, 45, -139, -1, 80, -306, -90, -66, -111, 152, -201, 98, 206, 30, -219, 262, 15, 42, 7}
}
, {{-48, 168, -23, 48, 3, -37, 6, -143, 114, 171, 274, -37, -176, -31, 30, -43, -238, -25, -15, -53, 5, -68, -147, -54, 332, 47, -83, -11, 52, -64, -78, 101, -148, -274, 72, 79, 72, -74, -56, 47, 53, 100, -46, 192, -61, -323, -3, 291, 27, 122, -140, 36, 26, -91, 48, -10, 258, 185, -165, -187, -59, -65, -1, -8, 97, 130, 90, 83, 85, 74, -13, 26, -26, -18, -27, -176, 12, 189, -129, -79, 62, 83, -21, -105, -247, -85, 117, -65, 152, 110, -128, 31, -57, -68, -56, -68, -108, 34, -17, -94, 122, -23, -214, -70, -85, -32, 10, 44, -176, 244, -62, 24, 16, 128, -32, 111, 102, 136, -8, 164, 183, -117, -24, 253, 8, -66, -15, 131}
}
, {{19, 179, 48, 158, -58, 38, -130, -84, -13, 54, 106, 142, -49, 18, 56, -84, 119, -5, 144, -169, 264, -75, -180, -17, 167, -34, 127, -42, 25, -265, 159, -224, -41, -144, 113, 68, 108, 161, 52, -175, -25, 255, -181, 204, -19, -77, -136, 39, 59, -245, -47, 42, 92, 218, 7, 182, 202, -67, 110, 40, 70, 180, -182, 101, 142, 115, -18, 16, -226, -85, 60, -58, 91, -107, 17, 192, -73, 50, 148, -33, 75, -88, 117, 15, 62, 51, 224, 63, 21, 69, 88, 6, 83, 76, -190, -159, 182, -26, -83, -84, 224, 222, 16, 224, 118, 262, -44, -135, 76, 257, -117, 121, 104, -6, -13, -43, 141, 134, -31, 316, -36, -182, 29, 88, -175, 213, -40, 190}
}
, {{-90, -63, -47, -38, 96, -15, 344, -195, -117, 93, 96, -58, -12, 51, -200, 120, -106, 36, 194, -126, -167, 27, 17, -136, 194, -50, -18, -25, 107, 131, 208, -113, -299, -76, 32, -37, 15, -75, 3, 139, 91, 17, 80, 130, 56, 57, 54, 102, 137, -51, 54, -287, -227, 4, 89, 49, -81, -37, 29, 66, -328, 192, -53, -35, 83, 98, 61, -47, 29, -104, 82, -46, -109, 333, -10, -70, -149, 107, 63, -99, -219, 123, 23, 55, -80, 218, 103, -4, 88, 143, -57, 71, -7, -43, -158, -16, 146, -150, -185, -144, -84, 172, -112, -125, -106, -43, -117, 54, 183, 154, -74, -94, 71, 190, -7, 37, 295, -39, -165, -29, 96, 76, -49, -36, 12, -5, -82, 173}
}
, {{-110, 24, 65, 95, 202, -30, 22, 32, 2, 220, -15, -44, 131, 153, -58, -35, -136, -7, -69, 66, 126, -251, -438, 203, 162, 114, -47, -86, 156, 301, -94, -167, -76, 58, -42, -131, 99, 80, 8, 47, 310, 110, 71, 5, 76, 118, 71, 97, -152, 142, 107, -79, 43, -151, -118, 186, 13, 45, -111, 211, 69, 34, -38, -134, 24, -6, -2, -202, 82, 33, -3, -76, -93, 344, -40, -84, -157, 49, 192, -170, -59, 28, -252, 13, -130, -36, 28, -12, 64, -106, -54, 64, 158, -34, -49, 37, -21, 300, -235, -131, -319, -279, -142, 35, -356, -211, -106, -41, 164, 229, 107, -115, -50, 122, 88, 80, 119, -91, -146, -39, -105, 247, -16, -177, -79, 151, 34, -145}
}
, {{267, -140, 62, -25, 72, -94, 162, 85, -35, 60, 73, 89, 81, -105, 77, 47, 96, -131, 40, -61, -93, 67, -95, 193, -70, 85, 5, -150, -207, -64, 219, 25, -62, -153, -84, 207, -30, -111, -152, 57, 96, -157, 7, -57, 134, -96, -30, 96, -161, 214, -203, 1, 53, -101, 77, 79, 0, 22, -1, 13, -187, -142, 3, -112, 23, -17, 155, 95, 85, -130, -102, 17, -30, 125, 174, -224, 174, 109, -181, 42, -156, -34, 57, -75, -38, 103, 65, -63, 71, 134, -271, -135, 25, -144, 134, -81, 131, -53, 144, -77, -54, 104, -98, 2, 30, -148, -12, 79, -8, -23, -13, 109, -97, -237, 96, -20, -63, -62, 57, -103, 48, -89, 178, 77, 76, -131, -3, -75}
}
, {{55, 128, 178, -1, -26, 3, -116, -76, -130, 156, 85, -290, -225, -5, 301, 175, -48, 77, -40, 38, 47, -150, -19, 215, 60, -50, -81, 245, -107, 264, -141, -176, -215, 20, -234, 158, -71, 50, 17, -198, 80, 69, 88, -77, 88, -138, -75, -16, -95, 38, 41, 388, 18, 85, 56, 176, -89, 70, -103, 325, -78, 57, -97, -81, 242, 8, 121, -67, -151, 28, -16, -38, -58, -30, 70, 144, 11, -160, 174, -168, -11, -228, -51, -24, -22, -80, 33, -83, 152, -33, 168, -38, 81, -19, -14, -1, 30, 161, -13, -134, 41, -124, -222, 198, -44, 304, 146, 153, 12, 41, 245, -76, 56, 146, -28, -47, -61, 50, 9, -37, -31, 171, 163, -6, -133, 67, -12, -63}
}
, {{-173, 60, 13, -97, 71, -172, -63, -168, -71, 120, -244, -192, -283, 109, -203, 83, -181, 213, 226, 153, -36, 157, 316, -68, 185, -167, -298, 260, -115, 16, -159, 82, -124, 41, 212, -358, 68, -67, 127, 255, -293, 51, 177, 294, -77, 42, 113, -22, 73, -229, 19, -84, -157, -3, 43, -140, 208, 41, 173, 76, 17, 133, -143, -12, 259, 52, -56, 42, -109, 55, -52, 261, -128, 199, 145, 74, 32, 104, 276, -38, -108, -45, 228, -391, -68, 6, 223, 390, 291, -130, 83, -97, -121, 54, -71, 77, -188, -104, -128, 243, -151, -80, -19, -7, -110, 218, -45, -61, 52, 152, 18, 91, -201, 68, -107, -14, 85, 81, -32, -31, 222, -65, -357, 75, -166, 77, -45, 282}
}
, {{-131, -7, 44, 65, 53, -16, -59, -80, 87, -189, 2, 111, 35, -63, 34, -22, -81, 84, 89, 227, -83, 117, -28, 78, 148, -22, -88, 148, 67, -278, -46, 148, -72, -47, -89, 96, 85, 41, -217, 158, -9, -277, 188, -47, -48, -162, 267, 220, -44, 210, 132, -20, -44, -52, -17, -51, -31, 73, -15, -159, -26, -132, 238, -15, 187, -111, -14, 20, 203, 171, 26, 72, 2, 101, -255, -255, 150, 40, -112, 41, 11, -161, 79, -15, 28, -51, -189, 49, 189, 53, -282, -149, -86, -195, -195, -92, -109, 9, 9, -2, -51, -21, -241, -176, 49, -224, 350, 202, -124, 63, 129, 37, -40, 84, -169, 103, -177, -81, 228, -73, -128, 27, 44, 191, -50, -193, -43, -103}
}
, {{-7, 12, 85, -116, 70, -41, 51, 87, -87, -62, 34, -34, 137, 110, 25, 40, -33, -259, 81, -114, 26, 83, -184, 205, -107, -70, 20, 52, -58, 152, 86, 53, -7, -202, -44, 205, -57, -85, -235, 71, -87, 175, 128, -128, 284, 128, -27, 139, -200, 71, -90, -48, 51, -108, -132, 181, -66, 10, 107, -83, 128, 66, 63, -16, -138, 73, 163, 26, -140, -108, -127, -176, 118, -96, 127, 26, -85, 54, 74, 85, -87, -224, 46, -137, -120, -77, 205, -54, -129, 57, -34, -211, 112, -47, 130, -113, 70, -101, 22, -20, -161, 124, -59, 43, 42, -206, 190, -60, -27, 97, 68, 280, 77, -112, 193, -88, 122, 58, -100, 7, -100, 126, 161, -187, -101, 61, 278, 27}
}
, {{-32, -34, 96, 75, 34, 172, 207, 7, 198, 262, -129, -197, 120, 14, -80, 13, -11, -60, 95, 22, 71, -125, -280, 61, 126, 110, -39, 4, 247, 45, -85, -130, -63, 252, -17, -114, -104, -185, -33, 0, 161, 203, -97, -84, -58, 122, -102, -70, -31, 255, -70, -103, -6, -338, -92, -146, -90, 116, -207, 150, -73, -10, -61, -134, -153, 70, -17, 42, -23, 33, -116, -66, -60, 9, 154, 220, -69, -203, -53, -105, -33, -219, -24, -136, -136, -111, -102, 88, -149, 24, -147, -27, -13, -85, 31, 13, 100, 215, 33, -242, -135, 94, -15, -42, -158, -75, -91, 70, 15, 180, 249, -79, 39, -118, 62, -236, 155, -19, -189, -29, -141, 224, -47, -180, 9, -30, -41, 90}
}
, {{145, -79, -22, 20, -95, -211, 124, -44, 97, -45, 100, 35, 99, -169, 38, -21, 244, -321, -6, -16, 110, -161, -73, 3, -61, 103, -178, -180, -338, -116, 100, 5, 47, -53, -118, -32, 188, -20, -9, 125, 136, -92, 68, 114, 82, 118, -103, -73, -72, 46, -206, -40, -28, 104, -104, -62, 102, 11, -10, -213, -247, 17, -121, 46, -102, -53, 188, 10, -3, -100, 62, 145, -23, 114, 97, -149, 89, -105, -133, 66, -102, 24, 154, -57, 157, 103, -194, -25, 54, 50, -20, 80, 8, 93, 8, -117, 170, 70, -88, 174, 17, -45, -68, -88, 55, -112, 56, 45, -13, -39, 8, 21, -83, 7, 82, -12, -22, 34, 76, 18, -42, -109, 16, -16, 65, -74, -92, -27}
}
, {{-155, -29, 60, 119, 107, 34, 128, 58, -77, 120, -47, -239, 30, 149, 80, 36, 67, -141, 55, 21, 177, -29, -383, 141, -3, 257, 127, 9, 262, 252, -41, -42, -34, 200, -47, -101, 8, 117, 124, -107, 258, 125, 55, -56, -140, 5, -136, -198, -131, 126, 57, -138, 48, 76, -27, 121, 26, 53, -236, 343, 195, -192, -182, 1, -42, -43, -96, -102, -77, -65, 56, -7, -43, 111, -14, -1, -81, -168, -131, -160, -39, -42, 17, 109, 13, -97, -55, -115, -99, -97, 70, 67, -18, -103, -187, 65, 149, 356, 38, -188, -227, -230, -12, 26, 25, -136, -144, 18, -21, 251, 150, -139, -172, 82, 109, 36, 124, 77, -11, 38, -239, 75, -2, -10, 16, 85, -104, -72}
}
, {{18, 152, 9, -15, -8, -64, 250, -23, 3, 50, -78, 198, 141, 1, -2, -112, -30, -133, 190, -165, 21, 121, -12, 131, 38, 3, 29, -38, -16, -120, -7, 28, -96, 20, 114, 87, -6, -146, -66, -92, 19, 125, -73, 112, 33, 67, -6, -51, -131, 9, 39, 38, -122, 17, 212, 9, 76, 3, 84, -1, 57, -24, 64, -32, -132, 167, 96, -15, -163, 58, 36, -84, 8, 5, 113, 23, 99, 0, 17, 79, 82, -22, 48, 30, -63, 42, 87, -85, -24, 6, -140, -110, -40, 1, 29, -178, 210, 58, 11, 24, -46, -44, -73, 173, 3, 12, 73, -2, 69, 179, -115, 121, 148, 13, -28, 25, 84, -22, -184, 158, 191, 5, -63, -80, -144, 73, 71, 149}
}
, {{60, 14, 41, -110, -64, -125, -32, -3, -112, -54, 113, -131, -8, -63, 197, 131, 163, 16, -66, 27, -35, 51, -23, 37, -94, 62, 146, -160, -1, -5, 350, -95, 109, -113, -103, 276, 93, 143, -79, -266, -34, -27, -20, -155, -36, -203, -207, 100, -81, 162, 80, 260, 202, 74, 147, 74, -55, -121, -34, 97, -10, 112, -183, 6, 8, 74, -78, 162, 29, -183, -162, -155, 11, -19, 17, -205, 35, 10, 131, -87, 33, -149, 102, -63, -65, 24, -46, -81, 57, -15, 204, 43, 122, -117, -78, -58, 54, -126, 46, -91, 139, 176, -158, 252, -38, 136, 274, 32, -125, 24, 47, 3, 42, 158, -107, -13, 72, 6, 97, 32, -12, -6, 223, 159, 205, 51, 36, 10}
}
, {{97, -35, 70, 99, -1, -32, 93, -336, 180, 158, -95, -100, -269, -78, 62, 55, -121, -58, -48, 4, 196, 117, 18, -162, 255, 221, 203, -22, -69, 15, -137, 3, -5, -8, 12, -15, 81, 78, -32, 7, 158, -157, -76, 303, -144, 63, -127, -31, -10, 4, -123, -125, 30, 244, 32, -260, 256, 172, -45, 145, -117, -248, 0, 225, 17, 33, 12, -122, 18, -123, 142, 193, -151, 93, 59, 7, 186, 91, -182, -59, 121, -151, 204, -37, -4, 110, -139, -151, 43, 50, -37, -51, -115, 103, -121, 115, 152, 194, 52, 166, -85, -23, -45, -217, 49, 164, -309, -41, -46, 54, -168, 13, -21, 125, -176, 117, -49, 186, -37, 123, 10, -37, -156, 156, -88, -18, -37, 147}
}
, {{-147, -39, -131, -62, -63, 18, 128, 138, -215, 148, 18, 156, 77, -238, -106, 5, 153, 75, 153, 194, -81, 37, -129, -93, -50, 3, -158, 31, -343, 74, 88, -72, -11, 104, -28, -129, -50, 31, -121, 163, 360, -50, -25, -98, 147, -60, -3, -39, -159, -86, 58, -28, 80, 61, 9, -51, -151, 43, 155, -18, -156, -268, 345, -56, -158, -36, 7, -80, 208, 31, 85, 247, -86, 166, -45, 53, -106, 64, 37, 30, -43, -199, 52, -198, 83, -15, -24, 41, -21, -160, -96, -91, -1, 16, -120, 98, 55, 187, 80, 138, -88, -549, 82, -5, -64, -167, 156, 79, -36, -2, -62, 159, -435, -139, -74, 176, -23, -175, 36, -243, 73, 199, 66, -151, -137, 23, 50, 30}
}
, {{-13, 11, 35, -25, 64, -24, 25, 22, -79, 34, 4, 381, -258, -86, 164, -61, 220, 86, -134, -222, 54, -59, -86, 6, -24, 36, -107, -28, 130, -116, -7, -191, -129, -68, 48, 150, -41, -107, -34, -193, 170, 75, -142, 222, 92, 67, -10, 1, 16, -189, -200, 66, 20, -6, 266, 76, 232, 10, 200, -36, -46, -97, 96, -50, 77, -36, 156, -77, -31, -193, 142, 6, 52, -126, 59, -9, -5, 135, 28, 166, -54, 52, 48, -60, -78, 64, 17, -124, 13, 14, -117, -63, 142, 40, -142, -25, 38, 199, 107, 12, -64, -146, 100, 189, -103, 50, -362, -74, 49, 61, -146, 57, 111, -62, -312, 95, 73, 83, -169, 91, 54, 143, -73, -55, -248, 187, -21, 70}
}
, {{67, 3, 12, 84, -36, 141, -155, 24, -96, 36, 11, -131, 8, 11, 53, 10, -47, 98, -2, 13, -16, -177, -138, -18, -29, -8, 198, 132, 89, 148, -104, 12, -133, 52, -153, 60, 135, 186, -96, -14, 63, -77, 22, -5, 73, -89, 193, -6, 4, -80, -34, -80, 22, -65, -23, 52, -39, 225, -55, 275, -55, -137, 64, -24, 203, -83, 5, 99, 113, 124, 158, -13, -188, -142, -159, -86, 8, -139, 121, -98, 51, -107, -14, 192, -95, -29, -269, 4, 314, -140, -46, -18, -151, -96, -129, 141, -55, 195, -41, -103, -208, -173, -31, 109, 73, -229, 35, 171, -55, -28, 142, -62, -291, 57, -85, 174, -61, 54, 189, -26, -41, 59, -9, 242, -63, -119, -85, -136}
}
, {{44, 25, 43, 14, 35, -224, 117, 180, 34, 71, -166, -185, 97, -84, -200, 104, -17, -46, 96, 119, 100, -268, -200, 186, 198, 72, -220, 281, -57, 217, -367, -166, -81, 135, -11, -267, 106, -207, 0, 366, 382, -125, 316, -6, 189, 51, 155, -68, -172, -55, -51, -211, -72, 49, -27, -60, -88, 106, 95, 251, 12, 17, 84, 16, 3, -203, 109, -246, 173, -38, -188, 74, -35, 352, 176, 179, -42, -246, -156, -12, -87, -104, 0, -130, 5, 49, -261, -4, 96, -118, -185, -16, -24, -27, -225, 283, 168, 268, -500, -41, -421, -157, -196, -157, -179, -99, 8, 18, 132, -13, 159, 135, -133, -55, 13, 44, 125, -85, -244, -165, 79, 208, -70, -278, -110, 25, 190, -46}
}
, {{-125, -84, 137, -89, 38, 93, 207, 176, 20, 148, -6, 5, -48, 71, -21, 19, -194, -167, 67, 57, -53, -271, -94, -3, -121, 62, 159, 58, 109, 110, 13, -103, 3, 79, -25, -193, -3, 62, 84, 153, -37, 240, 191, 13, -56, -45, -48, -67, 259, -32, -150, 117, -89, 209, 39, 169, -190, -52, -138, 116, 130, 126, 145, 42, -242, -118, -33, 23, 36, -4, 102, -219, -116, 306, 81, -65, -190, 47, 62, -99, 20, 243, -174, 91, -35, 136, 178, -1, -318, 34, -47, 192, 255, -71, 18, 94, 136, -73, -42, 9, -136, 186, 11, 152, -66, 105, -174, 111, 133, 116, -108, -58, 83, 141, 141, -277, 15, -24, 82, -50, 21, 66, -233, -240, 148, -36, -134, 22}
}
, {{316, -51, -8, -80, 8, -101, -132, -181, -29, 77, 106, -12, -178, -75, 221, 102, 306, -110, -209, -3, 90, -94, 19, 20, 13, 86, 46, -206, -405, -53, 124, -85, 63, 56, -47, -74, 138, 126, -75, -61, 55, 64, 28, -58, 30, -105, -211, 5, -44, -43, 43, 119, 46, 184, -238, -271, 102, -46, 91, -124, -330, 15, -67, 162, -70, -33, 36, 49, 143, -139, 193, 268, -254, 13, -83, 53, 77, -18, -93, 89, 94, -188, 206, 24, 212, -65, -195, -39, -58, -86, 51, 149, 126, 92, -49, 193, 178, 226, 8, 193, 21, -83, -34, -6, 63, 141, -141, 15, -85, -117, -104, -95, -88, 134, 36, 122, -204, 78, 98, 39, 29, -145, 56, -150, 39, -91, -18, 102}
}
, {{0, -152, -24, 88, 138, 55, 79, 199, 123, -35, 182, -43, 42, 51, -2, -51, -212, 8, 21, -55, -255, -44, -50, 46, -96, 29, -18, -7, -186, 99, 225, 58, -35, -13, 55, 241, -22, 31, -86, -33, -58, -7, 64, -74, 126, 196, -116, -1, 130, 4, 115, -78, 144, -125, 23, 42, -122, 40, 180, 28, 98, -130, -112, 19, -145, 56, 161, 63, 81, -81, -145, -65, 75, 56, 129, -38, 15, 58, 54, 22, 130, 47, 48, -98, -167, -154, 121, -46, -27, 330, 28, -125, -30, -145, 1, -57, 21, -241, 106, -116, 163, 31, 26, -142, 11, -147, 143, 1, -61, 130, -53, 119, 116, -279, 147, -8, 155, 106, 22, 17, 134, 125, 138, 52, 5, 187, 115, 105}
}
, {{-85, 33, 44, -167, -65, -13, 59, 137, 142, 118, 19, 106, -6, -71, -75, 25, -243, 29, -24, 158, -108, 22, 11, -45, -106, -76, -25, 35, 65, -80, 172, 82, 126, 63, -108, 22, -119, -67, -56, 221, -149, -183, 183, -44, -17, 28, 71, 240, 119, -9, -46, -50, 98, -3, -61, 29, -143, -6, 158, 28, -115, 66, 199, -203, 58, -57, -97, -48, 192, 137, -189, -75, -101, 239, 15, -27, -195, 116, -31, -39, -72, 194, 55, -246, -165, 12, 26, -44, -4, -31, -3, 45, 105, -112, 229, -80, -51, -239, -2, -244, -123, 42, -49, -72, -138, 34, -150, 139, -46, -40, -26, 66, -42, 87, -10, 12, -72, -39, 134, -194, -94, 99, 124, 19, 264, 7, 4, 16}
}
, {{-117, -69, 44, -275, -65, 65, 174, -66, 105, 135, -21, -15, -111, 124, -93, 3, -154, 211, 22, 261, -220, 43, 188, -435, -287, -235, -12, 190, 23, 135, -108, 76, 75, 34, -79, -266, 121, 96, -68, 91, -333, 132, 143, -22, 129, 55, -72, 122, 200, -43, -55, 256, -8, 134, -75, 142, -202, -292, -44, -32, -108, 191, 241, -124, -164, -72, -166, -29, 182, 145, 152, 5, -46, 129, -51, 122, -92, 85, 201, -25, -12, 226, -168, -51, -87, 159, 119, 320, -296, 56, 38, 43, -35, 30, 55, -108, 14, -450, 220, 13, 19, -88, -91, 36, -205, 209, -111, -62, -139, -106, -65, 108, 14, 72, -90, -236, -21, -177, 191, -134, 198, 170, -23, -66, 323, -80, -263, -7}
}
, {{-174, 155, -158, -176, 208, 58, 29, 1, -154, 118, -130, -439, -200, -172, 241, 147, 46, 295, 187, 91, -197, 210, -55, 12, 72, -95, 15, 236, -294, 118, -92, 118, -69, 80, 44, 26, -32, -22, -10, 19, 243, 57, 165, -19, 97, 14, 109, 30, -281, -72, -135, 71, -43, -68, 102, -30, -340, 20, -101, 57, -99, 71, 317, -140, 110, -149, 58, -8, -66, 80, 210, 211, -116, 253, 147, 259, 97, 110, 85, 182, 18, 76, 77, -213, 22, -90, 179, -56, 100, -347, 6, 13, -122, 178, 89, 280, -185, 117, -284, 123, -257, -277, -11, 106, -164, 90, -152, -71, 36, -169, -30, 137, -53, 13, -102, 263, -8, -180, -32, -232, 29, 138, 32, -266, -167, -83, 35, 9}
}
, {{-48, 72, 38, -1, 97, 63, 43, 234, -119, 216, 114, 173, -84, 71, 31, -85, -59, -132, 48, -19, -32, 27, -227, -14, -205, 49, 45, -43, -13, -33, 181, -103, 93, 84, -46, 2, -147, 45, 10, -3, 25, 54, -12, -11, 49, 52, -52, 92, -164, 71, -158, 211, 113, 105, 19, 98, -72, -9, 13, 46, -41, 61, 48, -144, -33, 11, -65, 94, 77, -42, 66, -30, 78, 139, -91, 81, -214, 17, -53, -214, -13, 45, 57, 8, -170, -130, 43, -112, -202, -8, 91, 229, 244, -111, -129, 28, 227, -10, -42, -52, -273, -187, -90, 66, -224, 54, -7, 7, -117, 134, -28, 21, 48, -18, 90, 55, -154, 7, 12, -133, -95, 224, 62, -147, 128, -12, -90, 100}
}
, {{-200, -27, 67, -75, -247, -28, 4, 21, -10, 118, -45, -177, 33, 168, 26, 39, 4, 62, -88, 115, 30, 18, -5, -86, -67, 44, -256, 101, 111, -79, 77, -60, 63, 296, -18, -91, -79, -13, 87, 36, 8, 121, -137, -50, -198, 14, -70, -74, -2, 171, -146, 151, 129, -71, 151, -79, -145, -93, -173, 332, -26, -34, 87, -73, -114, 27, -76, -30, -114, 116, -121, 29, -147, 12, 65, 101, -20, -3, -109, -103, 67, -62, -12, -7, -89, -35, 37, 84, -243, -128, -71, 98, -162, -2, -130, -1, 64, 104, -37, -37, -172, -136, -38, 13, -6, 60, -85, 61, -175, 74, 121, -151, 98, 95, -130, -150, -31, -88, 10, -57, -140, 87, 35, -41, -24, -106, -5, 4}
}
, {{-81, -156, 19, -23, 11, -61, 79, 189, 63, -61, 39, 228, 109, -6, -28, -116, 85, 94, 155, 57, -47, 49, 100, 8, -124, -160, 14, 35, -112, -112, 73, 219, 28, 6, -114, 65, -33, -95, -205, 102, -117, -59, 100, -95, 161, 138, 99, 66, 96, 100, 75, -13, 56, -176, -216, 99, -11, 26, 154, -89, 280, -1, 225, 28, -93, -47, 3, -164, 92, 35, -197, -122, -10, 108, 74, -81, -111, 37, -54, -9, 155, 105, -62, -239, -30, -147, 188, 40, -31, -94, -203, -264, -22, -134, 36, -195, 96, -240, 61, -9, 11, 0, 11, -71, 5, -202, 235, 184, -95, 40, 9, 127, -103, -41, 27, 16, -52, -34, 27, -103, 52, 144, 157, -7, 54, 103, 351, -47}
}
, {{-105, -182, 25, -50, -79, -21, 174, 119, -157, -94, -222, -77, 53, -2, -15, 76, 404, 177, -135, -14, 173, -5, -25, 27, 28, -35, 93, -71, -19, 32, 4, -105, 70, 161, -117, 95, -161, -129, 107, -363, -5, 48, -69, -43, -79, -107, -58, -73, 70, 100, -154, 72, 126, -2, 190, -79, 30, -152, 115, 65, -33, -144, 94, -152, -132, 12, 114, -23, 104, -14, 73, 99, 27, -160, -41, 123, 62, -15, -22, 156, 74, 38, -101, 54, 43, 63, -78, -117, -70, -20, 6, -69, -24, 38, -97, -1, -56, -83, 119, 67, 12, -126, 94, 139, -92, 190, -121, -191, -43, -61, -174, -32, 59, 114, -24, -142, -231, -76, -50, -18, 61, -120, -118, 29, 9, 11, 37, 124}
}
, {{277, 42, 64, -62, 116, -6, 6, 64, 19, 5, 143, -154, -25, -127, 107, 190, -60, -63, -165, 4, -102, 9, -24, 106, 126, 22, -62, -73, 34, 103, -70, -127, -82, -98, -113, 60, 145, 74, 26, 42, -17, -42, 225, -95, 139, -118, 70, 129, -76, 134, -40, 43, 30, 114, 54, 97, -104, -41, -92, -193, -136, 40, -101, -34, 125, 42, 47, 81, -1, -43, 117, 175, 6, 17, -77, -120, 49, -100, 31, -28, -128, 7, -125, 22, 26, -85, 65, -40, 163, 56, 147, 18, 98, -40, 59, 120, -62, -20, -126, -20, -77, -14, -150, 49, 9, -99, 3, 19, 17, -58, 153, -82, -30, 237, 47, 27, 36, -15, 70, -106, -29, 10, -3, 115, 113, -73, 62, -101}
}
, {{-42, 31, -84, -96, 50, -160, -136, 103, -122, -189, 138, 183, 42, 109, -34, -49, 90, -180, 100, -106, 91, 68, -23, 219, -90, 29, 88, 132, -112, -62, 264, 98, 27, -65, -46, 334, -44, -97, -142, 41, -57, -37, -148, -7, 137, 24, 63, 110, -186, 96, -90, 43, 96, -107, 29, 236, -18, 75, 112, -57, 203, -166, 14, -78, 52, 45, 201, -4, -149, 98, -110, -140, 211, -55, -1, 72, 100, -24, -85, 93, 56, -73, 49, -67, -207, -125, 160, -12, -23, 155, -133, -175, -9, -157, 226, -191, -58, -21, 96, 12, -125, 109, -119, 30, -51, -81, 227, 111, 50, 62, 1, 243, 159, -131, 52, -69, -37, 88, 2, 5, -121, 56, 175, -8, -166, 90, 33, -140}
}
, {{172, 9, 128, -58, -94, -176, -38, 27, 110, 77, 144, -71, -124, 28, 22, 159, -24, -135, -34, 14, 38, -204, 4, -18, -14, 265, 166, -334, 44, -111, 336, -104, 49, -172, -212, 29, 57, 138, 45, -109, -158, -153, 254, -83, -86, -228, -131, -20, -52, -104, 71, 162, 115, -12, 45, -33, 53, -59, -274, 120, 1, 21, -43, 97, 110, 106, -135, 55, 65, -5, -113, -71, -242, 177, -164, -155, 45, 88, 64, -154, -103, 212, 145, 85, -1, 11, -166, -186, 49, 75, 172, 144, 139, 8, 22, -81, -31, -142, -43, 19, -120, 354, -212, 89, 26, 150, 43, 65, 175, -23, -35, -217, 17, 46, -104, 96, -25, 186, 207, 171, -53, -62, -49, -26, 93, -6, -3, 140}
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

#define INPUT_CHANNELS  128
#define INPUT_SAMPLES   12
#define POOL_SIZE       12
#define POOL_STRIDE     12
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

#define INPUT_CHANNELS  128
#define INPUT_SAMPLES   12
#define POOL_SIZE       12
#define POOL_STRIDE     12
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

#define OUTPUT_DIM 128

typedef int16_t cnn_layers_flatten_output_type[OUTPUT_DIM];

#if 0
void cnn_layers_flatten(
  const number_t input[1][128], 			      // IN
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

#define OUTPUT_DIM 128

#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t

static inline void cnn_layers_flatten(
  const NUMBER_T input[1][128], 			      // IN
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

#define INPUT_SAMPLES 128
#define FC_UNITS 11

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

#define INPUT_SAMPLES 128
#define FC_UNITS 11
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

#define INPUT_SAMPLES 128
#define FC_UNITS 11


const int16_t cnn_layers_fc1_bias[FC_UNITS] = {-14, 28, -4, -14, -67, -15, -24, -20, 68, -16, 12}
;

const int16_t cnn_layers_fc1_kernel[FC_UNITS][INPUT_SAMPLES] = {{-135, -120, -21, 85, 42, -231, 171, 58, 52, -98, -113, 6, -176, 84, 5, 19, -66, -125, 58, -169, -46, -50, -61, -84, 81, -38, -132, -8, 25, -104, -21, 135, -121, -32, -88, 36, -30, -21, -272, 84, -9, -43, -5, 31, -137, 42, -70, -286, 99, 136, -43, 165, -73, 121, -58, 62, 123, -105, -207, -6, 156, -139, -138, -149, 146, -71, 4, -44, 103, 114, -73, -90, -30, -2, -200, -97, -168, 80, 71, 68, -136, -105, -41, -135, -27, 26, -154, 157, 44, 95, 177, -230, 71, -54, -69, -23, 101, -160, 21, 25, 38, 86, -7, 167, -219, -230, 51, -46, -301, 62, 88, -31, -33, 123, -39, -155, 139, -207, -98, -110, 67, -131, -164, -280, -75, 163, -183, 106}
, {70, -13, -205, -286, 146, 80, 13, -51, 90, -20, -251, 139, -63, -5, 112, 29, -164, -196, 42, 99, -104, 128, 119, -170, -158, 17, 82, -42, 14, -62, -10, -269, -44, -205, 187, 252, -87, -67, 37, -50, -255, -55, -32, -140, -143, -37, -62, -7, -13, 110, -130, -164, -80, 99, 193, -61, -163, -2, -27, -200, 33, -94, -3, 75, -51, 154, 1, -118, -143, -169, -161, 246, 102, 239, 112, -17, 10, 18, 144, -123, 190, -228, -57, -208, -189, -66, -30, -13, -237, -22, -168, 99, -38, -158, -78, 148, -45, -91, 32, -35, -17, -83, 174, 77, 61, 10, 6, -217, 4, -273, -266, 80, -125, -134, 93, -90, -126, -35, 102, 128, 131, -95, -120, 102, -200, -70, 60, -372}
, {32, 107, 166, 79, -145, 44, 143, -290, 152, -45, -114, 42, 76, 8, -245, 162, 40, -64, 103, -31, 3, 128, 96, 24, -318, 37, 67, -9, 230, 176, -4, -101, -191, 71, 104, -121, -129, 231, 30, -184, -33, 7, 162, 154, 13, -174, 35, 13, -211, 81, 155, -213, -114, -100, -185, -152, 222, 43, 81, -218, -103, 86, -86, -1, 176, -143, -230, -6, -61, -132, -24, -98, 64, -240, 46, 103, 27, 146, -271, -134, 116, 8, -228, -83, 123, 18, 81, -92, -97, 64, 64, 113, 106, -125, -10, -208, 48, -233, -240, -175, 93, -203, -233, 175, 39, 12, -2, -39, 13, -7, -184, -39, -110, 43, -266, -225, -210, 95, 130, -38, -245, -24, 70, 117, 20, -62, 98, -4}
, {200, -128, -191, 47, 98, -82, -104, -141, 129, -287, 160, -110, -208, -177, -75, 108, 176, -128, -286, 81, 3, 34, -91, 188, 133, -108, 59, -146, -128, -126, 94, -14, -69, 24, 7, -82, -138, 173, 123, 67, 17, -12, 47, -82, -231, -125, -49, 74, 154, -243, -104, -66, -97, -80, 94, 27, -4, 71, 76, -4, -36, 132, -164, -167, -22, -175, -130, 7, -109, 162, -126, -155, -269, 111, -83, -142, 130, 143, -270, -234, -18, -83, -109, 58, -105, 112, 30, -100, 221, -350, -128, -161, -164, -61, -211, 106, -22, 119, -178, -61, 77, 130, -189, -151, 75, -2, 12, -22, -40, 200, -204, -149, -165, -151, -85, 20, 55, 88, 152, 7, -92, 128, 27, 9, -96, 104, 74, 130}
, {141, -105, 129, -143, -164, -13, -53, 102, -104, -162, -115, 61, -245, -108, -320, -127, -68, 113, 32, -263, -206, 35, 94, -78, -148, -275, -156, -45, 15, 130, 124, 112, -41, 67, 204, -141, -100, -107, -221, 2, -1, -166, 69, -50, 132, -61, 45, -231, -252, -87, 118, 83, -132, -42, -176, 205, -201, 228, -385, 146, -94, 35, 77, -138, 23, -98, 131, 228, -23, -90, 144, -152, -92, 43, 157, 48, -141, -242, 48, 114, 45, -35, 48, 124, -8, -157, 33, 196, 4, 111, -65, 12, -265, 119, -219, -28, 210, 168, 167, 51, -228, -115, 143, -26, -308, 11, -262, -13, 28, -102, 157, -133, 130, -167, -179, 15, -247, 89, -82, -80, -106, -21, -130, -108, -108, -168, -155, 38}
, {-22, -159, 190, -108, -106, -217, -43, -105, -94, -23, -37, 65, -22, 97, 58, 49, 101, -178, -156, -130, 119, -237, -240, -175, -38, -165, -80, -59, 68, -229, -266, -177, 61, 198, -24, 110, -280, 46, -349, 155, 81, -343, 205, -191, 147, -269, -26, -151, -76, 57, -68, -189, 127, -178, -178, -212, 63, -172, -227, -189, -72, -166, 24, -73, -72, 34, 162, 110, -285, -15, 134, 90, -133, -12, -217, -96, -21, -49, -133, 130, -59, -87, -191, 185, -117, -15, -192, 146, 151, -331, -24, -193, 136, -216, -53, 29, 2, -79, -256, 138, -134, 97, -111, -34, -143, 111, 16, 132, -58, 34, 99, 194, -20, 98, 170, -63, 98, -204, -112, -193, 167, 144, 79, -178, 40, -104, -269, -108}
, {-143, -217, 112, -54, 175, -116, 72, -18, 94, 208, 81, -47, 144, -163, 210, 115, -27, -149, -140, -235, -171, -202, -126, -79, -8, 143, -185, 179, 106, -101, -271, -155, -104, -48, 41, 32, -55, -47, -129, -133, 96, -96, -213, -80, 187, 29, -142, -233, 79, 102, -161, 86, -90, 81, 82, -165, 50, 133, 141, 149, -144, 180, 163, -47, 27, -139, 68, 63, -4, -60, 17, 233, -80, 5, -209, 159, -54, 121, 53, -132, -201, 91, 171, -85, 10, -87, -58, -165, -163, 115, -101, 112, -36, -63, 177, 56, -105, 88, -54, -207, -270, -114, 72, -200, -338, -124, 160, 3, -88, -32, 102, -149, -111, -95, -130, 148, 136, -213, -40, 139, -112, -22, 80, -316, -100, -73, -248, 72}
, {-78, -42, 0, 34, -115, 33, -64, -96, -382, 62, -54, 55, 150, -13, 152, -294, -279, -21, 86, 48, 13, -37, -86, -71, 78, -116, 50, -123, -89, 43, -53, -204, 54, 118, -194, -158, 60, -292, 112, -114, -66, 135, -160, -189, -12, -32, -242, 118, 95, 81, -130, -65, 61, 198, 19, 95, -115, -274, 64, 129, -129, -160, 234, 80, -82, 50, 178, -16, -123, -69, -101, -56, 127, -164, 40, 70, -25, -19, 89, 179, 73, -127, -320, 99, -12, 107, -20, -104, -147, -131, 81, 125, 158, -112, 63, 159, -227, 110, -177, -118, 27, 103, -14, -93, 28, 30, -119, -75, 96, 54, -46, -156, 166, 4, -124, -282, 9, -7, -334, -181, 110, -191, 101, -41, 39, -64, 90, -221}
, {19, 18, -136, -123, -84, 52, -66, 194, -95, -164, 71, -136, -162, 270, 21, -178, -84, 158, 4, 102, 57, -61, -71, 44, -57, -46, 94, 48, -58, -22, 97, -41, -167, -184, -183, -140, 63, -55, 0, -163, -206, 38, -171, 10, 186, 59, -54, -1, -47, 65, -344, -124, 94, -108, -33, 20, 12, 25, 52, 41, 132, -121, -272, -77, -4, -247, -138, -163, 67, -153, -185, 94, -207, -49, 56, -1, 118, -114, -198, 17, -182, -115, 48, 93, 212, -84, 93, -90, -194, -90, -84, -162, 16, 155, 166, -275, -176, -202, 97, 136, 89, -65, -270, -28, 44, 156, 115, 197, 30, -115, 50, -76, -253, -106, 201, 19, -56, 75, -89, -291, -69, -80, -28, 19, -15, 54, 58, -148}
, {-73, 114, -178, 27, -321, 91, -225, -153, -28, 156, -88, -42, 116, -127, -84, -63, -131, 133, -147, 41, 190, 51, 118, -63, -4, 182, 91, 226, -52, -148, -106, -51, 151, -11, -147, 51, 103, -147, 48, -119, -229, 97, 4, 104, -45, -155, 43, 86, -33, -306, 44, 124, -137, -70, -141, -147, -54, -94, 46, -198, 111, 1, -31, 151, -96, 65, -67, -230, -125, -123, -107, -205, 96, -78, -93, -254, 59, -276, -46, -24, -88, 71, 56, -132, -190, -162, -51, -160, 57, -30, 2, -24, -230, 136, -30, 11, -32, -173, 134, -92, 50, -169, -3, -63, -28, -121, 102, -210, 83, -144, 40, 162, 106, -115, 26, 50, 134, 13, -70, 7, 106, 52, -146, -2, 115, -104, -42, -84}
, {-17, 124, -70, 85, 52, -113, -149, 108, -22, -40, 81, -147, -83, 14, -61, -150, 75, 93, 95, -215, -3, -25, -81, 74, 79, 102, -271, -241, -203, 101, -91, 132, 131, -58, -158, -50, 165, -4, -43, 78, 47, -91, -35, 167, -104, 165, 156, -153, 22, -95, 115, -8, 99, -97, 55, -39, -53, -111, -185, 5, -31, -6, -118, -112, -207, 79, -160, -117, 140, 43, 60, -142, -63, -2, 50, -92, 95, -146, 55, 34, 31, 165, 98, -81, 86, 84, -222, -129, -19, 53, -13, -23, -12, 38, 87, -70, -109, 28, 14, 81, -218, 84, 34, -38, -182, 34, -207, 108, -59, 2, -17, -79, 28, 85, 49, 143, -99, -97, 57, 122, -65, 96, 137, -5, 93, -20, -225, 15}
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
#include "cnn_layers_conv2_dw.h" // InputLayer is excluded
#include "cnn_layers_conv2_pw.h" // InputLayer is excluded
#include "cnn_layers_conv3_dw.h" // InputLayer is excluded
#include "cnn_layers_conv3_pw.h" // InputLayer is excluded
#include "cnn_layers_conv4_dw.h" // InputLayer is excluded
#include "cnn_layers_conv4_pw.h" // InputLayer is excluded
#include "cnn_layers_postpool.h" // InputLayer is excluded
#include "cnn_layers_flatten.h" // InputLayer is excluded
#include "cnn_layers_fc1.h"
#endif

#define MODEL_NAME cnn


#define MODEL_INPUT_DIM_0 186
#define MODEL_INPUT_DIM_1 10
#define MODEL_INPUT_DIMS 186 * 10

#define MODEL_OUTPUT_SAMPLES 11

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
#include "cnn_layers_conv2_dw.c"
#include "weights/cnn_layers_conv2_dw.c" // InputLayer is excluded
#include "cnn_layers_conv2_pw.c"
#include "weights/cnn_layers_conv2_pw.c" // InputLayer is excluded
#include "cnn_layers_conv3_dw.c"
#include "weights/cnn_layers_conv3_dw.c" // InputLayer is excluded
#include "cnn_layers_conv3_pw.c"
#include "weights/cnn_layers_conv3_pw.c" // InputLayer is excluded
#include "cnn_layers_conv4_dw.c"
#include "weights/cnn_layers_conv4_dw.c" // InputLayer is excluded
#include "cnn_layers_conv4_pw.c"
#include "weights/cnn_layers_conv4_pw.c" // InputLayer is excluded
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
    cnn_layers_conv2_pw_output_type cnn_layers_conv2_pw_output;
    cnn_layers_conv3_pw_output_type cnn_layers_conv3_pw_output;
    cnn_layers_conv4_pw_output_type cnn_layers_conv4_pw_output;
  } activations1;

  static union {
    cnn_layers_conv2_dw_output_type cnn_layers_conv2_dw_output;
    cnn_layers_conv3_dw_output_type cnn_layers_conv3_dw_output;
    cnn_layers_conv4_dw_output_type cnn_layers_conv4_dw_output;
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

  
  
  
  cnn_layers_conv2_dw(
    activations1.cnn_layers_conv1_output,
    cnn_layers_conv2_dw_kernel,
    cnn_layers_conv2_dw_bias,
    activations2.cnn_layers_conv2_dw_output
    );

  
  
  
  cnn_layers_conv2_pw(
    activations2.cnn_layers_conv2_dw_output,
    cnn_layers_conv2_pw_kernel,
    cnn_layers_conv2_pw_bias,
    activations1.cnn_layers_conv2_pw_output
    );

  
  
  
  cnn_layers_conv3_dw(
    activations1.cnn_layers_conv2_pw_output,
    cnn_layers_conv3_dw_kernel,
    cnn_layers_conv3_dw_bias,
    activations2.cnn_layers_conv3_dw_output
    );

  
  
  
  cnn_layers_conv3_pw(
    activations2.cnn_layers_conv3_dw_output,
    cnn_layers_conv3_pw_kernel,
    cnn_layers_conv3_pw_bias,
    activations1.cnn_layers_conv3_pw_output
    );

  
  
  
  cnn_layers_conv4_dw(
    activations1.cnn_layers_conv3_pw_output,
    cnn_layers_conv4_dw_kernel,
    cnn_layers_conv4_dw_bias,
    activations2.cnn_layers_conv4_dw_output
    );

  
  
  
  cnn_layers_conv4_pw(
    activations2.cnn_layers_conv4_dw_output,
    cnn_layers_conv4_pw_kernel,
    cnn_layers_conv4_pw_bias,
    activations1.cnn_layers_conv4_pw_output
    );

  
  
  
  cnn_layers_postpool(
    activations1.cnn_layers_conv4_pw_output,
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
