#include "ADC3101.h"

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>

LOG_MODULE_REGISTER(adc3101, LOG_LEVEL_INF);

#define ADC3101_MICPGA_GAIN_DB      46U
#define ADC3101_ADC_DIGITAL_GAIN_DB 8U

/* Internal state */
static const struct device *s_i2c = NULL;
static uint8_t s_addr = ADC3101_ADDR00;
static bool s_debug = false; /* default off */
static uint32_t s_last_recover_ms = 0U;

static bool is_recoverable_i2c_error(int ret)
{
    return (ret == -EIO) || (ret == -EBUSY) || (ret == -ETIMEDOUT);
}

static bool adc3101_should_recover_now(void)
{
    uint32_t now = k_uptime_get_32();
    uint32_t delta = now - s_last_recover_ms;
    if (delta < 120U) {
        return false;
    }
    s_last_recover_ms = now;
    return true;
}

void adc3101_init(const struct device *i2c, uint8_t address)
{
    s_i2c = i2c;
    s_addr = address;
}

int adc3101_write(uint8_t reg, uint8_t val)
{
    if (s_i2c == NULL || !device_is_ready(s_i2c)) {
        return -ENODEV;
    }

    uint8_t buf[2] = {reg, val};
    int ret = i2c_write(s_i2c, buf, sizeof(buf), s_addr);
    if (ret == 0) {
        LOG_DBG("I2C write ok: addr=0x%02x reg=0x%02x val=0x%02x", s_addr, reg, val);
        return 0;
    }

    LOG_WRN("I2C write failed: addr=0x%02x reg=0x%02x val=0x%02x ret=%d", s_addr, reg, val, ret);
    if (is_recoverable_i2c_error(ret) && adc3101_should_recover_now()) {
        int rec = i2c_recover_bus(s_i2c);
        if (rec != 0) {
            LOG_WRN("I2C recover after write failed: %d", rec);
            return ret;
        }
        k_msleep(5);
        ret = i2c_write(s_i2c, buf, sizeof(buf), s_addr);
    }

    return ret;
}

int adc3101_read(uint8_t reg)
{
    if (s_i2c == NULL || !device_is_ready(s_i2c)) {
        return -ENODEV;
    }

    uint8_t val = 0;
    int ret = i2c_write_read(s_i2c, s_addr, &reg, 1, &val, 1);
    if (ret == 0) {
        LOG_DBG("I2C read ok: addr=0x%02x reg=0x%02x val=0x%02x", s_addr, reg, val);
        return (int)val;
    }

    LOG_WRN("I2C read failed: addr=0x%02x reg=0x%02x ret=%d", s_addr, reg, ret);
    if (is_recoverable_i2c_error(ret) && adc3101_should_recover_now()) {
        int rec = i2c_recover_bus(s_i2c);
        if (rec != 0) {
            LOG_WRN("I2C recover after read failed: %d", rec);
            return ret;
        }
        k_msleep(5);
        ret = i2c_write_read(s_i2c, s_addr, &reg, 1, &val, 1);
        if (ret == 0) {
            LOG_DBG("I2C read ok after recover: addr=0x%02x reg=0x%02x val=0x%02x", s_addr, reg, val);
            return (int)val;
        }
    }

    return ret;
}

/* write with retry and optional readback verification */
static int adc3101_write_retry(uint8_t reg, uint8_t val, int retries, int delay_ms, bool verify)
{
    int ret = 0;
    for (int attempt = 0; attempt < retries; ++attempt) {
        ret = adc3101_write(reg, val);
        if (ret == 0 && verify) {
            int rd = adc3101_read(reg);
            if (rd < 0) {
                LOG_ERR("I2C verify read failed: reg=0x%02x ret=%d", reg, rd);
                ret = rd;
            } else if (rd != val) {
                LOG_ERR("I2C verify mismatch: reg=0x%02x write=0x%02x read=0x%02x", reg, val, rd);
                ret = -EIO;
            } else {
                LOG_DBG("I2C verify ok: reg=0x%02x = 0x%02x", reg, val);
            }
        }

        if (ret == 0) {
            return 0;
        }
        if (delay_ms > 0) {
            k_msleep(delay_ms);
        }
    }
    return ret;
}

