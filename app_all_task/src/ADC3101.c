#include "ADC3101.h"

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "tasks.h"

LOG_MODULE_REGISTER(adc3101, LOG_LEVEL_INF);

#define ADC3101_REG_PAGE_SELECT      0x00
#define ADC3101_REG_RESET            0x01
#define ADC3101_REG_CLKMUX           0x04
#define ADC3101_REG_PLL_P_R          0x05
#define ADC3101_REG_PLL_J            0x06
#define ADC3101_REG_PLL_D_MSB        0x07
#define ADC3101_REG_PLL_D_LSB        0x08
#define ADC3101_REG_NADC             0x12
#define ADC3101_REG_MADC             0x13
#define ADC3101_REG_AOSR             0x14
#define ADC3101_REG_AUDIO_IF         0x1B
#define ADC3101_REG_PROCESSING_BLOCK 0x3D
#define ADC3101_REG_ADC_POWER        0x51
#define ADC3101_REG_ADC_FINE_VOLUME  0x52
#define ADC3101_REG_LEFT_VOLUME      0x53
#define ADC3101_REG_RIGHT_VOLUME     0x54

#define ADC3101_PAGE_ANALOG          0x01
#define ADC3101_REG_MICBIAS          0x33
#define ADC3101_REG_LEFT_INPUT       0x34
#define ADC3101_REG_RIGHT_INPUT      0x37
#define ADC3101_REG_LEFT_PGA         0x3B
#define ADC3101_REG_RIGHT_PGA        0x3C

#define ADC3101_PAGE_FILTER          0x04

#define ADC3101_MICPGA_GAIN_DB       0U
#define ADC3101_ADC_DIGITAL_GAIN_DB  0U
#define ADC3101_RECOVER_GUARD_MS     50U
#define ADC3101_READY_POLL_MS        5U

static const struct device *s_i2c;
static uint8_t s_addr = ADC3101_ADDR00;
static uint32_t s_last_recover_ms;

static bool adc3101_is_ready(void)
{
	return (s_i2c != NULL) && device_is_ready(s_i2c);
}

static bool is_recoverable_i2c_error(int ret)
{
	return (ret == -EIO) || (ret == -EBUSY) || (ret == -ETIMEDOUT);
}

static bool adc3101_should_recover_now(void)
{
	uint32_t now = k_uptime_get_32();
	uint32_t delta = now - s_last_recover_ms;

	if (delta < ADC3101_RECOVER_GUARD_MS) {
		return false;
	}

	s_last_recover_ms = now;
	return true;
}

void adc3101_init(const struct device *i2c, uint8_t address)
{
	s_i2c = i2c;
	s_addr = address;
	s_last_recover_ms = 0U;
}

static int adc3101_write_raw(uint8_t reg, uint8_t val, bool quiet)
{
	uint8_t buf[2] = {reg, val};
	int ret;
	int first_ret;

	if (!adc3101_is_ready()) {
		return -ENODEV;
	}

	task_i2c2_lock();
	ret = i2c_write(s_i2c, buf, sizeof(buf), s_addr);
	first_ret = ret;
	if ((ret != 0) && is_recoverable_i2c_error(ret) && adc3101_should_recover_now()) {
		int rec = i2c_recover_bus(s_i2c);

		if (rec == 0) {
			k_msleep(2);
			ret = i2c_write(s_i2c, buf, sizeof(buf), s_addr);
		} else {
			LOG_DBG("I2C recover after write failed: %d", rec);
		}
	}
	task_i2c2_unlock();

	if (ret == 0) {
		if (first_ret != 0) {
			LOG_DBG("I2C write ok after retry: addr=0x%02x reg=0x%02x val=0x%02x first=%d",
				s_addr, reg, val, first_ret);
		}
		return 0;
	}

	if (!quiet) {
		LOG_WRN("I2C write failed: addr=0x%02x reg=0x%02x val=0x%02x first=%d final=%d",
			s_addr, reg, val, first_ret, ret);
	}
	return ret;
}

int adc3101_write(uint8_t reg, uint8_t val)
{
	return adc3101_write_raw(reg, val, false);
}

static int adc3101_read_u8(uint8_t reg, uint8_t *val, bool quiet, bool allow_recover)
{
	int ret;
	int first_ret;

	if ((val == NULL) || !adc3101_is_ready()) {
		return -ENODEV;
	}

	task_i2c2_lock();
	ret = i2c_write_read(s_i2c, s_addr, &reg, 1, val, 1);
	first_ret = ret;
	if (allow_recover && (ret != 0) && is_recoverable_i2c_error(ret) &&
	    adc3101_should_recover_now()) {
		int rec = i2c_recover_bus(s_i2c);

		if (rec == 0) {
			k_msleep(2);
			ret = i2c_write_read(s_i2c, s_addr, &reg, 1, val, 1);
		} else {
			LOG_DBG("I2C recover after read failed: %d", rec);
		}
	}
	task_i2c2_unlock();

	if (ret == 0) {
		if (first_ret != 0) {
			LOG_DBG("I2C read ok after retry: addr=0x%02x reg=0x%02x val=0x%02x first=%d",
				s_addr, reg, *val, first_ret);
		}
		return 0;
	}

	if (!quiet) {
		LOG_WRN("I2C read failed: addr=0x%02x reg=0x%02x first=%d final=%d",
			s_addr, reg, first_ret, ret);
	}

	return ret;
}

