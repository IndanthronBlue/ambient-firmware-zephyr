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



#error "Unrecognized round mode, only floor and nearest are supported by CMSIS-NN"

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

#define NUMBER_MIN_FLOAT -2147483648
#define NUMBER_MAX_FLOAT 2147483647

static inline float min_float(
    float a,
    float b) {
	if (a <= b)
		return a;
	return b;
}

static inline float max_float(
    float a,
    float b) {
	if (a >= b)
		return a;
	return b;
}

static inline float scale_number_t_float(
  float number, int scale_factor, round_mode_t round_mode) {
	return number;
}
static inline float clamp_to_number_t_float(
  float number) {
	return (float) number;
}
static inline float scale_and_clamp_to_number_t_float(
  float number, int scale_factor, round_mode_t round_mode) {
	return (float) number;
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
  * @file    averagepool1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    21 april 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _AVERAGE_POOLING1D_1_H_
#define _AVERAGE_POOLING1D_1_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS  9
#define INPUT_SAMPLES   186
#define POOL_SIZE       4
#define POOL_STRIDE     4
#define POOL_PAD        0 // Unsupported
#define POOL_LENGTH	    ( ( (INPUT_SAMPLES - POOL_SIZE + (2*POOL_PAD) ) / POOL_STRIDE ) + 1 )

typedef float average_pooling1d_1_output_type[POOL_LENGTH][INPUT_CHANNELS];

#if 0
void average_pooling1d_1(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS], 	    // IN
  number_t output[POOL_LENGTH][INPUT_CHANNELS]); 	// OUT
#endif

#undef INPUT_CHANNELS  
#undef INPUT_SAMPLES
#undef POOL_SIZE
#undef POOL_STRIDE
#undef POOL_PAD
#undef POOL_LENGTH