void adc3101_setup(void)
{
    /* 1. Define starting point */
    LOG_DBG("1. Define starting point:");
    LOG_DBG("Set register page to 0");
    (void)adc3101_write_retry(0x00, 0x00, 8, 5, true);
    k_msleep(10);

    /* Software reset */
    LOG_DBG("Initiate software reset");
    (void)adc3101_write_retry(0x01, 0x01, 8, 5, false);
    k_msleep(20);

    /* 2. Program Clock Settings */
    LOG_DBG("2. Program Clock Settings");
    LOG_DBG("ADC_CLKIN = MCLK, P=1, R=1, J=4, D=0000");
    (void)adc3101_write_retry(0x04, 0x00, 8, 5, true); /* CODEC_CLKIN = MCLK */
    (void)adc3101_write_retry(0x05, 0x11, 8, 5, true); /* PLL power down, P=1, R=1 */
    (void)adc3101_write_retry(0x06, 0x04, 8, 5, true); /* J=4 */
    (void)adc3101_write_retry(0x07, 0x00, 8, 5, true); /* D MSB */
    (void)adc3101_write_retry(0x08, 0x00, 8, 5, true); /* D LSB */

    /* NADC/MADC/AOSR */
    LOG_DBG("Program and power up NADC: NADC = 1, divider powered on");
    int ok_nadc = adc3101_write_retry(0x12, 0x81, 8, 5, true);

    LOG_DBG("Program and power up MADC: MADC = 2, divider powered on");
    int ok_madc = adc3101_write_retry(0x13, 0x82, 8, 5, true);

    LOG_DBG("Program OSR value: AOSR = 128");
    int ok_aosr = adc3101_write_retry(0x14, 0x80, 8, 5, true);

    LOG_DBG("Program I2S wordlength: 16-bit, slave mode");
    (void)adc3101_write_retry(0x1B, 0x00, 8, 5, true);

    LOG_DBG("Program the processing block to be used: PRB_P1");
    (void)adc3101_write_retry(0x3D, 0x01, 8, 5, true);
    k_msleep(10);

    /* 3. Program Analog Blocks */
    LOG_DBG("3. Program Analog Blocks");
    LOG_DBG("Set register Page to 1");
    (void)adc3101_write_retry(0x00, 0x01, 8, 5, true);
    k_msleep(10);

    LOG_DBG("Program MICBIAS: MICBIAS1 = 3.3V, MICBIAS2 = 3.3V");
    (void)adc3101_write_retry(0x33, 0b01111000, 8, 5, true); /* 3.3V */

    LOG_INF("Program MicPGA: Left/Right = %udB", ADC3101_MICPGA_GAIN_DB);
    (void)adc3101_write_retry(0x3b, (uint8_t)(ADC3101_MICPGA_GAIN_DB * 2U), 8, 5, true);
    (void)adc3101_write_retry(0x3c, (uint8_t)(ADC3101_MICPGA_GAIN_DB * 2U), 8, 5, true);

    LOG_DBG("Input selection: IN1L/IN1R as Single-Ended");
    (void)adc3101_write_retry(0x34, 0b00111111, 8, 5, true); /* Left ADC input selection */
    (void)adc3101_write_retry(0x37, 0b00111111, 8, 5, true); /* Right ADC input selection */

    /* 4. Program ADC */
    LOG_DBG("4. Program ADC");
    LOG_DBG("Set register Page to 0");
    (void)adc3101_write_retry(0x00, 0x00, 8, 5, true);
    k_msleep(10);

    LOG_DBG("Power up ADC channels");
    if (ok_nadc != 0 || ok_madc != 0 || ok_aosr != 0) {
        LOG_WRN("NADC/MADC/AOSR writes had errors; proceeding to power-up to avoid RX stall");
    }
    (void)adc3101_write_retry(0x51, 0xc2, 8, 5, true);
    k_msleep(10);

    LOG_DBG("Unmute digital volume control and set gain = 0 dB");
    (void)adc3101_write_retry(0x52, 0b00000000, 8, 5, true);

    LOG_DBG("Set left/right ADC volume control = %u dB", ADC3101_ADC_DIGITAL_GAIN_DB);
    (void)adc3101_write_retry(0x53, (uint8_t)(ADC3101_ADC_DIGITAL_GAIN_DB * 2U), 8, 5, true);
    (void)adc3101_write_retry(0x54, (uint8_t)(ADC3101_ADC_DIGITAL_GAIN_DB * 2U), 8, 5, true);

#if 1 /* High-pass filter */
    LOG_DBG("5. Program filters");
    LOG_DBG("Set register Page to 4");
    (void)adc3101_write_retry(0x00, 0x04, 8, 5, true);
    k_msleep(10);

    /* 30Hz high-pass Butterworth 1st order 0dB */
    (void)adc3101_write_retry(8, 0x7F, 8, 5, true);  /* N0 MSB */
    (void)adc3101_write_retry(9, 0x3F, 8, 5, true);  /* N0 LSB */
    (void)adc3101_write_retry(10, 0x80, 8, 5, true); /* N1 MSB */
    (void)adc3101_write_retry(11, 0xC1, 8, 5, true); /* N1 LSB */
    (void)adc3101_write_retry(12, 0x7E, 8, 5, true); /* D1 MSB */
    (void)adc3101_write_retry(13, 0x7F, 8, 5, true); /* D1 LSB */
#endif
}