int adc3101_read(uint8_t reg)
{
	uint8_t val = 0U;
	int ret = adc3101_read_u8(reg, &val, false, true);

	if (ret < 0) {
		return ret;
	}

	return (int)val;
}

int adc3101_wait_ready(uint32_t timeout_ms)
{
	uint32_t start = k_uptime_get_32();
	uint8_t val = 0U;
	int ret;

	if (!adc3101_is_ready()) {
		return -ENODEV;
	}

	do {
		ret = adc3101_read_u8(ADC3101_REG_PAGE_SELECT, &val, true, false);
		if (ret == 0) {
			LOG_INF("ADC3101 ACK at 0x%02x (reg0=0x%02x)",
				(unsigned int)s_addr, (unsigned int)val);
			return 0;
		}

		k_msleep(ADC3101_READY_POLL_MS);
	} while ((k_uptime_get_32() - start) < timeout_ms);

	LOG_WRN("ADC3101 no ACK at 0x%02x after %u ms: %d",
		(unsigned int)s_addr, (unsigned int)timeout_ms, ret);
	return ret;
}

static int adc3101_select_page(uint8_t *cur_page, uint8_t page)
{
	int ret;

	if ((cur_page != NULL) && (*cur_page == page)) {
		return 0;
	}

	ret = adc3101_write(ADC3101_REG_PAGE_SELECT, page);
	if ((ret == 0) && (cur_page != NULL)) {
		*cur_page = page;
	}

	return ret;
}

static int adc3101_write_page(uint8_t *cur_page, uint8_t page, uint8_t reg, uint8_t val)
{
	int ret = adc3101_select_page(cur_page, page);

	if (ret < 0) {
		return ret;
	}

	return adc3101_write(reg, val);
}

static int adc3101_verify_basic(void)
{
	uint8_t val = 0U;
	uint8_t page = 0xffU;
	int ret;

	ret = adc3101_select_page(&page, 0U);
	if (ret < 0) {
		return ret;
	}

	ret = adc3101_read_u8(ADC3101_REG_ADC_POWER, &val, false, true);
	if (ret < 0) {
		return ret;
	}
	if ((val & 0xc0U) != 0xc0U) {
		LOG_WRN("ADC power verify mismatch: reg=0x%02x", val);
		return -EIO;
	}

	ret = adc3101_read_u8(ADC3101_REG_AUDIO_IF, &val, false, true);
	if (ret < 0) {
		return ret;
	}
	if (val != 0x00U) {
		LOG_WRN("I2S format verify mismatch: reg=0x%02x", val);
		return -EIO;
	}

	return 0;
}

static void adc3101_setup_note_ret(int ret, int *first_ret, uint32_t *fail_count)
{
	if (ret >= 0) {
		return;
	}

	if ((first_ret != NULL) && (*first_ret == 0)) {
		*first_ret = ret;
	}
	if (fail_count != NULL) {
		(*fail_count)++;
	}
}

static void adc3101_setup_select_page(uint8_t *cur_page, uint8_t page,
				      int *first_ret, uint32_t *fail_count)
{
	int ret;

	if ((cur_page != NULL) && (*cur_page == page)) {
		return;
	}

	ret = adc3101_write_raw(ADC3101_REG_PAGE_SELECT, page, true);
	adc3101_setup_note_ret(ret, first_ret, fail_count);
	if (cur_page != NULL) {
		/* Some boards report a failed transaction even when the codec accepts it.
		 * Continue the script as if the page changed and let captured audio decide.
		 */
		*cur_page = page;
	}
}

static void adc3101_setup_write_page(uint8_t *cur_page, uint8_t page,
				     uint8_t reg, uint8_t val,
				     int *first_ret, uint32_t *fail_count)
{
	int ret;

	adc3101_setup_select_page(cur_page, page, first_ret, fail_count);
	ret = adc3101_write_raw(reg, val, true);
	adc3101_setup_note_ret(ret, first_ret, fail_count);
}

