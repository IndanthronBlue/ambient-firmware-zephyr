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

#ifndef _STEM_0_H_
#define _STEM_0_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      9
#define INPUT_SAMPLES       186
#define CONV_FILTERS        32
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         2

#define ZEROPADDING_LEFT    1
#define ZEROPADDING_RIGHT   1

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

typedef int16_t stem_0_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void stem_0(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const number_t kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS],  // IN

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

#endif//_STEM_0_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stem_0.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#elif defined(WITH_NMSIS_NN)
#include "riscv_nnfunctions.h"
#endif

#define INPUT_CHANNELS      9
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

#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stem_0(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const NUMBER_T kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS],  // IN

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

#error "CMSIS-NN requires the use of bias"

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
  * @file    weights/conv1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

#define INPUT_CHANNELS    9
#define CONV_FILTERS      32
#define CONV_KERNEL_SIZE  3
#define CONV_GROUPS       1


const int16_t  stem_0_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{-1, -1, -1, -1, -1, 0, -1, -1, 0}
, {0, -1, -1, -1, 0, 0, 0, 0, -1}
, {-1, -1, 0, -1, 0, 0, 0, -1, 0}
}
, {{0, -1, -1, 0, 0, 0, -1, 0, -1}
, {0, 0, -1, 0, 0, -1, 0, -1, -1}
, {0, 0, -1, -1, -1, 0, -1, -1, -1}
}
, {{-1, -1, 0, 0, -1, 0, -1, 0, 0}
, {-1, -1, 0, -1, -1, -1, -1, 0, -1}
, {0, -1, 0, -1, 0, 0, -1, -1, -1}
}
, {{-1, -1, -1, 0, -1, -1, 0, -1, -1}
, {0, -1, 0, 0, -1, -1, 0, -1, -1}
, {-1, -1, -1, -1, 0, 0, -1, 0, 0}
}
, {{0, -1, 0, -1, -1, 0, 0, -1, -1}
, {-1, 0, 0, -1, 0, -1, -1, 0, 0}
, {0, 0, 0, 0, -1, -1, 0, 0, -1}
}
, {{-1, 0, -1, 0, 0, 0, -1, -1, -1}
, {-1, -1, 0, -1, -1, -1, 0, 0, 0}
, {0, -1, -1, 0, 0, -1, -1, 0, -1}
}
, {{0, -1, -1, 0, 0, 0, -1, -1, -1}
, {-1, 0, 0, -1, -1, -1, -1, 0, -1}
, {-1, -1, 0, 0, -1, -1, 0, 0, 0}
}
, {{-1, 0, -1, 0, -1, -1, 0, -1, 0}
, {-1, 0, 0, 0, -1, 0, 0, 0, -1}
, {0, -1, 0, -1, 0, -1, -1, -1, -1}
}
, {{-1, 0, -1, -1, -1, -1, -1, -1, -1}
, {0, 0, -1, 0, -1, -1, -1, 0, -1}
, {-1, -1, 0, 0, -1, 0, -1, 0, 0}
}
, {{-1, 0, 0, 0, -1, 0, 0, -1, 0}
, {-1, 0, -1, -1, -1, 0, -1, 0, -1}
, {0, -1, -1, -1, -1, 0, -1, 0, 0}
}
, {{0, 0, -1, -1, -1, -1, -1, 0, -1}
, {0, 0, -1, -1, -1, -1, 0, 0, -1}
, {0, -1, 0, -1, -1, 0, -1, 0, 0}
}
, {{-1, 0, -1, -1, 0, -1, -1, -1, 0}
, {-1, 0, 0, 0, -1, 0, 0, 0, 0}
, {-1, -1, 0, 0, 0, -1, 0, 0, -1}
}
, {{-1, 0, -1, 0, 0, -1, -1, 0, 0}
, {0, -1, -1, -1, -1, 0, -1, -1, 0}
, {0, 0, -1, 0, -1, -1, 0, -1, -1}
}
, {{0, 0, 0, 0, 0, -1, 0, -1, -1}
, {0, 0, -1, -1, -1, -1, -1, 0, 0}
, {0, -1, 0, -1, 0, 0, -1, -1, -1}
}
, {{0, 0, -1, -1, 0, 0, 0, 0, -1}
, {0, -1, 0, -1, -1, -1, -1, 0, 0}
, {-1, -1, 0, -1, -1, -1, -1, 0, -1}
}
, {{-1, 0, 0, 0, 0, -1, -1, -1, 0}
, {0, -1, 0, 0, -1, 0, -1, 0, 0}
, {-1, 0, 0, 0, -1, -1, -1, -1, 0}
}
, {{-1, 0, -1, 0, 0, -1, 0, 0, 0}
, {0, -1, 0, 0, -1, -1, -1, 0, -1}
, {0, -1, 0, 0, 0, -1, 0, -1, -1}
}
, {{0, -1, 0, -1, 0, 0, 0, 0, 0}
, {0, 0, 0, 0, -1, -1, 0, -1, -1}
, {-1, 0, -1, -1, 0, -1, 0, 0, -1}
}
, {{-1, 0, -1, 0, 0, 0, -1, -1, -1}
, {-1, -1, -1, 0, -1, -1, -1, -1, -1}
, {0, -1, 0, -1, 0, 0, 0, -1, 0}
}
, {{0, -1, 0, 0, -1, 0, 0, 0, -1}
, {0, -1, -1, 0, -1, -1, -1, 0, 0}
, {-1, -1, -1, 0, 0, 0, -1, -1, 0}
}
, {{0, -1, -1, 0, -1, -1, 0, -1, 0}
, {0, -1, 0, 0, 0, -1, 0, -1, 0}
, {-1, -1, 0, -1, 0, 0, -1, 0, -1}
}
, {{-1, 0, -1, 0, -1, 0, -1, 0, 0}
, {-1, 0, -1, 0, 0, 0, -1, 0, -1}
, {0, -1, 0, -1, -1, -1, -1, -1, 0}
}
, {{0, 0, -1, -1, -1, 0, 0, -1, 0}
, {0, -1, 0, -1, 0, -1, -1, -1, -1}
, {-1, -1, 0, 0, 0, -1, 0, -1, 0}
}
, {{-1, -1, 0, -1, 0, -1, -1, -1, -1}
, {-1, -1, -1, -1, -1, 0, 0, 0, 0}
, {0, 0, 0, 0, 0, 0, -1, -1, -1}
}
, {{0, 0, 0, -1, 0, -1, 0, -1, -1}
, {-1, 0, 0, -1, 0, 0, -1, -1, -1}
, {-1, -1, 0, 0, -1, -1, 0, -1, 0}
}
, {{0, 0, 0, 0, 0, 0, -1, 0, 0}
, {0, 0, 0, 0, -1, 0, 0, -1, 0}
, {0, -1, -1, -1, -1, -1, -1, 0, 0}
}
, {{0, 0, -1, 0, -1, 0, -1, 0, 0}
, {-1, -1, -1, 0, -1, -1, 0, -1, -1}
, {0, -1, -1, -1, -1, 0, -1, -1, -1}
}
, {{0, -1, 0, -1, -1, 0, -1, 0, -1}
, {-1, -1, -1, 0, -1, -1, 0, -1, -1}
, {-1, -1, 0, 0, -1, -1, 0, -1, -1}
}
, {{-1, -1, -1, -1, 0, 0, -1, 0, -1}
, {0, 0, 0, -1, 0, -1, -1, 0, 0}
, {-1, 0, -1, 0, 0, -1, 0, 0, 0}
}
, {{-1, -1, 0, -1, -1, 0, 0, -1, -1}
, {-1, -1, -1, -1, 0, 0, 0, 0, 0}
, {-1, 0, -1, -1, -1, -1, -1, 0, 0}
}
, {{0, -1, -1, 0, -1, -1, -1, 0, -1}
, {0, 0, -1, 0, -1, 0, -1, -1, 0}
, {0, 0, 0, 0, 0, 0, -1, 0, -1}
}
, {{0, -1, -1, -1, -1, -1, 0, -1, -1}
, {-1, -1, -1, -1, 0, 0, -1, 0, -1}
, {-1, -1, -1, -1, -1, -1, 0, -1, 0}
}
}
;