#endif//_AVERAGE_POOLING1D_1_H_
/**
  ******************************************************************************
  * @file    averagepool.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "average_pooling1d_1.h"
#include "number.h"
#endif

#define INPUT_CHANNELS  9
#define INPUT_SAMPLES   186
#define POOL_SIZE       4
#define POOL_STRIDE     4
#define POOL_PAD        0 // Unsupported
#define POOL_LENGTH	    ( ( (INPUT_SAMPLES - POOL_SIZE + (2*POOL_PAD) ) / POOL_STRIDE ) + 1 )

#define ACTIVATION_LINEAR

// For fixed point quantization
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_NONE
#define NUMBER_T float
#define LONG_NUMBER_T float


static inline void average_pooling1d_1(
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
  * @file    conv1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _CONV1D_1_H_
#define _CONV1D_1_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      9
#define INPUT_SAMPLES       46
#define CONV_FILTERS        8
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         1

#define ZEROPADDING_LEFT    0
#define ZEROPADDING_RIGHT   0

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

typedef float conv1d_1_output_type[CONV_OUTSAMPLES][CONV_FILTERS];

#if 0
void conv1d_1(
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

#endif//_CONV1D_1_H_
/**
  ******************************************************************************
  * @file    conv.cc
  * @author  Sébastien Bilavarn, LEAT, CNRS, Université Côte d'Azur, France
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "conv1d_1.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#elif defined(WITH_NMSIS_NN)
#include "riscv_nnfunctions.h"
#endif

#define INPUT_CHANNELS      9
#define INPUT_SAMPLES       46
#define CONV_FILTERS        8
#define CONV_KERNEL_SIZE    3
#define CONV_STRIDE         1
#define CONV_GROUPS         1
#define CHANNELS_PER_GROUP  (INPUT_CHANNELS / CONV_GROUPS)
#define FILTERS_PER_GROUP   (CONV_FILTERS / CONV_GROUPS)

#define ZEROPADDING_LEFT    0
#define ZEROPADDING_RIGHT   0

#define CONV_OUTSAMPLES     ( ( (INPUT_SAMPLES - CONV_KERNEL_SIZE + ZEROPADDING_LEFT + ZEROPADDING_RIGHT) / CONV_STRIDE ) + 1 )

#define ACTIVATION_RELU

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_NONE
#define NUMBER_T float
#define LONG_NUMBER_T float


static inline void conv1d_1(
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

#error "Data type unsupported by CMSIS-NN"

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

#define INPUT_CHANNELS    9
#define CONV_FILTERS      8
#define CONV_KERNEL_SIZE  3
#define CONV_GROUPS       1


const float  conv1d_1_bias[CONV_FILTERS] = {-0x1.559ebe0000000p-2, -0x1.82fd720000000p-2, -0x1.91429a0000000p-4, -0x1.37308e0000000p-2, 0x1.4b95120000000p-4, -0x1.57a0640000000p-4, -0x1.e6c9900000000p-7, -0x1.c5f77c0000000p-3}
;

const float  conv1d_1_kernel[CONV_FILTERS][CONV_KERNEL_SIZE][INPUT_CHANNELS / CONV_GROUPS] = {{{0x1.bb24920000000p-4, 0x1.e2b9c20000000p-3, 0x1.d466040000000p-2, 0x1.4f1d0c0000000p-3, 0x1.b122540000000p-4, -0x1.1d63ec0000000p-4, -0x1.ad87880000000p-8, 0x1.9248b40000000p-8, 0x1.09145a0000000p-4}
, {-0x1.fea3e00000000p-7, 0x1.04e9640000000p-5, 0x1.3a1f060000000p-7, 0x1.b1280a0000000p-4, 0x1.e2b4740000000p-4, 0x1.44b35c0000000p-4, 0x1.335ac80000000p-4, 0x1.0ce00c0000000p-10, 0x1.bbf5840000000p-6}
, {-0x1.982e600000000p-4, -0x1.20116c0000000p-2, -0x1.6eeb820000000p-2, -0x1.7df2a00000000p-2, -0x1.642b7a0000000p-2, -0x1.333c400000000p-2, -0x1.b0f70a0000000p-3, -0x1.b31ba80000000p-2, -0x1.18ed220000000p-3}
}
, {{0x1.3e38800000000p-3, 0x1.b3a46e0000000p-7, 0x1.9095c80000000p-5, 0x1.e0b0ac0000000p-4, 0x1.12a37a0000000p-5, 0x1.772cea0000000p-3, -0x1.8f7c6a0000000p-2, 0x1.3d9ef80000000p-1, -0x1.d8a6520000000p-3}
, {-0x1.2da87a0000000p-3, -0x1.e5ec7a0000000p-5, 0x1.41325a0000000p-3, -0x1.56b3b20000000p-2, 0x1.506bdc0000000p-2, -0x1.049c6e0000000p-2, 0x1.485d6a0000000p-1, -0x1.9d0d080000000p-2, 0x1.528e160000000p-3}
, {0x1.16a1a60000000p-5, 0x1.54a8dc0000000p-3, -0x1.5822500000000p-2, 0x1.a585480000000p-3, -0x1.6179160000000p-3, -0x1.6f47f40000000p-4, -0x1.b3b8000000000p-3, 0x1.c8003a0000000p-3, -0x1.77feae0000000p-4}
}
, {{0x1.6b71b00000000p-4, -0x1.72d0940000000p-3, 0x1.10e3dc0000000p-3, -0x1.5deb520000000p-4, -0x1.fcd2ee0000000p-3, 0x1.03daa20000000p-2, -0x1.1c41cc0000000p-2, -0x1.4134760000000p-4, -0x1.38db580000000p-4}
, {-0x1.6c42160000000p-3, 0x1.70d3e40000000p-3, -0x1.f932ce0000000p-2, 0x1.a8b0180000000p-2, -0x1.50190c0000000p-2, -0x1.0c13b20000000p-1, 0x1.b0a4180000000p-2, -0x1.0b6e5c0000000p-2, -0x1.3152480000000p-6}
, {0x1.f26c560000000p-5, -0x1.716cdc0000000p-3, 0x1.25d23c0000000p-2, -0x1.0785200000000p-1, 0x1.3ef3f80000000p-3, 0x1.9eae4e0000000p-4, -0x1.b8455e0000000p-2, 0x1.167a580000000p-4, 0x1.5429280000000p-2}
}
, {{-0x1.8d51900000000p-6, 0x1.62d0420000000p-3, -0x1.2b6bf00000000p-6, -0x1.55551e0000000p-3, 0x1.a06cfa0000000p-2, -0x1.4595940000000p-1, 0x1.8a17e80000000p-2, -0x1.9dabb40000000p-3, 0x1.6ec3dc0000000p-4}
, {-0x1.0ab4a20000000p-4, -0x1.0c2eae0000000p-2, 0x1.410b820000000p-2, 0x1.734d680000000p-6, 0x1.18fe4e0000000p-4, 0x1.ae743c0000000p-2, -0x1.741cb60000000p-3, 0x1.80c6f60000000p-4, 0x1.19bcb80000000p-4}
, {0x1.4d2bac0000000p-5, 0x1.c715000000000p-4, -0x1.2f518c0000000p-3, 0x1.a084860000000p-3, -0x1.36e4a80000000p-2, 0x1.f392f80000000p-3, -0x1.03fee20000000p-2, 0x1.8f6dda0000000p-4, -0x1.2400de0000000p-4}
}
, {{-0x1.074a7e0000000p-3, -0x1.3145b20000000p-5, 0x1.e89c740000000p-7, -0x1.6305c80000000p-5, -0x1.8d201a0000000p-4, -0x1.68c5580000000p-2, -0x1.62ffb40000000p-4, -0x1.2588e80000000p-5, -0x1.de46700000000p-6}
, {-0x1.7108b80000000p-4, -0x1.3e15d40000000p-2, 0x1.19ea040000000p-5, -0x1.a72bd40000000p-3, -0x1.3b939a0000000p-2, 0x1.3308280000000p-2, -0x1.c95af80000000p-2, 0x1.387e0a0000000p-2, -0x1.074c440000000p-1}
, {0x1.9140de0000000p-6, -0x1.003fc40000000p-1, 0x1.7210180000000p-4, 0x1.12d5b20000000p-1, -0x1.bc3cae0000000p-2, -0x1.444c480000000p-2, 0x1.ebe9f80000000p-2, -0x1.0ca6880000000p-3, -0x1.f9d3680000000p-2}
}
, {{0x1.da14c60000000p-5, 0x1.4b29900000000p-4, -0x1.271b360000000p-2, 0x1.2292200000000p-1, -0x1.eb18460000000p-2, 0x1.300a860000000p-3, 0x1.83d35e0000000p-2, -0x1.68a0860000000p-2, 0x1.c0a6ce0000000p-3}
, {0x1.3c87440000000p-5, 0x1.5b74b80000000p-6, 0x1.1806ea0000000p-4, -0x1.0f4d2a0000000p-3, 0x1.1fb96a0000000p-3, -0x1.1d45b60000000p-2, 0x1.02a0120000000p-4, -0x1.2195d40000000p-3, -0x1.cd67380000000p-5}
, {0x1.6ab1760000000p-6, -0x1.2b15500000000p-4, 0x1.0ba86a0000000p-2, -0x1.31fcf20000000p-2, 0x1.40aab60000000p-2, 0x1.c2da300000000p-8, -0x1.3248660000000p-2, 0x1.5d8b400000000p-2, 0x1.66c9660000000p-5}
}
, {{-0x1.b612740000000p-5, 0x1.e6b1c80000000p-5, 0x1.298e2a0000000p-4, -0x1.178fa60000000p-2, 0x1.818ac40000000p-2, -0x1.59bf700000000p-2, -0x1.205d2c0000000p-3, -0x1.2579480000000p-2, 0x1.ec08240000000p-4}
, {0x1.3b75900000000p-3, -0x1.4cb2560000000p-2, 0x1.e1b2980000000p-4, 0x1.5ce51e0000000p-3, -0x1.12818a0000000p-1, 0x1.3950600000000p-2, -0x1.bf6b800000000p-6, -0x1.820e140000000p-2, 0x1.97ac8e0000000p-4}
, {-0x1.7d6a200000000p-4, 0x1.1a7a7e0000000p-2, -0x1.245dd60000000p-2, -0x1.3bcc060000000p-7, -0x1.3f10080000000p-4, -0x1.e789a60000000p-2, -0x1.4b33b00000000p-3, 0x1.0d2b520000000p-4, -0x1.28db260000000p-1}
}
, {{-0x1.ff9dec0000000p-4, -0x1.0937aa0000000p-4, -0x1.6b898e0000000p-3, -0x1.53b6120000000p-3, -0x1.9ae1b60000000p-5, 0x1.7e7f4c0000000p-3, 0x1.efc9440000000p-3, 0x1.1ada9c0000000p-2, 0x1.26019c0000000p-4}
, {-0x1.d241460000000p-3, 0x1.6619d40000000p-10, -0x1.21160e0000000p-5, 0x1.4bf2020000000p-3, -0x1.5512040000000p-6, 0x1.6e7e4c0000000p-6, -0x1.73e1520000000p-3, 0x1.f085b40000000p-4, -0x1.684b820000000p-4}
, {0x1.a4e4dc0000000p-2, 0x1.2d43080000000p-6, 0x1.db0b540000000p-3, -0x1.9b3ed40000000p-5, 0x1.26a2ce0000000p-5, -0x1.7dd2200000000p-2, -0x1.1c4d960000000p-2, -0x1.da4dac0000000p-11, 0x1.fc070a0000000p-5}
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

#ifndef _BATCH_NORMALIZATION_1_H_
#define _BATCH_NORMALIZATION_1_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS      8
#define INPUT_SAMPLES       44

typedef float batch_normalization_1_output_type[44][8];

#if 0
void batch_normalization_1(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const number_t kernel[INPUT_CHANNELS],                // IN
  const number_t bias[INPUT_CHANNELS],                  // IN
  batch_normalization_1_output_type output);                // OUT
#endif

#undef INPUT_CHANNELS
#undef INPUT_SAMPLES

#endif//_BATCH_NORMALIZATION_1_H_
/**
  ******************************************************************************
  * @file    batchnorm1d.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 august 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "batch_normalization_1.h"
#include "number.h"
#endif

#define INPUT_CHANNELS      8
#define INPUT_SAMPLES       44
#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_NONE
#define NUMBER_T float
#define LONG_NUMBER_T float


static inline void batch_normalization_1(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS],  // IN
  const NUMBER_T kernel[INPUT_CHANNELS],                // IN
  const NUMBER_T bias[INPUT_CHANNELS],                  // IN
  batch_normalization_1_output_type output) {                // OUT

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

const float batch_normalization_1_bias[8] = {-0x1.aa391c0000000p-3, -0x1.a1e2700000000p-3, -0x1.b953f60000000p-2, -0x1.e60fca0000000p-3, -0x1.4a4c0a0000000p-2, -0x1.2050700000000p-1, -0x1.c072c60000000p-2, -0x1.5aa2540000000p-2}
;
const float batch_normalization_1_kernel[8] = {0x1.059e480000000p+3, 0x1.2cfc3c0000000p+3, 0x1.5e89260000000p+1, 0x1.82bb040000000p+2, 0x1.4ab2840000000p-1, 0x1.e42a080000000p+1, 0x1.a314160000000p+1, 0x1.a238060000000p+2}
;
/**
  ******************************************************************************
  * @file    maxpool1d.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _MAX_POOLING1D_1_H_
#define _MAX_POOLING1D_1_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define INPUT_CHANNELS  8
#define INPUT_SAMPLES   44
#define POOL_SIZE       2
#define POOL_STRIDE     2
#define POOL_PAD        0 // Unsupported
#define POOL_LENGTH	    ( ( (INPUT_SAMPLES - POOL_SIZE + (2*POOL_PAD) ) / POOL_STRIDE ) + 1 )

typedef float max_pooling1d_1_output_type[POOL_LENGTH][INPUT_CHANNELS];

#if 0
void max_pooling1d_1(
  const number_t input[INPUT_SAMPLES][INPUT_CHANNELS], 	    // IN
  number_t output[POOL_LENGTH][INPUT_CHANNELS]); 	// OUT
#endif

#undef INPUT_CHANNELS  
#undef INPUT_SAMPLES
#undef POOL_SIZE
#undef POOL_STRIDE
#undef POOL_PAD
#undef POOL_LENGTH

#endif//_MAX_POOLING1D_1_H_
/**
  ******************************************************************************
  * @file    maxpool.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "max_pooling1d_1.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#endif

#define INPUT_CHANNELS  8
#define INPUT_SAMPLES   44
#define POOL_SIZE       2
#define POOL_STRIDE     2
#define POOL_PAD        0 // Unsupported
#define POOL_LENGTH	    ( ( (INPUT_SAMPLES - POOL_SIZE + (2*POOL_PAD) ) / POOL_STRIDE ) + 1 )

#define ACTIVATION_LINEAR

// For fixed point quantization
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_NONE
#define NUMBER_T float
#define LONG_NUMBER_T float


static inline void max_pooling1d_1(
  const NUMBER_T input[INPUT_SAMPLES][INPUT_CHANNELS], 	    // IN
  NUMBER_T output[POOL_LENGTH][INPUT_CHANNELS]) {	// OUT

  unsigned short pos_x, k; 	// loop indexes for output volume
  unsigned int x;
  static LONG_NUMBER_T max[INPUT_CHANNELS];

  for (pos_x = 0; pos_x < POOL_LENGTH; pos_x++) {
    for (k = 0; k < INPUT_CHANNELS; k++) {
#ifdef ACTIVATION_LINEAR
      max[k] = input[pos_x*POOL_STRIDE][k];
      x = 1;
#elif defined(ACTIVATION_RELU)
      max[k] = 0;
      x = 0;
#else
#error "Unsupported activation function"
#endif
    }

    for (; x < POOL_SIZE; x++) {
      for (k = 0; k < INPUT_CHANNELS; k++) {
        if (max[k] < input[(pos_x * POOL_STRIDE) + x][k])
          max[k] = input[(pos_x * POOL_STRIDE) + x][k];
      }
    }

    for (k = 0; k < INPUT_CHANNELS; k++) {
      output[pos_x][k] = scale_and_clamp_to(NUMBER_T, max[k], INPUT_SCALE_FACTOR - OUTPUT_SCALE_FACTOR, OUTPUT_ROUND_MODE);
    }
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
#undef NUMBER_T
#undef LONG_NUMBER_T
/**
  ******************************************************************************
  * @file    flatten.hh
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version V2.0
  * @date    24 january 2023
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef _FLATTEN_1_H_
#define _FLATTEN_1_H_

#ifndef SINGLE_FILE
#include "number.h"
#endif

#define OUTPUT_DIM 176

typedef float flatten_1_output_type[OUTPUT_DIM];

#if 0
void flatten_1(
  const number_t input[22][8], 			      // IN
	number_t output[OUTPUT_DIM]); 			                // OUT
#endif

#undef OUTPUT_DIM

#endif//_FLATTEN_1_H_
/**
  ******************************************************************************
  * @file    flatten.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 2.0.0
  * @date    26 november 2021
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "flatten_1.h"
#include "number.h"
#endif

#define OUTPUT_DIM 176

#define NUMBER_T float
#define LONG_NUMBER_T float

static inline void flatten_1(
  const NUMBER_T input[22][8], 			      // IN
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

#ifndef _DENSE_1_H_
#define _DENSE_1_H_

#ifndef SINGLE_FILE
#include "number.h"
#include <stdint.h>
#endif

#define INPUT_SAMPLES 176
#define FC_UNITS 2

typedef float dense_1_output_type[FC_UNITS];

#if 0
void dense_1(
  const number_t input[INPUT_SAMPLES], 			      // IN
	const number_t kernel[FC_UNITS][INPUT_SAMPLES],  // IN

	const number_t bias[FC_UNITS],			              // IN

	number_t output[FC_UNITS]); 			                // OUT
#endif

#undef INPUT_SAMPLES
#undef FC_UNITS

#endif//_DENSE_1_H_
/**
  ******************************************************************************
  * @file    fc.cc
  * @author  Pierre-Emmanuel Novac <penovac@unice.fr>, LEAT, CNRS, Université Côte d'Azur, France
  * @version 1.0.0
  * @date    24 march 2020
  * @brief   Template generating plain C code for the implementation of Convolutional Neural Networks on MCU
  */