int adc3101_setup(bool *weak_verify)
{
	uint8_t page = 0xffU;
	int first_ret = 0;
	uint32_t fail_count = 0U;
	int verify_ret;

	if (weak_verify != NULL) {
		*weak_verify = false;
	}

#define ADC3101_SETUP_WRITE(_page, _reg, _val) \
	adc3101_setup_write_page(&page, (_page), (_reg), (_val), &first_ret, &fail_count)

	adc3101_setup_select_page(&page, 0U, &first_ret, &fail_count);
	adc3101_setup_note_ret(adc3101_write_raw(ADC3101_REG_RESET, 0x01, true),
			       &first_ret, &fail_count);
	k_msleep(5);
	page = 0xffU;

	ADC3101_SETUP_WRITE(0U, ADC3101_REG_CLKMUX, 0x00);
	ADC3101_SETUP_WRITE(0U, ADC3101_REG_PLL_P_R, 0x11);
	ADC3101_SETUP_WRITE(0U, ADC3101_REG_PLL_J, 0x04);
	ADC3101_SETUP_WRITE(0U, ADC3101_REG_PLL_D_MSB, 0x00);
	ADC3101_SETUP_WRITE(0U, ADC3101_REG_PLL_D_LSB, 0x00);
	ADC3101_SETUP_WRITE(0U, ADC3101_REG_NADC, 0x81);
	ADC3101_SETUP_WRITE(0U, ADC3101_REG_MADC, 0x82);
	ADC3101_SETUP_WRITE(0U, ADC3101_REG_AOSR, 0x80);
	ADC3101_SETUP_WRITE(0U, ADC3101_REG_AUDIO_IF, 0x00);
	ADC3101_SETUP_WRITE(0U, ADC3101_REG_PROCESSING_BLOCK, 0x01);

	ADC3101_SETUP_WRITE(ADC3101_PAGE_ANALOG, ADC3101_REG_MICBIAS, 0x78);

	LOG_INF("Program MicPGA: Left/Right = %u dB", ADC3101_MICPGA_GAIN_DB);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_ANALOG, ADC3101_REG_LEFT_PGA,
			    (uint8_t)(ADC3101_MICPGA_GAIN_DB * 2U));
	ADC3101_SETUP_WRITE(ADC3101_PAGE_ANALOG, ADC3101_REG_RIGHT_PGA,
			    (uint8_t)(ADC3101_MICPGA_GAIN_DB * 2U));
	ADC3101_SETUP_WRITE(ADC3101_PAGE_ANALOG, ADC3101_REG_LEFT_INPUT, 0xf3);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_ANALOG, 0x36, 0x7f);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_ANALOG, ADC3101_REG_RIGHT_INPUT, 0xf3);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_ANALOG, 0x39, 0x7f);

	ADC3101_SETUP_WRITE(0U, ADC3101_REG_ADC_POWER, 0xc2);
	k_msleep(10);

	ADC3101_SETUP_WRITE(0U, ADC3101_REG_ADC_FINE_VOLUME, 0x00);
	ADC3101_SETUP_WRITE(0U, ADC3101_REG_LEFT_VOLUME,
			    (uint8_t)(ADC3101_ADC_DIGITAL_GAIN_DB * 2U));
	ADC3101_SETUP_WRITE(0U, ADC3101_REG_RIGHT_VOLUME,
			    (uint8_t)(ADC3101_ADC_DIGITAL_GAIN_DB * 2U));

	/* Match the Arduino firmware's enabled ADC3101 high-pass configuration. */
	ADC3101_SETUP_WRITE(0U, 0x0d, 0x02);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x0e, 0x60);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x0f, 0xe8);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x10, 0x3e);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x11, 0x2f);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x12, 0x60);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x13, 0xe8);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x14, 0x45);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x15, 0xdd);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x16, 0x49);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x17, 0x7c);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x4e, 0x60);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x4f, 0xe8);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x50, 0x3e);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x51, 0x2f);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x52, 0x60);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x53, 0xe8);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x54, 0x45);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x55, 0xdd);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x56, 0x49);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x57, 0x7c);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x08, 0x6a);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x09, 0xc3);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x0a, 0x95);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x0b, 0x3d);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x0c, 0xaa);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x0d, 0x79);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x48, 0x6a);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x49, 0xc3);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x4a, 0x95);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x4b, 0x3d);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x4c, 0xaa);
	ADC3101_SETUP_WRITE(ADC3101_PAGE_FILTER, 0x4d, 0x79);

#undef ADC3101_SETUP_WRITE

	verify_ret = adc3101_verify_basic();
	if ((fail_count != 0U) || (verify_ret < 0)) {
		if (weak_verify != NULL) {
			*weak_verify = true;
		}
		LOG_WRN("ADC3101 best-effort setup weak: write_failures=%u first=%d verify=%d",
			(unsigned int)fail_count, first_ret, verify_ret);
		return ADC3101_SETUP_WEAK_VERIFY;
	}

	return ADC3101_SETUP_OK;
}

int adc3101_soft_powerdown(void)
{
	uint8_t page = 0xffU;
	int ret;
	int first_ret = 0;

	ret = adc3101_write_page(&page, 0U, ADC3101_REG_ADC_FINE_VOLUME, 0x88);
	if ((ret < 0) && (first_ret == 0)) {
		first_ret = ret;
	}

	ret = adc3101_write_page(&page, ADC3101_PAGE_ANALOG, ADC3101_REG_MICBIAS, 0x00);
	if ((ret < 0) && (first_ret == 0)) {
		first_ret = ret;
	}

	ret = adc3101_write_page(&page, 0U, ADC3101_REG_ADC_POWER, 0x02);
	if ((ret < 0) && (first_ret == 0)) {
		first_ret = ret;
	}

	if (first_ret < 0) {
		LOG_DBG("ADC3101 soft powerdown incomplete: %d", first_ret);
		return first_ret;
	}

	LOG_INF("Codec ADC soft-powered down");
	return 0;
}
