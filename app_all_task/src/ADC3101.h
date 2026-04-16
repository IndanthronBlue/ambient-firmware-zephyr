#ifndef _ADC3101_H_
#define _ADC3101_H_

#include <stdint.h>
#include <zephyr/device.h>

#include "tasks.h"

#define ADC3101_ADDR00 0x18
#define ADC3101_ADDR01 0x19
#define ADC3101_ADDR10 0x1A
#define ADC3101_ADDR11 0x1B

/* Initialize ADC3101 driver with I2C device and address */
void adc3101_init(const struct device *i2c, uint8_t address);

/* Simple register accessors with status feedback */
int adc3101_write(uint8_t reg, uint8_t val);
int adc3101_read(uint8_t reg);

/* Run the setup sequence (clocking, analog, ADC power, filters) */
void adc3101_setup(void);

void adc3101_suspend(void);
void adc3101_resume(void);


#endif /* _ADC3101_H_ */