#ifndef SINGLE_FILE
#include "dense_1.h"
#include "number.h"
#endif

#ifdef WITH_CMSIS_NN
#include "arm_nnfunctions.h"
#elif defined(WITH_NMSIS_NN)
#include "riscv_nnfunctions.h"
#endif

#define INPUT_SAMPLES 176
#define FC_UNITS 2
#define ACTIVATION_LINEAR

// For fixed point quantization
#define WEIGHTS_SCALE_FACTOR 0
#define BIASES_SCALE_FACTOR 0
#define TMP_SCALE_FACTOR 0
#define INPUT_SCALE_FACTOR 0
#define OUTPUT_SCALE_FACTOR 0
#define OUTPUT_ROUND_MODE ROUND_MODE_NONE
#define NUMBER_T float
#define LONG_NUMBER_T float


static inline void dense_1(
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

#error "Data type unsupported by CMSIS-NN"

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

#define INPUT_SAMPLES 176
#define FC_UNITS 2


const float dense_1_bias[FC_UNITS] = {-0x1.a1fafe0000000p-10, 0x1.a200080000000p-10}
;

const float dense_1_kernel[FC_UNITS][INPUT_SAMPLES] = {{0x1.f774c80000000p-3, 0x1.34cd0c0000000p-4, -0x1.4bb10e0000000p-9, -0x1.1766920000000p-3, -0x1.75d4b40000000p-3, -0x1.15a5740000000p-4, -0x1.fc68080000000p-4, 0x1.8092620000000p-2, 0x1.b2e87e0000000p-4, 0x1.401d8c0000000p-4, -0x1.64aabe0000000p-3, -0x1.3c16300000000p-4, -0x1.e202680000000p-4, -0x1.83f3d20000000p-3, -0x1.9953600000000p-3, 0x1.e39ff60000000p-4, 0x1.ece27e0000000p-4, -0x1.0e0b600000000p-6, -0x1.80e6960000000p-4, -0x1.017a200000000p-4, -0x1.9201d60000000p-4, -0x1.7811d00000000p-5, -0x1.765f8e0000000p-7, -0x1.3e27780000000p-7, 0x1.7cf9520000000p-5, -0x1.9695720000000p-3, 0x1.b7b4320000000p-6, -0x1.80b9680000000p-4, -0x1.3846660000000p-8, -0x1.4736c20000000p-3, -0x1.d212e00000000p-5, -0x1.3766d00000000p-6, 0x1.f74b1a0000000p-3, -0x1.395d1a0000000p-5, -0x1.ca950c0000000p-3, -0x1.1b8a140000000p-3, -0x1.72b9a40000000p-3, 0x1.4351720000000p-4, -0x1.9b8a840000000p-3, 0x1.a250180000000p-3, 0x1.a669740000000p-6, 0x1.b78b7a0000000p-5, 0x1.6ee57c0000000p-4, 0x1.0e26080000000p-7, -0x1.a6066c0000000p-6, 0x1.11b5300000000p-5, -0x1.7e713a0000000p-3, 0x1.2437600000000p-3, 0x1.058c560000000p-5, -0x1.b88e4c0000000p-4, -0x1.4b9f900000000p-3, -0x1.1e39800000000p-5, -0x1.bc60400000000p-5, -0x1.90d45a0000000p-4, -0x1.30cc1a0000000p-3, 0x1.d1c0f60000000p-3, 0x1.0926580000000p-2, 0x1.c2768a0000000p-5, -0x1.237e760000000p-3, 0x1.056c1c0000000p-3, -0x1.052de40000000p-6, 0x1.20f41a0000000p-6, -0x1.dc86a60000000p-4, 0x1.d766b00000000p-4, 0x1.8869c20000000p-3, -0x1.8c97b80000000p-3, 0x1.cd76880000000p-5, -0x1.041fba0000000p-2, -0x1.b99fe80000000p-5, -0x1.4f4ce40000000p-5, -0x1.6ca9dc0000000p-7, 0x1.ccbc660000000p-3, 0x1.62c68e0000000p-4, -0x1.3427a00000000p-3, -0x1.83651e0000000p-3, -0x1.7122020000000p-4, 0x1.0cd4a40000000p-3, -0x1.2d52a80000000p-3, 0x1.e6df240000000p-4, 0x1.ba26d80000000p-4, 0x1.cb1e600000000p-6, -0x1.224c300000000p-5, 0x1.50f7ae0000000p-4, 0x1.57e23c0000000p-7, -0x1.ac6e0c0000000p-5, -0x1.de3da00000000p-4, -0x1.e6e09e0000000p-6, 0x1.807f640000000p-3, 0x1.d103fe0000000p-4, -0x1.8f2efc0000000p-5, -0x1.722dcc0000000p-3, -0x1.8b98520000000p-4, 0x1.0aebb20000000p-4, 0x1.fb14460000000p-5, -0x1.2faa420000000p-4, -0x1.8eb04a0000000p-5, 0x1.9924f00000000p-3, -0x1.08259c0000000p-3, -0x1.54538a0000000p-4, -0x1.660c720000000p-7, -0x1.76910a0000000p-4, -0x1.efdbe20000000p-4, -0x1.b8edbe0000000p-7, 0x1.28139a0000000p-3, 0x1.5a8d220000000p-3, -0x1.0612060000000p-4, -0x1.5241a60000000p-4, -0x1.b0c1480000000p-3, -0x1.93b3dc0000000p-6, -0x1.2fec2c0000000p-5, -0x1.3c66400000000p-9, 0x1.541ff20000000p-3, 0x1.4018080000000p-3, -0x1.5802680000000p-3, -0x1.2ca1c20000000p-3, -0x1.1cb9a60000000p-4, 0x1.4d85ca0000000p-4, -0x1.07bc200000000p-6, 0x1.1fa0660000000p-3, 0x1.66e9160000000p-7, 0x1.d5ef980000000p-6, -0x1.3e36440000000p-3, -0x1.27197e0000000p-4, -0x1.9ebf0e0000000p-6, -0x1.5d3e120000000p-5, -0x1.7ab9000000000p-8, -0x1.05d2fc0000000p-4, 0x1.db30b60000000p-4, 0x1.78123c0000000p-4, -0x1.78fb980000000p-4, -0x1.ee6be40000000p-4, -0x1.e665740000000p-4, -0x1.267fc60000000p-3, -0x1.c9d6a20000000p-9, -0x1.17bb060000000p-5, 0x1.0ed8ba0000000p-3, 0x1.d51ad40000000p-4, -0x1.1150200000000p-3, 0x1.2078c80000000p-8, 0x1.c7ec780000000p-10, 0x1.37b3b20000000p-6, -0x1.d915400000000p-3, 0x1.ba27920000000p-4, 0x1.40d6b20000000p-4, 0x1.131bb20000000p-4, 0x1.9157420000000p-8, -0x1.9c55920000000p-3, 0x1.65fa340000000p-6, -0x1.416eb00000000p-6, -0x1.aea3880000000p-6, -0x1.cd5ee20000000p-4, 0x1.5a77e00000000p-5, 0x1.d040be0000000p-3, 0x1.74135c0000000p-6, -0x1.71ed2a0000000p-3, 0x1.1d84d60000000p-6, 0x1.93184a0000000p-5, -0x1.f225d20000000p-3, -0x1.1c6e400000000p-4, 0x1.aaeaec0000000p-3, 0x1.2729860000000p-4, -0x1.9ba3560000000p-4, -0x1.52a29e0000000p-3, -0x1.d937600000000p-4, -0x1.7bfae00000000p-4, -0x1.ee51480000000p-3, -0x1.46dd6a0000000p-5, 0x1.c9e0b20000000p-4, 0x1.7aa7940000000p-6, -0x1.dab0660000000p-4, 0x1.96e7460000000p-5, -0x1.5e0ac20000000p-5, -0x1.61867e0000000p-4, -0x1.a72f5e0000000p-3, -0x1.db59340000000p-5, 0x1.1241d80000000p-2}
, {-0x1.3079a00000000p-3, 0x1.171ce20000000p-4, 0x1.6264360000000p-6, 0x1.0134440000000p-3, -0x1.dbb8dc0000000p-4, 0x1.1478680000000p-3, 0x1.ac143e0000000p-4, -0x1.7ae7100000000p-3, -0x1.70d9a60000000p-4, 0x1.390f4c0000000p-4, 0x1.a1237c0000000p-5, 0x1.07dc920000000p-4, 0x1.13044e0000000p-3, -0x1.0788c20000000p-6, -0x1.a1f7720000000p-5, -0x1.4cedea0000000p-3, -0x1.ac0f3a0000000p-6, 0x1.85f96e0000000p-4, 0x1.7054520000000p-3, 0x1.265cf00000000p-3, 0x1.1945b00000000p-4, 0x1.bfe4660000000p-6, -0x1.3b3c4a0000000p-4, -0x1.3209720000000p-2, -0x1.8b160e0000000p-3, -0x1.c9fdbc0000000p-8, 0x1.eca2f60000000p-5, 0x1.a861220000000p-4, 0x1.76a3840000000p-5, -0x1.a68fe20000000p-5, 0x1.b90b9c0000000p-6, -0x1.1fe2600000000p-2, -0x1.32ec2c0000000p-2, 0x1.13302c0000000p-4, -0x1.1e288c0000000p-5, 0x1.74639c0000000p-3, -0x1.2172dc0000000p-4, 0x1.0a06fa0000000p-4, -0x1.3699d00000000p-6, 0x1.06379a0000000p-5, -0x1.9f30580000000p-3, 0x1.7f9b5a0000000p-3, 0x1.52f5a00000000p-3, 0x1.297ef20000000p-6, 0x1.9d932e0000000p-6, 0x1.8ba83c0000000p-3, 0x1.8420880000000p-4, -0x1.77042a0000000p-6, -0x1.5145ba0000000p-3, 0x1.362dce0000000p-4, 0x1.d14eee0000000p-7, 0x1.ad918a0000000p-4, 0x1.a395e60000000p-7, 0x1.d04f100000000p-8, 0x1.1149b80000000p-4, -0x1.abc6000000000p-4, -0x1.e2c4ea0000000p-6, 0x1.9554180000000p-5, -0x1.83d6b40000000p-4, 0x1.2f90d40000000p-3, 0x1.0d7dd00000000p-5, 0x1.497e3c0000000p-3, 0x1.bb2ef00000000p-4, -0x1.2c66040000000p-3, -0x1.6767200000000p-4, -0x1.50a64a0000000p-4, 0x1.0e77780000000p-4, 0x1.a784d60000000p-7, -0x1.4d92aa0000000p-4, 0x1.96a80c0000000p-5, 0x1.71a94a0000000p-4, 0x1.84b38a0000000p-5, -0x1.d941b80000000p-3, 0x1.0dc5c60000000p-3, 0x1.ced72c0000000p-4, 0x1.3291440000000p-3, 0x1.a7baa80000000p-6, -0x1.0e582c0000000p-5, 0x1.f82af40000000p-4, -0x1.39e20a0000000p-3, -0x1.d807d40000000p-3, 0x1.2527320000000p-4, 0x1.0eff780000000p-2, 0x1.09af580000000p-3, 0x1.a563ce0000000p-6, 0x1.b515660000000p-5, 0x1.dca94a0000000p-5, -0x1.95d4440000000p-4, -0x1.4a381e0000000p-4, 0x1.262f1c0000000p-7, -0x1.dcea280000000p-5, 0x1.91e9f60000000p-3, 0x1.4027400000000p-3, 0x1.8761d40000000p-3, 0x1.0ce6500000000p-3, -0x1.d741a00000000p-3, 0x1.3154fe0000000p-5, 0x1.e662d60000000p-4, 0x1.c593ec0000000p-4, 0x1.303fd60000000p-4, 0x1.59817e0000000p-4, 0x1.88189e0000000p-8, -0x1.89cbca0000000p-7, -0x1.c0414e0000000p-3, -0x1.4d362e0000000p-4, 0x1.552e620000000p-3, -0x1.8e1bbc0000000p-8, -0x1.45963e0000000p-4, -0x1.0992ce0000000p-3, 0x1.a9ae140000000p-3, 0x1.7ec0be0000000p-4, -0x1.12fb600000000p-3, -0x1.033f280000000p-3, 0x1.bf73960000000p-4, 0x1.3df6660000000p-3, 0x1.2d910e0000000p-3, -0x1.af67ce0000000p-4, -0x1.afff300000000p-4, 0x1.ddc9040000000p-7, -0x1.0fe7d20000000p-2, -0x1.6038ba0000000p-3, -0x1.078bce0000000p-4, 0x1.77eb2e0000000p-4, 0x1.486b100000000p-4, -0x1.839d420000000p-5, -0x1.30001a0000000p-6, 0x1.2cdc480000000p-4, -0x1.40a3220000000p-3, -0x1.143d1e0000000p-3, 0x1.c22db40000000p-6, 0x1.b7afec0000000p-4, 0x1.2b07740000000p-3, -0x1.d04cbe0000000p-6, 0x1.d5cc740000000p-4, 0x1.793ad80000000p-5, 0x1.84e7200000000p-6, -0x1.7290080000000p-5, -0x1.989de40000000p-4, 0x1.ff469e0000000p-5, 0x1.9d1cf60000000p-4, 0x1.daa93e0000000p-4, -0x1.18c09a0000000p-5, 0x1.a9df7e0000000p-3, -0x1.1be7b80000000p-4, -0x1.3732300000000p-2, 0x1.8285060000000p-5, -0x1.d50ba80000000p-6, 0x1.3e0da80000000p-3, 0x1.562c140000000p-5, 0x1.6bd40e0000000p-3, 0x1.4ba2a00000000p-4, -0x1.c557b40000000p-3, -0x1.7dad2c0000000p-8, 0x1.b6c4680000000p-3, 0x1.9238700000000p-4, 0x1.0510860000000p-3, 0x1.527a9c0000000p-4, -0x1.bca0260000000p-5, -0x1.6dc4440000000p-8, -0x1.3aedce0000000p-6, -0x1.eec56e0000000p-3, -0x1.3e11300000000p-4, 0x1.e71b360000000p-4, 0x1.c4dea60000000p-5, 0x1.1ffc540000000p-4, -0x1.b3acea0000000p-6, 0x1.ce14340000000p-3, -0x1.0b50d80000000p-2, -0x1.c6ecc00000000p-3, 0x1.0ce9160000000p-2, 0x1.ce09300000000p-4, -0x1.42c3300000000p-9, 0x1.655d620000000p-2, -0x1.ca30e40000000p-6, 0x1.259c940000000p-3, -0x1.edc8860000000p-5}
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
#include "average_pooling1d_1.h" // InputLayer is excluded
#include "conv1d_1.h" // InputLayer is excluded
#include "batch_normalization_1.h" // InputLayer is excluded
#include "max_pooling1d_1.h" // InputLayer is excluded
#include "flatten_1.h" // InputLayer is excluded
#include "dense_1.h"
#endif


#define MODEL_INPUT_DIM_0 186
#define MODEL_INPUT_DIM_1 9
#define MODEL_INPUT_DIMS 186 * 9

#define MODEL_OUTPUT_SAMPLES 2

#define MODEL_INPUT_SCALE_FACTOR 0 // scale factor of InputLayer
#define MODEL_INPUT_ROUND_MODE ROUND_MODE_NONE
#define MODEL_INPUT_NUMBER_T float
#define MODEL_INPUT_LONG_NUMBER_T float

// node 0 is InputLayer so use its output shape as input shape of the model
// typedef  input_t[186][9];
typedef float input_t[186][9];
typedef dense_1_output_type output_t;


void cnn_detect(
  const input_t input,
  dense_1_output_type output);

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
#include "average_pooling1d_1.c" // InputLayer is excluded
#include "conv1d_1.c"
#include "weights/conv1d_1.c" // InputLayer is excluded
#include "batch_normalization_1.c"
#include "weights/batch_normalization_1.c" // InputLayer is excluded
#include "max_pooling1d_1.c" // InputLayer is excluded
#include "flatten_1.c" // InputLayer is excluded
#include "dense_1.c"
#include "weights/dense_1.c"
#endif


void cnn_detect(
  const input_t input,
  dense_1_output_type dense_1_output) {
  
#if 1
  // Output array allocation
  static union {
    average_pooling1d_1_output_type average_pooling1d_1_output;
    batch_normalization_1_output_type batch_normalization_1_output;
  } activations1;

  static union {
    conv1d_1_output_type conv1d_1_output;
    max_pooling1d_1_output_type max_pooling1d_1_output;
    flatten_1_output_type flatten_1_output;
  } activations2;
#endif


// Model layers call chain 
  
  
  average_pooling1d_1( // First layer uses input passed as model parameter
    input,
    activations1.average_pooling1d_1_output
    );
  
  
  conv1d_1(
    activations1.average_pooling1d_1_output,
    conv1d_1_kernel,
    conv1d_1_bias,
    activations2.conv1d_1_output
    );
  
  
  batch_normalization_1(
    activations2.conv1d_1_output,
    batch_normalization_1_kernel,
    batch_normalization_1_bias,
    activations1.batch_normalization_1_output
    );
  
  
  max_pooling1d_1(
    activations1.batch_normalization_1_output,
    activations2.max_pooling1d_1_output
    );
  
  
  flatten_1(
    activations2.max_pooling1d_1_output,
    activations2.flatten_1_output
    );
  
  
  dense_1(
    activations2.flatten_1_output,
    dense_1_kernel,
    dense_1_bias,// Last layer uses output passed as model parameter
    dense_1_output
    );
}

#ifdef __cplusplus
} // extern "C"
#endif
