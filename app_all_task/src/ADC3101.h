#ifndef _ADC3101_H_
#define _ADC3101_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>

#define ADC3101_ADDR00 0x18

#define ADC3101_SETUP_OK          0
#define ADC3101_SETUP_WEAK_VERIFY 1

/* Initialize ADC3101 driver with I2C device and address */
void adc3101_init(const struct device *i2c, uint8_t address);

/* Simple register accessors with status feedback */
int adc3101_write(uint8_t reg, uint8_t val);
int adc3101_read(uint8_t reg);

/* Wait until the device ACKs after a cold power-up/reset. */
int adc3101_wait_ready(uint32_t timeout_ms);

/* Run the cold setup sequence (clocking, analog, ADC power, filters). */
int adc3101_setup(bool *weak_verify);

/* Best-effort software shutdown before V_PERIPH is removed. */
int adc3101_soft_powerdown(void);

#endif /* _ADC3101_H_ */