#undef INPUT_CHANNELS
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_GROUPS
/**
  ******************************************************************************
  * @file    batchnorm1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    21 april 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STEM_1_H_
#define _STEM_1_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      32
#define INPUT_SAMPLES       93

typedef int16_t stem_1_output_type[93][32];

#if 0
void stem_1(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const number_t kernel[INPUT_CHANNELS],                // IN
  const number_t bias[INPUT_CHANNELS],                  // IN
  stem_1_output_type output);                // OUT
#endif

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES

#endif//_STEM_1_H_
/**
  ******************************************************************************
  * @file    batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stem_1.h"
#include "number.h"
#endif

#define INPUT_CHANNELS      32
#define INPUT_SAMPLES       93
#define ACTIVATION_RELU6

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stem_1(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const NUMBER_T kernel[INPUT_CHANNELS],                // IN
  const NUMBER_T bias[INPUT_CHANNELS],                  // IN
  stem_1_output_type output) {                // OUT

  LONG_NUMBER_T tmp;

  for (int x = 0; x < INPUT_SAMPLES; x++) {
    for (int z = 0; z < INPUT_CHANNELS; z++) {
      tmp = (LONG_NUMBER_T)input[x][z] * (LONG_NUMBER_T)kernel[z];

      // Scale for possible additional precision of bias
      tmp = scale(NUMBER_T, tmp, WEIGHTS_SCALE_FACTOR - TMP_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      // Scale bias to match accumulator
      tmp += scale(NUMBER_T, (LONG_NUMBER_T)bias[z], BIASES_SCALE_FACTOR - TMP_SCALE_FACTOR - INPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);

      // Activation function
#ifdef ACTIVATION_LINEAR
      // Linear (MEANS NONE)
      output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU) || defined(ACTIVATION_RELU6)
      // ReLU
      if (tmp < 0) {
        output[x][z] = 0;
      } else {
#if defined(ACTIVATION_RELU6)
        if (tmp > scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE)) {
          tmp = scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE);
        }
#endif
        output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      }
#else
#error "Unsupported activation function"
#endif
    }
  }
}

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef ACTIVATION_RELU6
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
/**
  ******************************************************************************
  * @file    weights/batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

const int16_t stem_1_bias[32] = {-1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1}
;
const int16_t stem_1_kernel[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
;
/**
  ******************************************************************************
  * @file    conv1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_1_0_CONV_0_H_
#define _STAGE_1_0_CONV_0_H_

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

typedef int16_t stage_1_0_conv_0_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void stage_1_0_conv_0(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const number_t kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS],  // IN

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

#endif//_STAGE_1_0_CONV_0_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_1_0_conv_0.h"
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

#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_1_0_conv_0(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const NUMBER_T kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS],  // IN

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

#error "CMSIS-NN requires the use of bias"

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


const int16_t  stage_1_0_conv_0_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{-2}
, {1}
, {0}
}
, {{0}
, {-2}
, {-1}
}
, {{1}
, {-1}
, {1}
}
, {{0}
, {0}
, {-1}
}
, {{-1}
, {0}
, {-2}
}
, {{-1}
, {0}
, {-1}
}
, {{-1}
, {0}
, {0}
}
, {{-1}
, {0}
, {-2}
}
, {{-2}
, {0}
, {-2}
}
, {{-1}
, {0}
, {-2}
}
, {{0}
, {1}
, {1}
}
, {{1}
, {-1}
, {0}
}
, {{-1}
, {0}
, {1}
}
, {{-2}
, {-2}
, {-1}
}
, {{1}
, {0}
, {1}
}
, {{0}
, {0}
, {-2}
}
, {{0}
, {-1}
, {0}
}
, {{-2}
, {-1}
, {0}
}
, {{-1}
, {0}
, {-1}
}
, {{0}
, {-1}
, {0}
}
, {{-1}
, {0}
, {0}
}
, {{0}
, {1}
, {-1}
}
, {{0}
, {0}
, {0}
}
, {{1}
, {-1}
, {-2}
}
, {{-1}
, {-2}
, {1}
}
, {{1}
, {-1}
, {-2}
}
, {{0}
, {1}
, {-1}
}
, {{0}
, {0}
, {-2}
}
, {{-2}
, {0}
, {0}
}
, {{0}
, {-1}
, {-1}
}
, {{-1}
, {0}
, {1}
}
, {{0}
, {1}
, {0}
}
}
;

#undef INPUT_CHANNELS
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_GROUPS
/**
  ******************************************************************************
  * @file    batchnorm1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    21 april 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_1_0_CONV_1_H_
#define _STAGE_1_0_CONV_1_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      32
#define INPUT_SAMPLES       47

typedef int16_t stage_1_0_conv_1_output_type[47][32];

#if 0
void stage_1_0_conv_1(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const number_t kernel[INPUT_CHANNELS],                // IN
  const number_t bias[INPUT_CHANNELS],                  // IN
  stage_1_0_conv_1_output_type output);                // OUT
#endif

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES

#endif//_STAGE_1_0_CONV_1_H_
/**
  ******************************************************************************
  * @file    batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_1_0_conv_1.h"
#include "number.h"
#endif

#define INPUT_CHANNELS      32
#define INPUT_SAMPLES       47
#define ACTIVATION_RELU6

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_1_0_conv_1(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const NUMBER_T kernel[INPUT_CHANNELS],                // IN
  const NUMBER_T bias[INPUT_CHANNELS],                  // IN
  stage_1_0_conv_1_output_type output) {                // OUT

  LONG_NUMBER_T tmp;

  for (int x = 0; x < INPUT_SAMPLES; x++) {
    for (int z = 0; z < INPUT_CHANNELS; z++) {
      tmp = (LONG_NUMBER_T)input[x][z] * (LONG_NUMBER_T)kernel[z];

      // Scale for possible additional precision of bias
      tmp = scale(NUMBER_T, tmp, WEIGHTS_SCALE_FACTOR - TMP_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      // Scale bias to match accumulator
      tmp += scale(NUMBER_T, (LONG_NUMBER_T)bias[z], BIASES_SCALE_FACTOR - TMP_SCALE_FACTOR - INPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);

      // Activation function
#ifdef ACTIVATION_LINEAR
      // Linear (MEANS NONE)
      output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU) || defined(ACTIVATION_RELU6)
      // ReLU
      if (tmp < 0) {
        output[x][z] = 0;
      } else {
#if defined(ACTIVATION_RELU6)
        if (tmp > scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE)) {
          tmp = scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE);
        }
#endif
        output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      }
#else
#error "Unsupported activation function"
#endif
    }
  }
}

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef ACTIVATION_RELU6
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
/**
  ******************************************************************************
  * @file    weights/batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

const int16_t stage_1_0_conv_1_bias[32] = {0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
;
const int16_t stage_1_0_conv_1_kernel[32] = {0, 0, 0, 0, 0, 31, 0, 1, 0, 0, 1, 0, 31, 0, 31, 17, 2, 31, 0, 1, 0, 0, 31, 31, 0, 0, 0, 31, 31, 0, 0, 0}
;
/**
  ******************************************************************************
  * @file    conv1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_1_0_CONV_3_H_
#define _STAGE_1_0_CONV_3_H_

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

typedef int16_t stage_1_0_conv_3_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void stage_1_0_conv_3(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const number_t kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS],  // IN

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

#endif//_STAGE_1_0_CONV_3_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_1_0_conv_3.h"
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

#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_1_0_conv_3(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const NUMBER_T kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS],  // IN

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

#error "CMSIS-NN requires the use of bias"

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


const int16_t  stage_1_0_conv_3_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0}
}
, {{0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1}
}
, {{-1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1}
}
, {{-1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0}
}
, {{0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1}
}
, {{0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1}
}
, {{-1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0}
}
, {{0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1}
}
, {{0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0}
}
, {{-1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0}
}
, {{-1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1}
}
, {{0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0}
}
, {{0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0}
}
, {{-1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1}
}
, {{0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0}
}
, {{-1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1}
}
, {{-1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0}
}
, {{-1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1}
}
, {{-1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1}
}
, {{0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1}
}
, {{0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0}
}
, {{-1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0}
}
, {{-1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0}
}
, {{-1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0}
}
, {{-1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0}
}
, {{0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1}
}
, {{-1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0}
}
, {{0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0}
}
, {{0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1}
}
, {{0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0}
}
, {{0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0}
}
, {{-1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0}
}
, {{0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1}
}
, {{0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1}
}
, {{-1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0}
}
, {{-1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0}
}
, {{-1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1}
}
, {{0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0}
}
, {{-1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0}
}
, {{0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0}
}
, {{0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1}
}
, {{-1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1}
}
, {{-1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0}
}
, {{0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0}
}
, {{0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1}
}
, {{-1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0}
}
, {{0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0}
}
, {{-1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1}
}
, {{0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0}
}
, {{0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1}
}
, {{-1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0}
}
, {{-1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0}
}
, {{-1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0}
}
, {{-1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0}
}
, {{0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0}
}
, {{0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1}
}
, {{-1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0}
}
, {{-1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0}
}
, {{-1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1}
}
, {{0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0}
}
, {{-1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0}
}
, {{0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1}
}
, {{0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0}
}
, {{-1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0}
}
}
;

#undef INPUT_CHANNELS
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_GROUPS
/**
  ******************************************************************************
  * @file    batchnorm1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    21 april 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_1_0_CONV_4_H_
#define _STAGE_1_0_CONV_4_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      64
#define INPUT_SAMPLES       47

typedef int16_t stage_1_0_conv_4_output_type[47][64];

#if 0
void stage_1_0_conv_4(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const number_t kernel[INPUT_CHANNELS],                // IN
  const number_t bias[INPUT_CHANNELS],                  // IN
  stage_1_0_conv_4_output_type output);                // OUT
#endif

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES

#endif//_STAGE_1_0_CONV_4_H_
/**
  ******************************************************************************
  * @file    batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_1_0_conv_4.h"
#include "number.h"
#endif

#define INPUT_CHANNELS      64
#define INPUT_SAMPLES       47
#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_1_0_conv_4(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const NUMBER_T kernel[INPUT_CHANNELS],                // IN
  const NUMBER_T bias[INPUT_CHANNELS],                  // IN
  stage_1_0_conv_4_output_type output) {                // OUT

  LONG_NUMBER_T tmp;

  for (int x = 0; x < INPUT_SAMPLES; x++) {
    for (int z = 0; z < INPUT_CHANNELS; z++) {
      tmp = (LONG_NUMBER_T)input[x][z] * (LONG_NUMBER_T)kernel[z];

      // Scale for possible additional precision of bias
      tmp = scale(NUMBER_T, tmp, WEIGHTS_SCALE_FACTOR - TMP_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      // Scale bias to match accumulator
      tmp += scale(NUMBER_T, (LONG_NUMBER_T)bias[z], BIASES_SCALE_FACTOR - TMP_SCALE_FACTOR - INPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);

      // Activation function
#ifdef ACTIVATION_LINEAR
      // Linear (MEANS NONE)
      output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU) || defined(ACTIVATION_RELU6)
      // ReLU
      if (tmp < 0) {
        output[x][z] = 0;
      } else {
#if defined(ACTIVATION_RELU6)
        if (tmp > scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE)) {
          tmp = scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE);
        }
#endif
        output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      }
#else
#error "Unsupported activation function"
#endif
    }
  }
}

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef ACTIVATION_LINEAR
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
/**
  ******************************************************************************
  * @file    weights/batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

const int16_t stage_1_0_conv_4_bias[64] = {-1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0}
;
const int16_t stage_1_0_conv_4_kernel[64] = {2, 2, 0, 0, 1, 0, 2, 3, 3, 1, 1, 0, 2, 0, 2, 2, 2, 2, 0, 0, 1, 0, 1, 0, 2, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0, 2, 0, 0, 2, 1, 2, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0}
;
/**
  ******************************************************************************
  * @file    conv1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_2_0_CONV_0_H_
#define _STAGE_2_0_CONV_0_H_

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

typedef int16_t stage_2_0_conv_0_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void stage_2_0_conv_0(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const number_t kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS],  // IN

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

#endif//_STAGE_2_0_CONV_0_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_2_0_conv_0.h"
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

#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_2_0_conv_0(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const NUMBER_T kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS],  // IN

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

#error "CMSIS-NN requires the use of bias"

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


const int16_t  stage_2_0_conv_0_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{0}
, {-2}
, {1}
}
, {{-1}
, {-1}
, {-1}
}
, {{1}
, {0}
, {0}
}
, {{0}
, {-1}
, {1}
}
, {{0}
, {-1}
, {0}
}
, {{-1}
, {1}
, {0}
}
, {{-1}
, {0}
, {0}
}
, {{-1}
, {-1}
, {-1}
}
, {{-2}
, {0}
, {1}
}
, {{-1}
, {-1}
, {-1}
}
, {{0}
, {1}
, {-1}
}
, {{-1}
, {-1}
, {-2}
}
, {{-1}
, {0}
, {1}
}
, {{-2}
, {0}
, {0}
}
, {{0}
, {0}
, {1}
}
, {{-1}
, {0}
, {-1}
}
, {{-1}
, {-1}
, {-1}
}
, {{0}
, {0}
, {0}
}
, {{-2}
, {1}
, {-1}
}
, {{1}
, {1}
, {1}
}
, {{-1}
, {-1}
, {-2}
}
, {{-1}
, {0}
, {-1}
}
, {{-1}
, {0}
, {0}
}
, {{-1}
, {-1}
, {0}
}
, {{0}
, {0}
, {-1}
}
, {{1}
, {0}
, {0}
}
, {{-2}
, {-1}
, {-1}
}
, {{-2}
, {-1}
, {0}
}
, {{-1}
, {-2}
, {1}
}
, {{-1}
, {1}
, {0}
}
, {{0}
, {-2}
, {-1}
}
, {{0}
, {0}
, {-1}
}
, {{0}
, {-1}
, {1}
}
, {{-1}
, {0}
, {-1}
}
, {{-1}
, {-1}
, {1}
}
, {{-2}
, {-2}
, {-1}
}
, {{0}
, {-2}
, {1}
}
, {{-1}
, {0}
, {0}
}
, {{0}
, {0}
, {0}
}
, {{-2}
, {-1}
, {-2}
}
, {{0}
, {-1}
, {-2}
}
, {{-1}
, {-1}
, {1}
}
, {{0}
, {0}
, {1}
}
, {{1}
, {-1}
, {1}
}
, {{-1}
, {0}
, {-1}
}
, {{-1}
, {-2}
, {-2}
}
, {{0}
, {-2}
, {-1}
}
, {{0}
, {-1}
, {0}
}
, {{0}
, {-2}
, {0}
}
, {{1}
, {-2}
, {-1}
}
, {{-2}
, {0}
, {0}
}
, {{-1}
, {0}
, {1}
}
, {{-1}
, {-1}
, {1}
}
, {{-1}
, {0}
, {0}
}
, {{0}
, {-1}
, {0}
}
, {{-1}
, {1}
, {0}
}
, {{1}
, {1}
, {-1}
}
, {{0}
, {-1}
, {-1}
}
, {{0}
, {-2}
, {-1}
}
, {{-1}
, {-1}
, {1}
}
, {{-1}
, {-1}
, {-1}
}
, {{0}
, {0}
, {-1}
}
, {{1}
, {0}
, {0}
}
, {{0}
, {-1}
, {-1}
}
}
;

#undef INPUT_CHANNELS
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_GROUPS
/**
  ******************************************************************************
  * @file    batchnorm1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    21 april 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_2_0_CONV_1_H_
#define _STAGE_2_0_CONV_1_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      64
#define INPUT_SAMPLES       24

typedef int16_t stage_2_0_conv_1_output_type[24][64];

#if 0
void stage_2_0_conv_1(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const number_t kernel[INPUT_CHANNELS],                // IN
  const number_t bias[INPUT_CHANNELS],                  // IN
  stage_2_0_conv_1_output_type output);                // OUT
#endif

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES

#endif//_STAGE_2_0_CONV_1_H_
/**
  ******************************************************************************
  * @file    batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_2_0_conv_1.h"
#include "number.h"
#endif

#define INPUT_CHANNELS      64
#define INPUT_SAMPLES       24
#define ACTIVATION_RELU6

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_2_0_conv_1(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const NUMBER_T kernel[INPUT_CHANNELS],                // IN
  const NUMBER_T bias[INPUT_CHANNELS],                  // IN
  stage_2_0_conv_1_output_type output) {                // OUT

  LONG_NUMBER_T tmp;

  for (int x = 0; x < INPUT_SAMPLES; x++) {
    for (int z = 0; z < INPUT_CHANNELS; z++) {
      tmp = (LONG_NUMBER_T)input[x][z] * (LONG_NUMBER_T)kernel[z];

      // Scale for possible additional precision of bias
      tmp = scale(NUMBER_T, tmp, WEIGHTS_SCALE_FACTOR - TMP_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      // Scale bias to match accumulator
      tmp += scale(NUMBER_T, (LONG_NUMBER_T)bias[z], BIASES_SCALE_FACTOR - TMP_SCALE_FACTOR - INPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);

      // Activation function
#ifdef ACTIVATION_LINEAR
      // Linear (MEANS NONE)
      output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU) || defined(ACTIVATION_RELU6)
      // ReLU
      if (tmp < 0) {
        output[x][z] = 0;
      } else {
#if defined(ACTIVATION_RELU6)
        if (tmp > scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE)) {
          tmp = scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE);
        }
#endif
        output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      }
#else
#error "Unsupported activation function"
#endif
    }
  }
}

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef ACTIVATION_RELU6
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
/**
  ******************************************************************************
  * @file    weights/batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

const int16_t stage_2_0_conv_1_bias[64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0}
;
const int16_t stage_2_0_conv_1_kernel[64] = {0, 0, 31, 31, 0, 0, 0, 1, 0, 0, 1, 31, 0, 31, 0, 0, 0, 0, 31, 31, 0, 0, 1, 0, 0, 31, 0, 0, 0, 0, 0, 0, 0, 31, 0, 0, 0, 31, 0, 0, 0, 0, 0, 31, 0, 0, 1, 31, 4, 0, 31, 31, 0, 1, 0, 0, 0, 4, 0, 31, 0, 31, 0, 0}
;
/**
  ******************************************************************************
  * @file    conv1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_2_0_CONV_3_H_
#define _STAGE_2_0_CONV_3_H_

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

typedef int16_t stage_2_0_conv_3_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void stage_2_0_conv_3(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const number_t kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS],  // IN

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

#endif//_STAGE_2_0_CONV_3_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_2_0_conv_3.h"
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

#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_2_0_conv_3(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const NUMBER_T kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS],  // IN

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

#error "CMSIS-NN requires the use of bias"

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


const int16_t  stage_2_0_conv_3_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1}
}
, {{-1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1}
}
, {{-1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0}
}
, {{0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1}
}
, {{0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1}
}
, {{0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1}
}
, {{-1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0}
}
, {{-1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1}
}
, {{-1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1}
}
, {{-1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0}
}
, {{-1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1}
}
, {{0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1}
}
, {{-1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0}
}
, {{-1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0}
}
, {{0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0}
}
, {{-1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1}
}
, {{0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1}
}
, {{-1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1}
}
, {{0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0}
}
, {{0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1}
}
, {{0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0}
}
, {{-1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0}
}
, {{-1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0}
}
, {{0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0}
}
, {{0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0}
}
, {{-1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1}
}
, {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1}
}
, {{0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0}
}
, {{-1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1}
}
, {{0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1}
}
, {{-1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1}
}
, {{-1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1}
}
, {{-1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1}
}
, {{-1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1}
}
, {{0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1}
}
, {{0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
}
, {{0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1}
}
, {{0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1}
}
, {{-1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1}
}
, {{-1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0}
}
, {{-1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0}
}
, {{-1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1}
}
, {{0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1}
}
, {{0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0}
}
, {{-1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0}
}
, {{-1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0}
}
, {{0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1}
}
, {{0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0}
}
, {{0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1}
}
, {{-1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0}
}
, {{-1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1}
}
, {{-1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0}
}
, {{-1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1}
}
, {{-1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1}
}
, {{0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0}
}
, {{0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0}
}
, {{-1, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0}
}
, {{0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1}
}
, {{0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0}
}
, {{0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0}
}
, {{0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0}
}
, {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1}
}
, {{-1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1}
}
, {{-1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0}
}
, {{0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1}
}
, {{0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0}
}
, {{-1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0}
}
, {{0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0}
}
, {{-1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0}
}
, {{-1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0}
}
, {{0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1}
}
, {{0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1}
}
, {{0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1}
}
, {{0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0}
}
, {{0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0}
}
, {{-1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0}
}
, {{-1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1}
}
, {{-1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1}
}
, {{-1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0}
}
, {{0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0}
}
, {{0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1}
}
, {{-1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0}
}
, {{0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1}
}
, {{-1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1}
}
, {{0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1}
}
, {{-1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0}
}
, {{0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0}
}
, {{0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1}
}
, {{-1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1}
}
, {{-1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0}
}
, {{-1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1}
}
, {{0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1}
}
, {{0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0}
}
, {{0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0}
}
, {{-1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1}
}
, {{-1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1}
}
, {{0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1}
}
, {{0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0}
}
, {{0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0}
}
, {{0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1}
}
, {{-1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1}
}
, {{0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0}
}
, {{-1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0}
}
, {{0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1}
}
, {{0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1}
}
, {{-1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0}
}
, {{0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1}
}
, {{-1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0}
}
, {{0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1}
}
, {{-1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1}
}
, {{0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0}
}
, {{0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0}
}
, {{-1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0}
}
, {{0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1}
}
, {{0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0}
}
, {{0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1}
}
, {{0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0}
}
, {{-1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1}
}
, {{0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0}
}
, {{0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1}
}
, {{0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0}
}
, {{-1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0}
}
, {{0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1}
}
, {{0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0}
}
, {{0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0}
}
, {{-1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1}
}
, {{0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0}
}
, {{-1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0}
}
}
;

#undef INPUT_CHANNELS
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_GROUPS
/**
  ******************************************************************************
  * @file    batchnorm1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    21 april 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_2_0_CONV_4_H_
#define _STAGE_2_0_CONV_4_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       24

typedef int16_t stage_2_0_conv_4_output_type[24][128];

#if 0
void stage_2_0_conv_4(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const number_t kernel[INPUT_CHANNELS],                // IN
  const number_t bias[INPUT_CHANNELS],                  // IN
  stage_2_0_conv_4_output_type output);                // OUT
#endif

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES

#endif//_STAGE_2_0_CONV_4_H_
/**
  ******************************************************************************
  * @file    batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_2_0_conv_4.h"
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       24
#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_2_0_conv_4(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const NUMBER_T kernel[INPUT_CHANNELS],                // IN
  const NUMBER_T bias[INPUT_CHANNELS],                  // IN
  stage_2_0_conv_4_output_type output) {                // OUT

  LONG_NUMBER_T tmp;

  for (int x = 0; x < INPUT_SAMPLES; x++) {
    for (int z = 0; z < INPUT_CHANNELS; z++) {
      tmp = (LONG_NUMBER_T)input[x][z] * (LONG_NUMBER_T)kernel[z];

      // Scale for possible additional precision of bias
      tmp = scale(NUMBER_T, tmp, WEIGHTS_SCALE_FACTOR - TMP_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      // Scale bias to match accumulator
      tmp += scale(NUMBER_T, (LONG_NUMBER_T)bias[z], BIASES_SCALE_FACTOR - TMP_SCALE_FACTOR - INPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);

      // Activation function
#ifdef ACTIVATION_LINEAR
      // Linear (MEANS NONE)
      output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU) || defined(ACTIVATION_RELU6)
      // ReLU
      if (tmp < 0) {
        output[x][z] = 0;
      } else {
#if defined(ACTIVATION_RELU6)
        if (tmp > scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE)) {
          tmp = scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE);
        }
#endif
        output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      }
#else
#error "Unsupported activation function"
#endif
    }
  }
}

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef ACTIVATION_LINEAR
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
/**
  ******************************************************************************
  * @file    weights/batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

const int16_t stage_2_0_conv_4_bias[128] = {-1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0}
;
const int16_t stage_2_0_conv_4_kernel[128] = {0, 1, 0, 0, 0, 2, 0, 2, 0, 1, 2, 1, 0, 0, 0, 2, 0, 2, 0, 0, 1, 2, 2, 0, 2, 0, 4, 0, 0, 0, 2, 1, 1, 0, -1, 1, 0, 0, 0, 0, 0, 1, 2, 2, 0, 0, 3, 1, 2, 1, 2, -1, 0, 2, 0, 0, 0, 2, 0, 0, 1, 3, 1, -1, 0, 0, 0, 1, 0, 0, 0, 2, 0, -1, -1, 3, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 1, -1, 0, 2, 2, 0, 0, 1, 0, 2, -1, 1, 0, 2, 0, 1, 0, 1, 0, 0, 1, 2, 0, 0, -1, -1, 1, 2, 1, 0, 2, 2, 0, 0, 2}
;
/**
  ******************************************************************************
  * @file    conv1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_2_1_CONV_0_H_
#define _STAGE_2_1_CONV_0_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       24
#define CONV_FILTERS        128
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         1

#define ZEROPADDING_LEFT    1
#define ZEROPADDING_RIGHT   1

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

typedef int16_t stage_2_1_conv_0_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void stage_2_1_conv_0(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const number_t kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS],  // IN

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

#endif//_STAGE_2_1_CONV_0_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_2_1_conv_0.h"
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
#define CONV_STRIDE         1
#define CONV_GROUPS         128
#define CHANNELS_PER_GROUP  (INPUT_CHANNELS / CONV_GROUPS)
#define FILTERS_PER_GROUP   (CONV_FILTERS / CONV_GROUPS)

#define ZEROPADDING_LEFT    1
#define ZEROPADDING_RIGHT   1

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_2_1_conv_0(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const NUMBER_T kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS],  // IN

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

#error "CMSIS-NN requires the use of bias"

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


const int16_t  stage_2_1_conv_0_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{-1}
, {-1}
, {1}
}
, {{1}
, {-1}
, {-2}
}
, {{-1}
, {-1}
, {-1}
}
, {{1}
, {1}
, {-2}
}
, {{0}
, {0}
, {-2}
}
, {{0}
, {1}
, {1}
}
, {{-2}
, {-2}
, {1}
}
, {{-1}
, {0}
, {-1}
}
, {{0}
, {0}
, {1}
}
, {{0}
, {0}
, {-2}
}
, {{1}
, {-1}
, {0}
}
, {{-1}
, {-2}
, {0}
}
, {{0}
, {1}
, {-2}
}
, {{1}
, {-1}
, {1}
}
, {{-1}
, {-1}
, {-1}
}
, {{0}
, {0}
, {-1}
}
, {{0}
, {0}
, {-1}
}
, {{0}
, {0}
, {-1}
}
, {{0}
, {-1}
, {-1}
}
, {{-2}
, {-1}
, {-1}
}
, {{-2}
, {-2}
, {0}
}
, {{0}
, {0}
, {0}
}
, {{-1}
, {-1}
, {1}
}
, {{-1}
, {1}
, {-1}
}
, {{0}
, {1}
, {0}
}
, {{-1}
, {-1}
, {-2}
}
, {{-2}
, {-2}
, {-2}
}
, {{-1}
, {1}
, {-1}
}
, {{-2}
, {1}
, {0}
}
, {{-1}
, {-1}
, {-1}
}
, {{-2}
, {-1}
, {0}
}
, {{-1}
, {1}
, {-1}
}
, {{-1}
, {-1}
, {0}
}
, {{-1}
, {0}
, {-2}
}
, {{1}
, {0}
, {0}
}
, {{0}
, {1}
, {1}
}
, {{-2}
, {-2}
, {1}
}
, {{1}
, {0}
, {1}
}
, {{0}
, {1}
, {1}
}
, {{0}
, {0}
, {0}
}
, {{0}
, {-2}
, {0}
}
, {{1}
, {0}
, {-2}
}
, {{-1}
, {-2}
, {-2}
}
, {{-1}
, {1}
, {-2}
}
, {{0}
, {-1}
, {-1}
}
, {{-1}
, {-1}
, {-2}
}
, {{-1}
, {0}
, {0}
}
, {{-1}
, {0}
, {-2}
}
, {{-2}
, {-1}
, {0}
}
, {{0}
, {-2}
, {0}
}
, {{-1}
, {-2}
, {-1}
}
, {{-1}
, {-1}
, {-1}
}
, {{-2}
, {1}
, {-1}
}
, {{-1}
, {-1}
, {-1}
}
, {{-1}
, {-1}
, {-1}
}
, {{1}
, {-1}
, {-1}
}
, {{1}
, {0}
, {1}
}
, {{1}
, {-1}
, {-2}
}
, {{0}
, {-2}
, {1}
}
, {{1}
, {0}
, {1}
}
, {{-1}
, {0}
, {0}
}
, {{-2}
, {0}
, {0}
}
, {{0}
, {0}
, {0}
}
, {{1}
, {-1}
, {-1}
}
, {{-1}
, {-1}
, {-1}
}
, {{1}
, {-2}
, {0}
}
, {{0}
, {-1}
, {0}
}
, {{1}
, {0}
, {0}
}
, {{1}
, {-2}
, {0}
}
, {{-1}
, {-1}
, {-2}
}
, {{1}
, {-1}
, {1}
}
, {{-1}
, {-1}
, {0}
}
, {{-1}
, {-2}
, {1}
}
, {{-1}
, {-1}
, {-1}
}
, {{-1}
, {0}
, {0}
}
, {{-1}
, {-1}
, {0}
}
, {{0}
, {-2}
, {1}
}
, {{-1}
, {0}
, {-1}
}
, {{-1}
, {0}
, {1}
}
, {{-1}
, {-1}
, {1}
}
, {{-2}
, {0}
, {-1}
}
, {{0}
, {-1}
, {0}
}
, {{0}
, {0}
, {-1}
}
, {{-1}
, {0}
, {-1}
}
, {{-2}
, {0}
, {1}
}
, {{-1}
, {1}
, {-1}
}
, {{-2}
, {0}
, {0}
}
, {{-1}
, {0}
, {0}
}
, {{-1}
, {0}
, {-1}
}
, {{-1}
, {0}
, {-1}
}
, {{1}
, {1}
, {0}
}
, {{0}
, {-1}
, {-1}
}
, {{-1}
, {1}
, {0}
}
, {{-1}
, {-1}
, {-1}
}
, {{0}
, {1}
, {-2}
}
, {{1}
, {-2}
, {0}
}
, {{0}
, {0}
, {1}
}
, {{0}
, {0}
, {-1}
}
, {{1}
, {-2}
, {-1}
}
, {{0}
, {1}
, {1}
}
, {{0}
, {1}
, {-1}
}
, {{0}
, {-1}
, {-2}
}
, {{0}
, {-1}
, {-2}
}
, {{-2}
, {1}
, {0}
}
, {{1}
, {0}
, {-1}
}
, {{-1}
, {0}
, {-1}
}
, {{-1}
, {-1}
, {-2}
}
, {{0}
, {1}
, {-1}
}
, {{-1}
, {-1}
, {-1}
}
, {{1}
, {0}
, {-1}
}
, {{0}
, {1}
, {-2}
}
, {{0}
, {0}
, {0}
}
, {{-1}
, {0}
, {-1}
}
, {{-2}
, {-1}
, {-1}
}
, {{1}
, {-1}
, {0}
}
, {{1}
, {1}
, {0}
}
, {{-1}
, {0}
, {0}
}
, {{-2}
, {-1}
, {0}
}
, {{-2}
, {0}
, {-2}
}
, {{0}
, {0}
, {1}
}
, {{0}
, {1}
, {0}
}
, {{0}
, {0}
, {-1}
}
, {{-2}
, {-1}
, {1}
}
, {{-1}
, {-2}
, {-1}
}
, {{-1}
, {-2}
, {-1}
}
, {{-1}
, {-1}
, {0}
}
, {{-2}
, {-1}
, {1}
}
, {{-1}
, {0}
, {-1}
}
}
;

#undef INPUT_CHANNELS
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_GROUPS
/**
  ******************************************************************************
  * @file    batchnorm1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    21 april 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_2_1_CONV_1_H_
#define _STAGE_2_1_CONV_1_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       24

typedef int16_t stage_2_1_conv_1_output_type[24][128];

#if 0
void stage_2_1_conv_1(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const number_t kernel[INPUT_CHANNELS],                // IN
  const number_t bias[INPUT_CHANNELS],                  // IN
  stage_2_1_conv_1_output_type output);                // OUT
#endif

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES

#endif//_STAGE_2_1_CONV_1_H_
/**
  ******************************************************************************
  * @file    batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_2_1_conv_1.h"
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       24
#define ACTIVATION_RELU6

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_2_1_conv_1(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const NUMBER_T kernel[INPUT_CHANNELS],                // IN
  const NUMBER_T bias[INPUT_CHANNELS],                  // IN
  stage_2_1_conv_1_output_type output) {                // OUT

  LONG_NUMBER_T tmp;

  for (int x = 0; x < INPUT_SAMPLES; x++) {
    for (int z = 0; z < INPUT_CHANNELS; z++) {
      tmp = (LONG_NUMBER_T)input[x][z] * (LONG_NUMBER_T)kernel[z];

      // Scale for possible additional precision of bias
      tmp = scale(NUMBER_T, tmp, WEIGHTS_SCALE_FACTOR - TMP_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      // Scale bias to match accumulator
      tmp += scale(NUMBER_T, (LONG_NUMBER_T)bias[z], BIASES_SCALE_FACTOR - TMP_SCALE_FACTOR - INPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);

      // Activation function
#ifdef ACTIVATION_LINEAR
      // Linear (MEANS NONE)
      output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU) || defined(ACTIVATION_RELU6)
      // ReLU
      if (tmp < 0) {
        output[x][z] = 0;
      } else {
#if defined(ACTIVATION_RELU6)
        if (tmp > scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE)) {
          tmp = scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE);
        }
#endif
        output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      }
#else
#error "Unsupported activation function"
#endif
    }
  }
}

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef ACTIVATION_RELU6
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
/**
  ******************************************************************************
  * @file    weights/batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

const int16_t stage_2_1_conv_1_bias[128] = {-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0}
;
const int16_t stage_2_1_conv_1_kernel[128] = {2, 0, 0, 0, 0, 0, 31, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 1, 31, 0, 0, 0, 0, 31, 0, 0, 0, 12, 0, 7, 0, 1, 0, 0, 0, 0, 31, 0, 0, 31, 0, 0, 0, 0, 31, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 0, 0, 0, 1, 6, 0, 0, 0, 0, 0, 0, 0, 0, 31, 10, 6, 0, 0, 0, 0, 0, 1, 0, 4, 0, 31, 2, 0, 1, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 31, 1, 0, 0, 7, 0, 31, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 1, 0, 0, 0, 31, 0, 0}
;
/**
  ******************************************************************************
  * @file    conv1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_2_1_CONV_3_H_
#define _STAGE_2_1_CONV_3_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       24
#define CONV_FILTERS        128
#define CONV_KERNEL_SIZE    1
#define CONV_STRIDE         1

#define ZEROPADDING_LEFT    0
#define ZEROPADDING_RIGHT   0

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

typedef int16_t stage_2_1_conv_3_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void stage_2_1_conv_3(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const number_t kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS],  // IN

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

#endif//_STAGE_2_1_CONV_3_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_2_1_conv_3.h"
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
#define CONV_KERNEL_SIZE    1
#define CONV_STRIDE         1
#define CONV_GROUPS         1
#define CHANNELS_PER_GROUP  (INPUT_CHANNELS / CONV_GROUPS)
#define FILTERS_PER_GROUP   (CONV_FILTERS / CONV_GROUPS)

#define ZEROPADDING_LEFT    0
#define ZEROPADDING_RIGHT   0

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_2_1_conv_3(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const NUMBER_T kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS],  // IN

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

#error "CMSIS-NN requires the use of bias"

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


const int16_t  stage_2_1_conv_3_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{-1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1}
}
, {{0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0}
}
, {{-1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1}
}
, {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1}
}
, {{-1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0}
}
, {{-1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0}
}
, {{0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1}
}
, {{0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0}
}
, {{-1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0}
}
, {{-1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0}
}
, {{-1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1}
}
, {{-1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1}
}
, {{-1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1}
}
, {{0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0}
}
, {{-1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1}
}
, {{0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1}
}
, {{-1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0}
}
, {{-1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0}
}
, {{0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0}
}
, {{0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1}
}
, {{-1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0}
}
, {{0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1}
}
, {{-1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0}
}
, {{-1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1}
}
, {{0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0}
}
, {{0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1}
}
, {{0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0}
}
, {{-1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0}
}
, {{-1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0}
}
, {{-1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0}
}
, {{0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0}
}
, {{-1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0}
}
, {{0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1}
}
, {{0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0}
}
, {{0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1}
}
, {{0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0}
}
, {{0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1}
}
, {{-1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1}
}
, {{-1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1}
}
, {{-1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1}
}
, {{0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0}
}
, {{0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0}
}
, {{0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1}
}
, {{-1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1}
}
, {{0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1}
}
, {{0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1}
}
, {{0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0}
}
, {{0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0}
}
, {{0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1}
}
, {{0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1}
}
, {{-1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1}
}
, {{-1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1}
}
, {{0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1}
}
, {{0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0}
}
, {{-1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0}
}
, {{-1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1}
}
, {{-1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0}
}
, {{-1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1}
}
, {{-1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1}
}
, {{0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0}
}
, {{0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1}
}
, {{-1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1}
}
, {{-1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0}
}
, {{-1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1}
}
, {{-1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0}
}
, {{0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1}
}
, {{0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0}
}
, {{0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0}
}
, {{-1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0}
}
, {{0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0}
}
, {{0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1}
}
, {{-1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0}
}
, {{0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1}
}
, {{0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0}
}
, {{0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0}
}
, {{0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1}
}
, {{-1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1}
}
, {{-1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1}
}
, {{0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0}
}
, {{-1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1}
}
, {{-1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0}
}
, {{0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1}
}
, {{0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0}
}
, {{-1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1}
}
, {{-1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1}
}
, {{0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1}
}
, {{0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0}
}
, {{-1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0}
}
, {{-1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0}
}
, {{-1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0}
}
, {{0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1}
}
, {{-1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0}
}
, {{-1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0}
}
, {{0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1}
}
, {{0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1}
}
, {{-1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1}
}
, {{0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0}
}
, {{0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1}
}
, {{-1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1}
}
, {{-1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0}
}
, {{0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1}
}
, {{-1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1}
}
, {{0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0}
}
, {{-1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1}
}
, {{0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0}
}
, {{-1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1}
}
, {{-1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0}
}
, {{0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0}
}
, {{-1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0}
}
, {{0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1}
}
, {{-1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0}
}
, {{0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0}
}
, {{0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1}
}
, {{0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0}
}
, {{0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1}
}
, {{0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0}
}
, {{0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1}
}
, {{0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1}
}
, {{-1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1}
}
, {{0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0}
}
, {{0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1}
}
, {{0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1}
}
, {{0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1}
}
, {{-1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1}
}
, {{-1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0}
}
, {{0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0}
}
, {{0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0}
}
, {{-1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1}
}
}
;

#undef INPUT_CHANNELS
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_GROUPS
/**
  ******************************************************************************
  * @file    batchnorm1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    21 april 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_2_1_CONV_4_H_
#define _STAGE_2_1_CONV_4_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       24

typedef int16_t stage_2_1_conv_4_output_type[24][128];

#if 0
void stage_2_1_conv_4(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const number_t kernel[INPUT_CHANNELS],                // IN
  const number_t bias[INPUT_CHANNELS],                  // IN
  stage_2_1_conv_4_output_type output);                // OUT
#endif

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES

#endif//_STAGE_2_1_CONV_4_H_
/**
  ******************************************************************************
  * @file    batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_2_1_conv_4.h"
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       24
#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_2_1_conv_4(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const NUMBER_T kernel[INPUT_CHANNELS],                // IN
  const NUMBER_T bias[INPUT_CHANNELS],                  // IN
  stage_2_1_conv_4_output_type output) {                // OUT

  LONG_NUMBER_T tmp;

  for (int x = 0; x < INPUT_SAMPLES; x++) {
    for (int z = 0; z < INPUT_CHANNELS; z++) {
      tmp = (LONG_NUMBER_T)input[x][z] * (LONG_NUMBER_T)kernel[z];

      // Scale for possible additional precision of bias
      tmp = scale(NUMBER_T, tmp, WEIGHTS_SCALE_FACTOR - TMP_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      // Scale bias to match accumulator
      tmp += scale(NUMBER_T, (LONG_NUMBER_T)bias[z], BIASES_SCALE_FACTOR - TMP_SCALE_FACTOR - INPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);

      // Activation function
#ifdef ACTIVATION_LINEAR
      // Linear (MEANS NONE)
      output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU) || defined(ACTIVATION_RELU6)
      // ReLU
      if (tmp < 0) {
        output[x][z] = 0;
      } else {
#if defined(ACTIVATION_RELU6)
        if (tmp > scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE)) {
          tmp = scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE);
        }
#endif
        output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      }
#else
#error "Unsupported activation function"
#endif
    }
  }
}

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef ACTIVATION_LINEAR
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
/**
  ******************************************************************************
  * @file    weights/batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

const int16_t stage_2_1_conv_4_bias[128] = {-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0}
;
const int16_t stage_2_1_conv_4_kernel[128] = {1, 1, 0, 0, 0, 0, 0, -1, 0, 2, -1, 0, 2, -1, 0, 2, 2, 1, 0, 2, 2, 0, 0, 2, 1, 1, -1, 2, 2, 1, 0, 0, 0, 0, 2, 1, 2, 0, 2, 0, 0, 0, 0, 0, 1, 1, 2, 1, 0, 1, -1, 1, -2, 0, 0, 2, 0, 1, 0, 0, 2, 0, -2, 1, 0, 2, 0, 2, 1, 2, -1, 0, 0, 2, 1, 2, -1, 2, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 3, 0, 0, 1, 0, 2, 2, 0, 0, 1, 0, 1, 2, 1, 0, 0, 0, -1, 0, 1, 0, 0, 1, 2, 0, 1, 0, 2, 1, 2, 0, 0, 2, 0, 2, 0, 1}
;
/**
  ******************************************************************************
  * @file    operator.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _ADD_H_
#define _ADD_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

typedef int16_t add_output_type[24][128];

#if 0
void add(

  const number_t vector_in_1[24][128], // doesn't work with inverted data_format

  const number_t vector_in_2[24][128], // doesn't work with inverted data_format

  add_output_type vector_out);     // OUT
#endif

#endif//_ADD_H_
/**
  ******************************************************************************
  * @file    operator.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "add.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#endif

#define ACTIVATION_LINEAR

// For fixed point quantization
#define ACC_SCALE_FACTOR 0 // Get maximum scale factor of previous layers
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t

static inline void add(

  const NUMBER_T vector_in_1[24][128], // doesn't work with inverted data_format

  const NUMBER_T vector_in_2[24][128], // doesn't work with inverted data_format

  add_output_type vector_out) {    // OUT

  size_t x;
  LONG_NUMBER_T output_acc;


  NUMBER_T *i_1 = (NUMBER_T*)vector_in_1;

  NUMBER_T *i_2 = (NUMBER_T*)vector_in_2;


  NUMBER_T *o = (NUMBER_T*)vector_out;

  for (x = 0; x < 24*128; x++) {
    // scale all fixed point inputs to same factor and add them, negative factor is left shift
    output_acc = 
                    + scale(NUMBER_T, (LONG_NUMBER_T)i_1[x], 0 - ACC_SCALE_FACTOR, OUTPUT_ROUND_MODE)
                 
                    + scale(NUMBER_T, (LONG_NUMBER_T)i_2[x], 0 - ACC_SCALE_FACTOR, OUTPUT_ROUND_MODE)
                 ;
#ifdef ACTIVATION_LINEAR
    o[x] = scale_and_clamp_to(NUMBER_T, output_acc, ACC_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU)
    if (output_acc < 0) {
      o[x] = 0;
    } else {
      o[x] = scale_and_clamp_to(NUMBER_T, output_acc, ACC_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
    }
#else
#error "Unsupported activation function"
#endif
  }
}

#undef ACTIVATION_LINEAR
#undef ACC_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
#undef NUMBER_T
#undef LONG_NUMBER_T
/**
  ******************************************************************************
  * @file    conv1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_3_0_CONV_0_H_
#define _STAGE_3_0_CONV_0_H_

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

typedef int16_t stage_3_0_conv_0_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void stage_3_0_conv_0(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const number_t kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS],  // IN

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

#endif//_STAGE_3_0_CONV_0_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_3_0_conv_0.h"
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

#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_3_0_conv_0(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const NUMBER_T kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS],  // IN

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

#error "CMSIS-NN requires the use of bias"

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


const int16_t  stage_3_0_conv_0_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{0}
, {-1}
, {-1}
}
, {{1}
, {0}
, {-2}
}
, {{-2}
, {0}
, {-2}
}
, {{-1}
, {-1}
, {1}
}
, {{-1}
, {0}
, {-1}
}
, {{-2}
, {-1}
, {0}
}
, {{0}
, {-1}
, {0}
}
, {{0}
, {0}
, {0}
}
, {{0}
, {0}
, {-1}
}
, {{-1}
, {-1}
, {1}
}
, {{-2}
, {-1}
, {-1}
}
, {{1}
, {0}
, {1}
}
, {{-1}
, {-2}
, {-1}
}
, {{0}
, {-1}
, {-2}
}
, {{-1}
, {-1}
, {-1}
}
, {{-1}
, {-2}
, {0}
}
, {{0}
, {1}
, {1}
}
, {{-2}
, {-2}
, {-1}
}
, {{-2}
, {0}
, {0}
}
, {{0}
, {-2}
, {0}
}
, {{0}
, {1}
, {-1}
}
, {{-1}
, {0}
, {-1}
}
, {{-1}
, {-2}
, {-2}
}
, {{-2}
, {-1}
, {0}
}
, {{-1}
, {0}
, {0}
}
, {{-2}
, {0}
, {1}
}
, {{-1}
, {1}
, {-2}
}
, {{-1}
, {0}
, {1}
}
, {{-2}
, {-1}
, {-2}
}
, {{-1}
, {-1}
, {-1}
}
, {{-2}
, {1}
, {-1}
}
, {{-2}
, {-1}
, {0}
}
, {{-2}
, {-1}
, {1}
}
, {{0}
, {-1}
, {-2}
}
, {{0}
, {1}
, {1}
}
, {{0}
, {-1}
, {0}
}
, {{-1}
, {0}
, {0}
}
, {{1}
, {-1}
, {0}
}
, {{-1}
, {-2}
, {-1}
}
, {{0}
, {-1}
, {1}
}
, {{1}
, {0}
, {0}
}
, {{0}
, {-1}
, {0}
}
, {{-1}
, {-2}
, {-1}
}
, {{-1}
, {-1}
, {-1}
}
, {{0}
, {-1}
, {1}
}
, {{-1}
, {0}
, {-1}
}
, {{-2}
, {-1}
, {0}
}
, {{-2}
, {-2}
, {-1}
}
, {{-1}
, {1}
, {1}
}
, {{-2}
, {-1}
, {0}
}
, {{-1}
, {-1}
, {-1}
}
, {{0}
, {-1}
, {0}
}
, {{-2}
, {1}
, {-2}
}
, {{0}
, {1}
, {0}
}
, {{0}
, {-1}
, {-1}
}
, {{0}
, {0}
, {0}
}
, {{-1}
, {0}
, {-1}
}
, {{-2}
, {-1}
, {0}
}
, {{1}
, {-2}
, {-1}
}
, {{0}
, {-1}
, {0}
}
, {{0}
, {0}
, {0}
}
, {{-1}
, {0}
, {-1}
}
, {{-1}
, {-1}
, {-2}
}
, {{-1}
, {-1}
, {0}
}
, {{-2}
, {-1}
, {0}
}
, {{1}
, {1}
, {0}
}
, {{-1}
, {-2}
, {1}
}
, {{1}
, {-1}
, {-1}
}
, {{-1}
, {-1}
, {-1}
}
, {{-1}
, {0}
, {-1}
}
, {{0}
, {-2}
, {0}
}
, {{-1}
, {-1}
, {-1}
}
, {{1}
, {1}
, {-1}
}
, {{1}
, {-1}
, {1}
}
, {{-2}
, {1}
, {-1}
}
, {{-1}
, {1}
, {0}
}
, {{-2}
, {1}
, {0}
}
, {{0}
, {-1}
, {0}
}
, {{-1}
, {-1}
, {-1}
}
, {{1}
, {-2}
, {0}
}
, {{0}
, {1}
, {-1}
}
, {{-1}
, {0}
, {-2}
}
, {{0}
, {-1}
, {-2}
}
, {{-1}
, {-2}
, {1}
}
, {{0}
, {1}
, {0}
}
, {{-1}
, {0}
, {-2}
}
, {{0}
, {0}
, {-1}
}
, {{-1}
, {-1}
, {0}
}
, {{0}
, {-1}
, {-2}
}
, {{-1}
, {0}
, {-1}
}
, {{1}
, {0}
, {-1}
}
, {{0}
, {0}
, {0}
}
, {{-1}
, {-2}
, {0}
}
, {{0}
, {0}
, {-1}
}
, {{-2}
, {-2}
, {1}
}
, {{-1}
, {0}
, {0}
}
, {{1}
, {0}
, {-1}
}
, {{-2}
, {-1}
, {-2}
}
, {{0}
, {1}
, {1}
}
, {{0}
, {1}
, {-1}
}
, {{-1}
, {0}
, {1}
}
, {{-2}
, {-1}
, {-1}
}
, {{1}
, {0}
, {0}
}
, {{-1}
, {0}
, {0}
}
, {{0}
, {-1}
, {1}
}
, {{-1}
, {0}
, {0}
}
, {{-2}
, {-1}
, {-1}
}
, {{0}
, {-2}
, {-1}
}
, {{-1}
, {-1}
, {-1}
}
, {{0}
, {-1}
, {0}
}
, {{0}
, {1}
, {-1}
}
, {{1}
, {0}
, {0}
}
, {{-1}
, {0}
, {0}
}
, {{-1}
, {-2}
, {-1}
}
, {{1}
, {0}
, {-1}
}
, {{1}
, {0}
, {-1}
}
, {{-1}
, {0}
, {1}
}
, {{0}
, {0}
, {0}
}
, {{-1}
, {0}
, {0}
}
, {{0}
, {0}
, {-1}
}
, {{0}
, {-2}
, {1}
}
, {{0}
, {-1}
, {0}
}
, {{-1}
, {0}
, {-1}
}
, {{0}
, {1}
, {-1}
}
, {{-1}
, {-1}
, {-2}
}
, {{0}
, {1}
, {-1}
}
, {{-2}
, {-1}
, {-1}
}
, {{1}
, {0}
, {-1}
}
}
;

#undef INPUT_CHANNELS
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_GROUPS
/**
  ******************************************************************************
  * @file    batchnorm1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    21 april 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_3_0_CONV_1_H_
#define _STAGE_3_0_CONV_1_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       12

typedef int16_t stage_3_0_conv_1_output_type[12][128];

#if 0
void stage_3_0_conv_1(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const number_t kernel[INPUT_CHANNELS],                // IN
  const number_t bias[INPUT_CHANNELS],                  // IN
  stage_3_0_conv_1_output_type output);                // OUT
#endif

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES

#endif//_STAGE_3_0_CONV_1_H_
/**
  ******************************************************************************
  * @file    batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_3_0_conv_1.h"
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       12
#define ACTIVATION_RELU6

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_3_0_conv_1(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const NUMBER_T kernel[INPUT_CHANNELS],                // IN
  const NUMBER_T bias[INPUT_CHANNELS],                  // IN
  stage_3_0_conv_1_output_type output) {                // OUT

  LONG_NUMBER_T tmp;

  for (int x = 0; x < INPUT_SAMPLES; x++) {
    for (int z = 0; z < INPUT_CHANNELS; z++) {
      tmp = (LONG_NUMBER_T)input[x][z] * (LONG_NUMBER_T)kernel[z];

      // Scale for possible additional precision of bias
      tmp = scale(NUMBER_T, tmp, WEIGHTS_SCALE_FACTOR - TMP_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      // Scale bias to match accumulator
      tmp += scale(NUMBER_T, (LONG_NUMBER_T)bias[z], BIASES_SCALE_FACTOR - TMP_SCALE_FACTOR - INPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);

      // Activation function
#ifdef ACTIVATION_LINEAR
      // Linear (MEANS NONE)
      output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU) || defined(ACTIVATION_RELU6)
      // ReLU
      if (tmp < 0) {
        output[x][z] = 0;
      } else {
#if defined(ACTIVATION_RELU6)
        if (tmp > scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE)) {
          tmp = scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE);
        }
#endif
        output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      }
#else
#error "Unsupported activation function"
#endif
    }
  }
}

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef ACTIVATION_RELU6
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
/**
  ******************************************************************************
  * @file    weights/batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

const int16_t stage_3_0_conv_1_bias[128] = {-1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0}
;
const int16_t stage_3_0_conv_1_kernel[128] = {1, 0, 31, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 1, 31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 0, 0, 0, 31, 0, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 31, 0, 31, 0, 0, 0, 0, 0, 0, 1, 0, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 0, 0, 0, 22, 0, 0, 0, 0, 0, 31, 0, 31, 0, 0, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 0, 0, 0, 31, 0}
;
/**
  ******************************************************************************
  * @file    conv1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_3_0_CONV_3_H_
#define _STAGE_3_0_CONV_3_H_

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

typedef int16_t stage_3_0_conv_3_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void stage_3_0_conv_3(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const number_t kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS],  // IN

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

#endif//_STAGE_3_0_CONV_3_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_3_0_conv_3.h"
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

#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_3_0_conv_3(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const NUMBER_T kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS],  // IN

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

#error "CMSIS-NN requires the use of bias"

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


const int16_t  stage_3_0_conv_3_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1}
}
, {{0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0}
}
, {{0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0}
}
, {{-1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1}
}
, {{-1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1}
}
, {{-1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1}
}
, {{0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0}
}
, {{-1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1}
}
, {{0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1}
}
, {{-1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1}
}
, {{-1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0}
}
, {{-1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1}
}
, {{0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1}
}
, {{0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1}
}
, {{0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0}
}
, {{-1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0}
}
, {{-1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1}
}
, {{0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0}
}
, {{-1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0}
}
, {{-1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1}
}
, {{0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1}
}
, {{-1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0}
}
, {{0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1}
}
, {{0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0}
}
, {{0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0}
}
, {{0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1}
}
, {{0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0}
}
, {{0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0}
}
, {{0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0}
}
, {{0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0}
}
, {{-1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1}
}
, {{-1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0}
}
, {{0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0}
}
, {{0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1}
}
, {{-1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1}
}
, {{0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0}
}
, {{0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0}
}
, {{-1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0}
}
, {{0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1}
}
, {{-1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0}
}
, {{-1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0}
}
, {{0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1}
}
, {{0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0}
}
, {{-1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1}
}
, {{-1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1}
}
, {{0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1}
}
, {{-1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0}
}
, {{0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1}
}
, {{0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0}
}
, {{-1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0}
}
, {{-1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1}
}
, {{0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1}
}
, {{-1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0}
}
, {{0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1}
}
, {{0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1}
}
, {{-1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1}
}
, {{0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1}
}
, {{0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0}
}
, {{0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0}
}
, {{-1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1}
}
, {{0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1}
}
, {{-1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1}
}
, {{-1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0}
}
, {{0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1}
}
, {{-1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1}
}
, {{0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0}
}
, {{-1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1}
}
, {{0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1}
}
, {{0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1}
}
, {{0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1}
}
, {{-1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1}
}
, {{-1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0}
}
, {{0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0}
}
, {{0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1}
}
, {{-1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1}
}
, {{-1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1}
}
, {{-1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1}
}
, {{0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0}
}
, {{-1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1}
}
, {{-1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1}
}
, {{0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1}
}
, {{0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0}
}
, {{-1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0}
}
, {{-1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1}
}
, {{0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0}
}
, {{0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1}
}
, {{0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1}
}
, {{0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0}
}
, {{0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1}
}
, {{-1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0}
}
, {{0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1}
}
, {{0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0}
}
, {{0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1}
}
, {{0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1}
}
, {{0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0}
}
, {{0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1}
}
, {{-1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0}
}
, {{-1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0}
}
, {{0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1}
}
, {{0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0}
}
, {{0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0}
}
, {{0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1}
}
, {{-1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0}
}
, {{0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1}
}
, {{0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1}
}
, {{0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1}
}
, {{0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1}
}
, {{0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1}
}
, {{0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1}
}
, {{0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0}
}
, {{0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1}
}
, {{0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0}
}
, {{-1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1}
}
, {{0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0}
}
, {{0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1}
}
, {{0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1}
}
, {{0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1}
}
, {{-1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1}
}
, {{-1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0}
}
, {{-1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1}
}
, {{0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1}
}
, {{-1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1}
}
, {{-1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0}
}
, {{-1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0}
}
, {{0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0}
}
, {{0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1}
}
, {{-1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1}
}
, {{-1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0}
}
}
;

#undef INPUT_CHANNELS
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_GROUPS
/**
  ******************************************************************************
  * @file    batchnorm1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    21 april 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_3_0_CONV_4_H_
#define _STAGE_3_0_CONV_4_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       12

typedef int16_t stage_3_0_conv_4_output_type[12][128];

#if 0
void stage_3_0_conv_4(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const number_t kernel[INPUT_CHANNELS],                // IN
  const number_t bias[INPUT_CHANNELS],                  // IN
  stage_3_0_conv_4_output_type output);                // OUT
#endif

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES

#endif//_STAGE_3_0_CONV_4_H_
/**
  ******************************************************************************
  * @file    batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_3_0_conv_4.h"
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       12
#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_3_0_conv_4(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const NUMBER_T kernel[INPUT_CHANNELS],                // IN
  const NUMBER_T bias[INPUT_CHANNELS],                  // IN
  stage_3_0_conv_4_output_type output) {                // OUT

  LONG_NUMBER_T tmp;

  for (int x = 0; x < INPUT_SAMPLES; x++) {
    for (int z = 0; z < INPUT_CHANNELS; z++) {
      tmp = (LONG_NUMBER_T)input[x][z] * (LONG_NUMBER_T)kernel[z];

      // Scale for possible additional precision of bias
      tmp = scale(NUMBER_T, tmp, WEIGHTS_SCALE_FACTOR - TMP_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      // Scale bias to match accumulator
      tmp += scale(NUMBER_T, (LONG_NUMBER_T)bias[z], BIASES_SCALE_FACTOR - TMP_SCALE_FACTOR - INPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);

      // Activation function
#ifdef ACTIVATION_LINEAR
      // Linear (MEANS NONE)
      output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU) || defined(ACTIVATION_RELU6)
      // ReLU
      if (tmp < 0) {
        output[x][z] = 0;
      } else {
#if defined(ACTIVATION_RELU6)
        if (tmp > scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE)) {
          tmp = scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE);
        }
#endif
        output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      }
#else
#error "Unsupported activation function"
#endif
    }
  }
}

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef ACTIVATION_LINEAR
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
/**
  ******************************************************************************
  * @file    weights/batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

const int16_t stage_3_0_conv_4_bias[128] = {-1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0}
;
const int16_t stage_3_0_conv_4_kernel[128] = {4, 2, 2, 2, 1, 2, -3, 2, 2, 3, 1, 1, 2, -1, 2, -1, -2, -2, -1, 1, 2, 2, -1, -3, -1, 1, -4, 3, 3, -2, -1, 2, 3, -3, 2, 1, 4, 1, 1, 3, 2, 2, 1, 0, 2, 1, 3, 0, -1, -2, -1, 2, 1, -2, 0, 3, 3, 2, 1, 1, 0, 2, -1, 0, 2, 2, 4, 3, 1, -2, 0, 2, 2, 1, 3, 2, 3, 2, 2, 2, 2, 1, 2, 4, 4, 1, 1, 3, -1, 0, 3, 3, 2, 1, -1, 3, 3, -1, 2, 3, 0, -1, 1, 2, 0, 0, -2, 2, -1, 2, 2, -1, 1, 2, 3, -1, 3, -2, 1, 2, 3, -1, 1, 2, 0, 2, 1, 3}
;
/**
  ******************************************************************************
  * @file    conv1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_3_1_CONV_0_H_
#define _STAGE_3_1_CONV_0_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       12
#define CONV_FILTERS        128
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         1

#define ZEROPADDING_LEFT    1
#define ZEROPADDING_RIGHT   1

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

typedef int16_t stage_3_1_conv_0_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void stage_3_1_conv_0(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const number_t kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS],  // IN

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

#endif//_STAGE_3_1_CONV_0_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_3_1_conv_0.h"
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
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         1
#define CONV_GROUPS         128
#define CHANNELS_PER_GROUP  (INPUT_CHANNELS / CONV_GROUPS)
#define FILTERS_PER_GROUP   (CONV_FILTERS / CONV_GROUPS)

#define ZEROPADDING_LEFT    1
#define ZEROPADDING_RIGHT   1

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_3_1_conv_0(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const NUMBER_T kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS],  // IN

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

#error "CMSIS-NN requires the use of bias"

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


const int16_t  stage_3_1_conv_0_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{-1}
, {-2}
, {1}
}
, {{-1}
, {-2}
, {0}
}
, {{0}
, {0}
, {-1}
}
, {{0}
, {0}
, {1}
}
, {{-2}
, {0}
, {0}
}
, {{-2}
, {0}
, {-1}
}
, {{-2}
, {0}
, {1}
}
, {{1}
, {-1}
, {-1}
}
, {{-1}
, {-2}
, {0}
}
, {{0}
, {0}
, {-1}
}
, {{0}
, {0}
, {-1}
}
, {{1}
, {0}
, {0}
}
, {{0}
, {-1}
, {-1}
}
, {{-2}
, {-1}
, {0}
}
, {{0}
, {1}
, {-1}
}
, {{-2}
, {-1}
, {-2}
}
, {{-1}
, {-1}
, {-1}
}
, {{0}
, {-1}
, {-1}
}
, {{-1}
, {0}
, {0}
}
, {{0}
, {-2}
, {-1}
}
, {{-2}
, {-1}
, {1}
}
, {{0}
, {-2}
, {-2}
}
, {{-2}
, {-2}
, {1}
}
, {{0}
, {-1}
, {0}
}
, {{-1}
, {0}
, {-1}
}
, {{-2}
, {-2}
, {-1}
}
, {{0}
, {-1}
, {-1}
}
, {{0}
, {-1}
, {-1}
}
, {{-1}
, {0}
, {-1}
}
, {{0}
, {-1}
, {0}
}
, {{1}
, {0}
, {-1}
}
, {{-1}
, {-2}
, {-1}
}
, {{-1}
, {0}
, {-2}
}
, {{0}
, {-1}
, {-1}
}
, {{0}
, {-2}
, {1}
}
, {{-1}
, {-1}
, {-1}
}
, {{-1}
, {-1}
, {-2}
}
, {{-1}
, {-1}
, {1}
}
, {{0}
, {-1}
, {0}
}
, {{-2}
, {-1}
, {1}
}
, {{-1}
, {-1}
, {0}
}
, {{-1}
, {0}
, {0}
}
, {{-1}
, {0}
, {0}
}
, {{0}
, {1}
, {-1}
}
, {{0}
, {-1}
, {-2}
}
, {{-2}
, {-1}
, {0}
}
, {{1}
, {0}
, {0}
}
, {{0}
, {-1}
, {0}
}
, {{0}
, {-2}
, {-2}
}
, {{0}
, {-1}
, {-1}
}
, {{-1}
, {0}
, {-1}
}
, {{0}
, {0}
, {0}
}
, {{-2}
, {-1}
, {-2}
}
, {{-2}
, {0}
, {0}
}
, {{-2}
, {-1}
, {0}
}
, {{0}
, {-1}
, {-1}
}
, {{0}
, {0}
, {1}
}
, {{0}
, {0}
, {1}
}
, {{-1}
, {-1}
, {-2}
}
, {{0}
, {-1}
, {-1}
}
, {{-1}
, {0}
, {0}
}
, {{0}
, {-2}
, {-1}
}
, {{-1}
, {0}
, {-1}
}
, {{0}
, {1}
, {1}
}
, {{-2}
, {0}
, {-1}
}
, {{1}
, {0}
, {0}
}
, {{0}
, {-2}
, {-1}
}
, {{0}
, {0}
, {1}
}
, {{0}
, {0}
, {-1}
}
, {{0}
, {0}
, {-1}
}
, {{-2}
, {-2}
, {-2}
}
, {{-1}
, {0}
, {0}
}
, {{-1}
, {-2}
, {-1}
}
, {{0}
, {0}
, {0}
}
, {{0}
, {0}
, {1}
}
, {{0}
, {-2}
, {-2}
}
, {{-2}
, {-1}
, {0}
}
, {{-1}
, {0}
, {-2}
}
, {{-1}
, {0}
, {-1}
}
, {{-1}
, {-1}
, {1}
}
, {{0}
, {1}
, {0}
}
, {{-1}
, {-2}
, {0}
}
, {{0}
, {0}
, {0}
}
, {{0}
, {-1}
, {0}
}
, {{-2}
, {-1}
, {-2}
}
, {{-2}
, {0}
, {0}
}
, {{0}
, {1}
, {-1}
}
, {{-1}
, {0}
, {0}
}
, {{1}
, {-1}
, {0}
}
, {{0}
, {0}
, {-1}
}
, {{-2}
, {-2}
, {-1}
}
, {{-1}
, {0}
, {-1}
}
, {{1}
, {-1}
, {-1}
}
, {{0}
, {1}
, {0}
}
, {{0}
, {0}
, {0}
}
, {{-2}
, {-1}
, {-2}
}
, {{-1}
, {-1}
, {-1}
}
, {{-2}
, {1}
, {0}
}
, {{0}
, {-1}
, {-2}
}
, {{-1}
, {-1}
, {-1}
}
, {{-1}
, {0}
, {1}
}
, {{0}
, {0}
, {0}
}
, {{-2}
, {1}
, {0}
}
, {{-1}
, {-1}
, {-1}
}
, {{1}
, {-1}
, {-1}
}
, {{1}
, {-1}
, {-1}
}
, {{0}
, {-1}
, {-1}
}
, {{1}
, {-2}
, {1}
}
, {{0}
, {-1}
, {-1}
}
, {{0}
, {0}
, {0}
}
, {{1}
, {-1}
, {-1}
}
, {{-2}
, {-2}
, {0}
}
, {{0}
, {-1}
, {0}
}
, {{1}
, {1}
, {0}
}
, {{1}
, {0}
, {-1}
}
, {{-1}
, {-1}
, {-1}
}
, {{0}
, {0}
, {0}
}
, {{-1}
, {0}
, {1}
}
, {{0}
, {0}
, {0}
}
, {{-1}
, {-1}
, {-1}
}
, {{0}
, {-1}
, {-1}
}
, {{1}
, {-1}
, {-1}
}
, {{-1}
, {1}
, {-1}
}
, {{-1}
, {0}
, {-2}
}
, {{-2}
, {-2}
, {-1}
}
, {{-1}
, {0}
, {0}
}
, {{0}
, {1}
, {1}
}
, {{0}
, {0}
, {0}
}
}
;

#undef INPUT_CHANNELS
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_GROUPS
/**
  ******************************************************************************
  * @file    batchnorm1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    21 april 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_3_1_CONV_1_H_
#define _STAGE_3_1_CONV_1_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       12

typedef int16_t stage_3_1_conv_1_output_type[12][128];

#if 0
void stage_3_1_conv_1(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const number_t kernel[INPUT_CHANNELS],                // IN
  const number_t bias[INPUT_CHANNELS],                  // IN
  stage_3_1_conv_1_output_type output);                // OUT
#endif

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES

#endif//_STAGE_3_1_CONV_1_H_
/**
  ******************************************************************************
  * @file    batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_3_1_conv_1.h"
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       12
#define ACTIVATION_RELU6

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_3_1_conv_1(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const NUMBER_T kernel[INPUT_CHANNELS],                // IN
  const NUMBER_T bias[INPUT_CHANNELS],                  // IN
  stage_3_1_conv_1_output_type output) {                // OUT

  LONG_NUMBER_T tmp;

  for (int x = 0; x < INPUT_SAMPLES; x++) {
    for (int z = 0; z < INPUT_CHANNELS; z++) {
      tmp = (LONG_NUMBER_T)input[x][z] * (LONG_NUMBER_T)kernel[z];

      // Scale for possible additional precision of bias
      tmp = scale(NUMBER_T, tmp, WEIGHTS_SCALE_FACTOR - TMP_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      // Scale bias to match accumulator
      tmp += scale(NUMBER_T, (LONG_NUMBER_T)bias[z], BIASES_SCALE_FACTOR - TMP_SCALE_FACTOR - INPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);

      // Activation function
#ifdef ACTIVATION_LINEAR
      // Linear (MEANS NONE)
      output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU) || defined(ACTIVATION_RELU6)
      // ReLU
      if (tmp < 0) {
        output[x][z] = 0;
      } else {
#if defined(ACTIVATION_RELU6)
        if (tmp > scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE)) {
          tmp = scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE);
        }
#endif
        output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      }
#else
#error "Unsupported activation function"
#endif
    }
  }
}

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef ACTIVATION_RELU6
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
/**
  ******************************************************************************
  * @file    weights/batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

const int16_t stage_3_1_conv_1_bias[128] = {0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
;
const int16_t stage_3_1_conv_1_kernel[128] = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 4, 3, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 2, 6, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 2, 0, 6, 4, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 3, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 4, 9, 0, 0, 8, 6, 0, 0, 8, 0, 0, 1, 0, 0, 0, 9, 0, 0, 1, 0, 0, 0, 0, 0, 3, 0, 0, 0}
;
/**
  ******************************************************************************
  * @file    conv1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_3_1_CONV_3_H_
#define _STAGE_3_1_CONV_3_H_

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

typedef int16_t stage_3_1_conv_3_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void stage_3_1_conv_3(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const number_t kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS],  // IN

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

#endif//_STAGE_3_1_CONV_3_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_3_1_conv_3.h"
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

#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_3_1_conv_3(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],                    // IN
  const NUMBER_T kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS],  // IN

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

#error "CMSIS-NN requires the use of bias"

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


const int16_t  stage_3_1_conv_3_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{-1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0}
}
, {{0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1}
}
, {{0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1}
}
, {{-1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1}
}
, {{-1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0}
}
, {{-1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0}
}
, {{0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0}
}
, {{-1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0}
}
, {{0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0}
}
, {{-1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0}
}
, {{0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1}
}
, {{0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1}
}
, {{-1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1}
}
, {{0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1}
}
, {{-1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1}
}
, {{0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0}
}
, {{0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0}
}
, {{-1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0}
}
, {{0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1}
}
, {{0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0}
}
, {{0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0}
}
, {{-1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0}
}
, {{0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1}
}
, {{-1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1}
}
, {{0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1}
}
, {{-1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1}
}
, {{-1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1}
}
, {{0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1}
}
, {{-1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0}
}
, {{0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0}
}
, {{0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0}
}
, {{0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1}
}
, {{0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0}
}
, {{-1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1}
}
, {{0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0}
}
, {{0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0}
}
, {{0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0}
}
, {{0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1}
}
, {{0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1}
}
, {{0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1}
}
, {{-1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0}
}
, {{-1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1}
}
, {{0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1}
}
, {{-1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1}
}
, {{-1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1}
}
, {{-1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1}
}
, {{0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0}
}
, {{0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1}
}
, {{-1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0}
}
, {{0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1}
}
, {{-1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0}
}
, {{0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1}
}
, {{0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1}
}
, {{0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0}
}
, {{-1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1}
}
, {{-1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0}
}
, {{0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1}
}
, {{-1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0}
}
, {{-1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0}
}
, {{-1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1}
}
, {{0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1}
}
, {{0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0}
}
, {{0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0}
}
, {{-1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0}
}
, {{-1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0}
}
, {{0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1}
}
, {{0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0}
}
, {{-1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0}
}
, {{-1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0}
}
, {{0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1}
}
, {{-1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1}
}
, {{0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1}
}
, {{0, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0}
}
, {{0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0}
}
, {{-1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1}
}
, {{-1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0}
}
, {{-1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1}
}
, {{-1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0}
}
, {{0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1}
}
, {{0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0}
}
, {{0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0}
}
, {{-1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0}
}
, {{0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0}
}
, {{0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0}
}
, {{-1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0}
}
, {{-1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1}
}
, {{0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1}
}
, {{0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0}
}
, {{0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1}
}
, {{0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1}
}
, {{-1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0}
}
, {{-1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1}
}
, {{-1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1}
}
, {{-1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0}
}
, {{-1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1}
}
, {{-1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1}
}
, {{0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0}
}
, {{0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1}
}
, {{-1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1}
}
, {{0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0}
}
, {{-1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1}
}
, {{-1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1}
}
, {{0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0}
}
, {{-1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1}
}
, {{-1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0}
}
, {{-1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1}
}
, {{0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1}
}
, {{-1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0}
}
, {{-1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0}
}
, {{-1, -1, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0}
}
, {{0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1}
}
, {{-1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1}
}
, {{-1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1}
}
, {{0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1}
}
, {{-1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1}
}
, {{0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0}
}
, {{0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0}
}
, {{-1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1}
}
, {{0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1}
}
, {{-1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0}
}
, {{-1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1}
}
, {{0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1}
}
, {{-1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0}
}
, {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0}
}
, {{0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0}
}
, {{0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1}
}
, {{0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1}
}
, {{-1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1}
}
}
;

#undef INPUT_CHANNELS
#undef CONV_FILTERS
#undef CONV_KERNEL_SIZE
#undef CONV_GROUPS
/**
  ******************************************************************************
  * @file    batchnorm1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    21 april 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _STAGE_3_1_CONV_4_H_
#define _STAGE_3_1_CONV_4_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       12

typedef int16_t stage_3_1_conv_4_output_type[12][128];

#if 0
void stage_3_1_conv_4(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const number_t kernel[INPUT_CHANNELS],                // IN
  const number_t bias[INPUT_CHANNELS],                  // IN
  stage_3_1_conv_4_output_type output);                // OUT
#endif

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES

#endif//_STAGE_3_1_CONV_4_H_
/**
  ******************************************************************************
  * @file    batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "stage_3_1_conv_4.h"
#include "number.h"
#endif

#define INPUT_CHANNELS      128
#define INPUT_SAMPLES       12
#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void stage_3_1_conv_4(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const NUMBER_T kernel[INPUT_CHANNELS],                // IN
  const NUMBER_T bias[INPUT_CHANNELS],                  // IN
  stage_3_1_conv_4_output_type output) {                // OUT

  LONG_NUMBER_T tmp;

  for (int x = 0; x < INPUT_SAMPLES; x++) {
    for (int z = 0; z < INPUT_CHANNELS; z++) {
      tmp = (LONG_NUMBER_T)input[x][z] * (LONG_NUMBER_T)kernel[z];

      // Scale for possible additional precision of bias
      tmp = scale(NUMBER_T, tmp, WEIGHTS_SCALE_FACTOR - TMP_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      // Scale bias to match accumulator
      tmp += scale(NUMBER_T, (LONG_NUMBER_T)bias[z], BIASES_SCALE_FACTOR - TMP_SCALE_FACTOR - INPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);

      // Activation function
#ifdef ACTIVATION_LINEAR
      // Linear (MEANS NONE)
      output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU) || defined(ACTIVATION_RELU6)
      // ReLU
      if (tmp < 0) {
        output[x][z] = 0;
      } else {
#if defined(ACTIVATION_RELU6)
        if (tmp > scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE)) {
          tmp = scale(NUMBER_T, 6, -(INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR), OUTPUT_ROUND_MODE);
        }
#endif
        output[x][z] = scale_and_clamp_to(NUMBER_T, tmp, INPUT_SCALE_FACTOR + TMP_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
      }
#else
#error "Unsupported activation function"
#endif
    }
  }
}

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES
#undef ACTIVATION_LINEAR
#undef WEIGHTS_SCALE_FACTOR
#undef BIASES_SCALE_FACTOR
#undef TMP_SCALE_FACTOR
#undef INPUT_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
/**
  ******************************************************************************
  * @file    weights/batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#include <stdint.h>

const int16_t stage_3_1_conv_4_bias[128] = {-1, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1}
;
const int16_t stage_3_1_conv_4_kernel[128] = {1, -2, 2, 3, -3, 2, -2, 3, -2, -2, 2, -3, 2, 3, -3, -2, 1, -2, -2, -3, 2, 2, 3, 2, 2, -2, 3, -3, -4, 2, 2, 2, 2, 2, -3, 2, 1, -2, 1, 2, 3, 2, -3, 3, 4, 3, 3, 3, 1, -3, 3, 3, -1, 2, 3, 3, 2, -3, 4, 2, 2, 3, -2, -2, 2, 3, -2, 5, 3, -2, 2, -3, 0, 1, 1, 1, 3, 3, 2, 2, 1, -3, 4, 4, 3, 3, -1, 2, 2, 2, 4, 3, -2, 3, 4, 3, -3, 2, 4, 1, 3, 3, 2, -2, 4, -3, 4, -1, 1, 1, -2, 3, 4, 1, 2, -3, 3, -2, 4, 3, 2, 3, -2, 2, 1, 2, 1, -2}
;
/**
  ******************************************************************************
  * @file    operator.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _ADD_1_H_
#define _ADD_1_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

typedef int16_t add_1_output_type[12][128];

#if 0
void add_1(

  const number_t vector_in_1[12][128], // doesn't work with inverted data_format

  const number_t vector_in_2[12][128], // doesn't work with inverted data_format

  add_1_output_type vector_out);     // OUT
#endif

#endif//_ADD_1_H_
/**
  ******************************************************************************
  * @file    operator.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "add_1.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#endif

#define ACTIVATION_LINEAR

// For fixed point quantization
#define ACC_SCALE_FACTOR 0 // Get maximum scale factor of previous layers
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t

static inline void add_1(

  const NUMBER_T vector_in_1[12][128], // doesn't work with inverted data_format

  const NUMBER_T vector_in_2[12][128], // doesn't work with inverted data_format

  add_1_output_type vector_out) {    // OUT

  size_t x;
  LONG_NUMBER_T output_acc;


  NUMBER_T *i_1 = (NUMBER_T*)vector_in_1;

  NUMBER_T *i_2 = (NUMBER_T*)vector_in_2;


  NUMBER_T *o = (NUMBER_T*)vector_out;

  for (x = 0; x < 12*128; x++) {
    // scale all fixed point inputs to same factor and add them, negative factor is left shift
    output_acc = 
                    + scale(NUMBER_T, (LONG_NUMBER_T)i_1[x], 0 - ACC_SCALE_FACTOR, OUTPUT_ROUND_MODE)
                 
                    + scale(NUMBER_T, (LONG_NUMBER_T)i_2[x], 0 - ACC_SCALE_FACTOR, OUTPUT_ROUND_MODE)
                 ;
#ifdef ACTIVATION_LINEAR
    o[x] = scale_and_clamp_to(NUMBER_T, output_acc, ACC_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
#elif defined(ACTIVATION_RELU)
    if (output_acc < 0) {
      o[x] = 0;
    } else {
      o[x] = scale_and_clamp_to(NUMBER_T, output_acc, ACC_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
    }
#else
#error "Unsupported activation function"
#endif
  }
}

#undef ACTIVATION_LINEAR
#undef ACC_SCALE_FACTOR
#undef OUTPUT_SCALE_FACTOR
#undef NUMBER_T
#undef LONG_NUMBER_T
/**
  ******************************************************************************
  * @file    averagepool1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    21 april 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _POOLING_H_
#define _POOLING_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS  128
#define INPUT_SAMPLES   12
#define POOL_SIZE       12
#define POOL_STRIDE     12
#define POOL_PAD        0 // Unsupported
#define POOL_LENGTH	    ( ( (INPUT_SAMPLES - POOL_SIZE + (2*POOL_PAD) ) / POOL_STRIDE ) + 1 )

typedef int16_t pooling_output_type[POOL_LENGTH][INPUT_CHANNELS];

#if 0
void pooling(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS], 	    // IN
  number_t output[POOL_LENGTH][INPUT_CHANNELS]); 	// OUT
#endif

#undef INPUT_CHANNELS  
#undef INPUT_SAMPLES
#undef POOL_SIZE
#undef POOL_STRIDE
#undef POOL_PAD
#undef POOL_LENGTH

#endif//_POOLING_H_
/**
  ******************************************************************************
  * @file    averagepool.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "pooling.h"
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
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void pooling(
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

#ifndef _FLATTEN_H_
#define _FLATTEN_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define OUTPUT_DIM 128

typedef int16_t flatten_output_type[OUTPUT_DIM];

#if 0
void flatten(
  const number_t input[1][128], 			      // IN
	number_t output[OUTPUT_DIM]); 			                // OUT
#endif

#undef OUTPUT_DIM

#endif//_FLATTEN_H_
/**
  ******************************************************************************
  * @file    flatten.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 2.0.0
  * @date    26 november 2021
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "flatten.h"
#include "number.h"
#endif

#define OUTPUT_DIM 128

#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t

static inline void flatten(
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

#ifndef _CLASSIFIER_H_
#define _CLASSIFIER_H_

#ifndef SINGLE_FILE
#include "number.h"
#include <stdint.h>
#endif

#define INPUT_SAMPLES 128
#define FC_UNITS 11

typedef int16_t classifier_output_type[FC_UNITS];

#if 0
void classifier(
  const number_t input[INPUT_SAMPLES], 			      // IN
	const number_t kernel[FC_UNITS][INPUT_SAMPLES],  // IN

	const number_t bias[FC_UNITS],			              // IN

	number_t output[FC_UNITS]); 			                // OUT
#endif

#undef INPUT_SAMPLES
#undef FC_UNITS

#endif//_CLASSIFIER_H_
/**
  ******************************************************************************
  * @file    fc.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "classifier.h"
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
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define NUMBER_T int16_t
#define LONG_NUMBER_T int32_t


static inline void classifier(
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


const int16_t classifier_bias[FC_UNITS] = {-1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0}
;

const int16_t classifier_kernel[FC_UNITS][INPUT_SAMPLES] = {{0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0}
, {-1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, -1, 0}
, {0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1}
, {0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1}
, {-1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -1}
, {0, 0, 0, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1}
, {0, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, 0, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1}
, {-1, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, 0, 0}
, {-1, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, 0, -1, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, -1}
, {-1, -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, -1, -1, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0}
, {-1, -1, -1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, -1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, 0}
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
#include "stem_0.h" // InputLayer is excluded
#include "stem_1.h" // InputLayer is excluded
#include "stage_1_0_conv_0.h" // InputLayer is excluded
#include "stage_1_0_conv_1.h" // InputLayer is excluded
#include "stage_1_0_conv_3.h" // InputLayer is excluded
#include "stage_1_0_conv_4.h" // InputLayer is excluded
#include "stage_2_0_conv_0.h" // InputLayer is excluded
#include "stage_2_0_conv_1.h" // InputLayer is excluded
#include "stage_2_0_conv_3.h" // InputLayer is excluded
#include "stage_2_0_conv_4.h" // InputLayer is excluded
#include "stage_2_1_conv_0.h" // InputLayer is excluded
#include "stage_2_1_conv_1.h" // InputLayer is excluded
#include "stage_2_1_conv_3.h" // InputLayer is excluded
#include "stage_2_1_conv_4.h" // InputLayer is excluded
#include "add.h" // InputLayer is excluded
#include "stage_3_0_conv_0.h" // InputLayer is excluded
#include "stage_3_0_conv_1.h" // InputLayer is excluded
#include "stage_3_0_conv_3.h" // InputLayer is excluded
#include "stage_3_0_conv_4.h" // InputLayer is excluded
#include "stage_3_1_conv_0.h" // InputLayer is excluded
#include "stage_3_1_conv_1.h" // InputLayer is excluded
#include "stage_3_1_conv_3.h" // InputLayer is excluded
#include "stage_3_1_conv_4.h" // InputLayer is excluded
#include "add_1.h" // InputLayer is excluded
#include "pooling.h" // InputLayer is excluded
#include "flatten.h" // InputLayer is excluded
#include "classifier.h"
#endif


#define MODEL_INPUT_DIM_0 186
#define MODEL_INPUT_DIM_1 9
#define MODEL_INPUT_DIMS 186 * 9

#define MODEL_OUTPUT_SAMPLES 11

#define MODEL_INPUT_SCALE_FACTOR 0 // scale factor of InputLayer
#define MODEL_INPUT_ROUND_MODE ROUND_MODE_FLOOR
#define MODEL_INPUT_NUMBER_T int16_t
#define MODEL_INPUT_LONG_NUMBER_T int32_t

#define MODEL_OUTPUT_SCALE_FACTOR 0 // scale factor of last layer
#define MODEL_OUTPUT_ROUND_MODE ROUND_MODE_FLOOR
#define MODEL_OUTPUT_NUMBER_T int16_t
#define MODEL_OUTPUT_LONG_NUMBER_T int32_t

// node 0 is InputLayer so use its output shape as input shape of the model
// typedef  input_t[186][9];
typedef int16_t input_t[186][9];
typedef classifier_output_type output_t;


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
#include "stem_0.c"
#include "weights/stem_0.c" // InputLayer is excluded
#include "stem_1.c"
#include "weights/stem_1.c" // InputLayer is excluded
#include "stage_1_0_conv_0.c"
#include "weights/stage_1_0_conv_0.c" // InputLayer is excluded
#include "stage_1_0_conv_1.c"
#include "weights/stage_1_0_conv_1.c" // InputLayer is excluded
#include "stage_1_0_conv_3.c"
#include "weights/stage_1_0_conv_3.c" // InputLayer is excluded
#include "stage_1_0_conv_4.c"
#include "weights/stage_1_0_conv_4.c" // InputLayer is excluded
#include "stage_2_0_conv_0.c"
#include "weights/stage_2_0_conv_0.c" // InputLayer is excluded
#include "stage_2_0_conv_1.c"
#include "weights/stage_2_0_conv_1.c" // InputLayer is excluded
#include "stage_2_0_conv_3.c"
#include "weights/stage_2_0_conv_3.c" // InputLayer is excluded
#include "stage_2_0_conv_4.c"
#include "weights/stage_2_0_conv_4.c" // InputLayer is excluded
#include "stage_2_1_conv_0.c"
#include "weights/stage_2_1_conv_0.c" // InputLayer is excluded
#include "stage_2_1_conv_1.c"
#include "weights/stage_2_1_conv_1.c" // InputLayer is excluded
#include "stage_2_1_conv_3.c"
#include "weights/stage_2_1_conv_3.c" // InputLayer is excluded
#include "stage_2_1_conv_4.c"
#include "weights/stage_2_1_conv_4.c" // InputLayer is excluded
#include "add.c" // InputLayer is excluded
#include "stage_3_0_conv_0.c"
#include "weights/stage_3_0_conv_0.c" // InputLayer is excluded
#include "stage_3_0_conv_1.c"
#include "weights/stage_3_0_conv_1.c" // InputLayer is excluded
#include "stage_3_0_conv_3.c"
#include "weights/stage_3_0_conv_3.c" // InputLayer is excluded
#include "stage_3_0_conv_4.c"
#include "weights/stage_3_0_conv_4.c" // InputLayer is excluded
#include "stage_3_1_conv_0.c"
#include "weights/stage_3_1_conv_0.c" // InputLayer is excluded
#include "stage_3_1_conv_1.c"
#include "weights/stage_3_1_conv_1.c" // InputLayer is excluded
#include "stage_3_1_conv_3.c"
#include "weights/stage_3_1_conv_3.c" // InputLayer is excluded
#include "stage_3_1_conv_4.c"
#include "weights/stage_3_1_conv_4.c" // InputLayer is excluded
#include "add_1.c" // InputLayer is excluded
#include "pooling.c" // InputLayer is excluded
#include "flatten.c" // InputLayer is excluded
#include "classifier.c"
#include "weights/classifier.c"
#endif


void cnn(
  const input_t input,
  classifier_output_type classifier_output) {
  
  // Output array allocation
  static union {
    stem_0_output_type stem_0_output;
    stage_1_0_conv_0_output_type stage_1_0_conv_0_output;
    stage_1_0_conv_3_output_type stage_1_0_conv_3_output;
    stage_2_0_conv_0_output_type stage_2_0_conv_0_output;
    stage_2_0_conv_3_output_type stage_2_0_conv_3_output;
    stage_2_1_conv_0_output_type stage_2_1_conv_0_output;
    stage_2_1_conv_3_output_type stage_2_1_conv_3_output;
    add_output_type add_output;
    stage_3_0_conv_1_output_type stage_3_0_conv_1_output;
    stage_3_0_conv_4_output_type stage_3_0_conv_4_output;
    pooling_output_type pooling_output;
    flatten_output_type flatten_output;
  } activations1;

  static union {
    stem_1_output_type stem_1_output;
    stage_1_0_conv_1_output_type stage_1_0_conv_1_output;
    stage_1_0_conv_4_output_type stage_1_0_conv_4_output;
    stage_2_0_conv_1_output_type stage_2_0_conv_1_output;
    stage_2_0_conv_4_output_type stage_2_0_conv_4_output;
    stage_3_0_conv_0_output_type stage_3_0_conv_0_output;
    stage_3_0_conv_3_output_type stage_3_0_conv_3_output;
    stage_3_1_conv_0_output_type stage_3_1_conv_0_output;
    stage_3_1_conv_3_output_type stage_3_1_conv_3_output;
    add_1_output_type add_1_output;
  } activations2;

  static union {
    stage_2_1_conv_1_output_type stage_2_1_conv_1_output;
    stage_2_1_conv_4_output_type stage_2_1_conv_4_output;
    stage_3_1_conv_1_output_type stage_3_1_conv_1_output;
    stage_3_1_conv_4_output_type stage_3_1_conv_4_output;
  } activations3;


// Model layers call chain 
  
  
  stem_0( // Model input is passed as model parameter
    input,
    stem_0_kernel,
    activations1.stem_0_output
    );
  
  
  stem_1(
    activations1.stem_0_output,
    stem_1_kernel,
    stem_1_bias,
    activations2.stem_1_output
    );
  
  
  stage_1_0_conv_0(
    activations2.stem_1_output,
    stage_1_0_conv_0_kernel,
    activations1.stage_1_0_conv_0_output
    );
  
  
  stage_1_0_conv_1(
    activations1.stage_1_0_conv_0_output,
    stage_1_0_conv_1_kernel,
    stage_1_0_conv_1_bias,
    activations2.stage_1_0_conv_1_output
    );
  
  
  stage_1_0_conv_3(
    activations2.stage_1_0_conv_1_output,
    stage_1_0_conv_3_kernel,
    activations1.stage_1_0_conv_3_output
    );
  
  
  stage_1_0_conv_4(
    activations1.stage_1_0_conv_3_output,
    stage_1_0_conv_4_kernel,
    stage_1_0_conv_4_bias,
    activations2.stage_1_0_conv_4_output
    );
  
  
  stage_2_0_conv_0(
    activations2.stage_1_0_conv_4_output,
    stage_2_0_conv_0_kernel,
    activations1.stage_2_0_conv_0_output
    );
  
  
  stage_2_0_conv_1(
    activations1.stage_2_0_conv_0_output,
    stage_2_0_conv_1_kernel,
    stage_2_0_conv_1_bias,
    activations2.stage_2_0_conv_1_output
    );
  
  
  stage_2_0_conv_3(
    activations2.stage_2_0_conv_1_output,
    stage_2_0_conv_3_kernel,
    activations1.stage_2_0_conv_3_output
    );
  
  
  stage_2_0_conv_4(
    activations1.stage_2_0_conv_3_output,
    stage_2_0_conv_4_kernel,
    stage_2_0_conv_4_bias,
    activations2.stage_2_0_conv_4_output
    );
  
  
  stage_2_1_conv_0(
    activations2.stage_2_0_conv_4_output,
    stage_2_1_conv_0_kernel,
    activations1.stage_2_1_conv_0_output
    );
  
  
  stage_2_1_conv_1(
    activations1.stage_2_1_conv_0_output,
    stage_2_1_conv_1_kernel,
    stage_2_1_conv_1_bias,
    activations3.stage_2_1_conv_1_output
    );
  
  
  stage_2_1_conv_3(
    activations3.stage_2_1_conv_1_output,
    stage_2_1_conv_3_kernel,
    activations1.stage_2_1_conv_3_output
    );
  
  
  stage_2_1_conv_4(
    activations1.stage_2_1_conv_3_output,
    stage_2_1_conv_4_kernel,
    stage_2_1_conv_4_bias,
    activations3.stage_2_1_conv_4_output
    );
  
  
  add(
    activations2.stage_2_0_conv_4_output,
    activations3.stage_2_1_conv_4_output,
    activations1.add_output
    );
  
  
  stage_3_0_conv_0(
    activations1.add_output,
    stage_3_0_conv_0_kernel,
    activations2.stage_3_0_conv_0_output
    );
  
  
  stage_3_0_conv_1(
    activations2.stage_3_0_conv_0_output,
    stage_3_0_conv_1_kernel,
    stage_3_0_conv_1_bias,
    activations1.stage_3_0_conv_1_output
    );
  
  
  stage_3_0_conv_3(
    activations1.stage_3_0_conv_1_output,
    stage_3_0_conv_3_kernel,
    activations2.stage_3_0_conv_3_output
    );
  
  
  stage_3_0_conv_4(
    activations2.stage_3_0_conv_3_output,
    stage_3_0_conv_4_kernel,
    stage_3_0_conv_4_bias,
    activations1.stage_3_0_conv_4_output
    );
  
  
  stage_3_1_conv_0(
    activations1.stage_3_0_conv_4_output,
    stage_3_1_conv_0_kernel,
    activations2.stage_3_1_conv_0_output
    );
  
  
  stage_3_1_conv_1(
    activations2.stage_3_1_conv_0_output,
    stage_3_1_conv_1_kernel,
    stage_3_1_conv_1_bias,
    activations3.stage_3_1_conv_1_output
    );
  
  
  stage_3_1_conv_3(
    activations3.stage_3_1_conv_1_output,
    stage_3_1_conv_3_kernel,
    activations2.stage_3_1_conv_3_output
    );
  
  
  stage_3_1_conv_4(
    activations2.stage_3_1_conv_3_output,
    stage_3_1_conv_4_kernel,
    stage_3_1_conv_4_bias,
    activations3.stage_3_1_conv_4_output
    );
  
  
  add_1(
    activations1.stage_3_0_conv_4_output,
    activations3.stage_3_1_conv_4_output,
    activations2.add_1_output
    );
  
  
  pooling(
    activations2.add_1_output,
    activations1.pooling_output
    );
  
  
  flatten(
    activations1.pooling_output,
    activations1.flatten_output
    );
  
  
  classifier(
    activations1.flatten_output,
    classifier_kernel,
    classifier_bias,// Last layer uses output passed as model parameter
    classifier_output
    );
}

#ifdef __cplusplus
} // extern "C"
#endif
