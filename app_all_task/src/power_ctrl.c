#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
/* #include <zephyr/drivers/regulator.h> */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_SOC_SERIES_STM32U5X)
#include <stm32_ll_pwr.h>
#include <gpio/gpio_stm32.h>
#endif

#include "tasks.h"

LOG_MODULE_REGISTER(power_ctrl, LOG_LEVEL_INF);

/*
#if DT_NODE_HAS_STATUS(DT_NODELABEL(v_periph), okay)
static const struct device *const vperiph_dev = DEVICE_DT_GET(DT_NODELABEL(v_periph));
#else
static const struct device *const vperiph_dev;
#endif
*/

#if DT_NODE_HAS_STATUS(DT_NODELABEL(v_periph), okay)
static const struct gpio_dt_spec vperiph_en =
	GPIO_DT_SPEC_GET(DT_NODELABEL(v_periph), enable_gpios);
#else
static const struct gpio_dt_spec vperiph_en = {0};
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gps_on_off), okay)
static const struct gpio_dt_spec gps_en = GPIO_DT_SPEC_GET(DT_NODELABEL(gps_on_off), gpios);
#else
static const struct gpio_dt_spec gps_en = {0};
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gps_vbckp), okay)
static const struct gpio_dt_spec gps_vbckp = GPIO_DT_SPEC_GET(DT_NODELABEL(gps_vbckp), gpios);
#else
static const struct gpio_dt_spec gps_vbckp = {0};
#endif

#if DT_NODE_HAS_PROP(DT_ALIAS(led0), gpios)
static const struct gpio_dt_spec led0_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
#else
static const struct gpio_dt_spec led0_gpio = {0};
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(power_ctrl_pins), spi3_sck_gpios)
static const struct gpio_dt_spec spi3_sck_gpio =
	GPIO_DT_SPEC_GET(DT_NODELABEL(power_ctrl_pins), spi3_sck_gpios);
#else
static const struct gpio_dt_spec spi3_sck_gpio = {0};
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(power_ctrl_pins), spi3_mosi_gpios)
static const struct gpio_dt_spec spi3_mosi_gpio =
	GPIO_DT_SPEC_GET(DT_NODELABEL(power_ctrl_pins), spi3_mosi_gpios);
#else
static const struct gpio_dt_spec spi3_mosi_gpio = {0};
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(spi3), cs_gpios)
static const struct gpio_dt_spec spi3_cs_lora =
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(spi3), cs_gpios, 0);
static const struct gpio_dt_spec spi3_cs_sd =
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(spi3), cs_gpios, 1);
#else
static const struct gpio_dt_spec spi3_cs_lora = {0};
static const struct gpio_dt_spec spi3_cs_sd = {0};
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lora), okay)
static const struct gpio_dt_spec lora_reset =
	GPIO_DT_SPEC_GET(DT_NODELABEL(lora), reset_gpios);
static const struct gpio_dt_spec lora_busy =
	GPIO_DT_SPEC_GET(DT_NODELABEL(lora), busy_gpios);
static const struct gpio_dt_spec lora_rxen =
	GPIO_DT_SPEC_GET(DT_NODELABEL(lora), rx_enable_gpios);
static const struct gpio_dt_spec lora_dio1 =
	GPIO_DT_SPEC_GET(DT_NODELABEL(lora), dio1_gpios);
#else
static const struct gpio_dt_spec lora_reset = {0};
static const struct gpio_dt_spec lora_busy = {0};
static const struct gpio_dt_spec lora_rxen = {0};
static const struct gpio_dt_spec lora_dio1 = {0};
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi3), okay) && defined(PINCTRL_STATE_SLEEP)
PINCTRL_DT_DEFINE(DT_NODELABEL(spi3));
static const struct pinctrl_dev_config *const spi3_pcfg =
	PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(spi3));
#else
static const struct pinctrl_dev_config *const spi3_pcfg;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay) && defined(PINCTRL_STATE_SLEEP)
PINCTRL_DT_DEFINE(DT_NODELABEL(i2c1));
static const struct pinctrl_dev_config *const i2c1_pcfg =
	PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(i2c1));
#else
static const struct pinctrl_dev_config *const i2c1_pcfg;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c2), okay) && defined(PINCTRL_STATE_SLEEP)
PINCTRL_DT_DEFINE(DT_NODELABEL(i2c2));
static const struct pinctrl_dev_config *const i2c2_pcfg =
	PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(i2c2));
#else
static const struct pinctrl_dev_config *const i2c2_pcfg;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sai1_b), okay) && defined(PINCTRL_STATE_SLEEP)
PINCTRL_DT_DEFINE(DT_NODELABEL(sai1_b));
static const struct pinctrl_dev_config *const sai1b_pcfg =
	PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(sai1_b));
#else
static const struct pinctrl_dev_config *const sai1b_pcfg;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi3), okay)
static const struct device *const spi3_dev = DEVICE_DT_GET(DT_NODELABEL(spi3));
#else
static const struct device *const spi3_dev;
#endif

static bool vperiph_io_parked;
static bool spi3_pm_suspended;

static void power_ctrl_try_apply_sleep_pinctrl(const struct pinctrl_dev_config *pcfg,
					 const char *name);
static void power_ctrl_try_apply_default_pinctrl(const struct pinctrl_dev_config *pcfg,
					   const char *name);

/**
 * @brief Suspend SPI3 through PM API when available.
 *
 * Uses a guarded fallback path so SPI3 can enter low-power mode even when
 * pinctrl sleep state alone is not sufficient.
 */
static void power_ctrl_spi3_suspend_fallback(void)
{
#if IS_ENABLED(CONFIG_PM_DEVICE)
	if ((spi3_dev == NULL) || !device_is_ready(spi3_dev)) {
		return;
	}

	if (spi3_pm_suspended) {
		return;
	}

	int ret = pm_device_action_run(spi3_dev, PM_DEVICE_ACTION_SUSPEND);
	if ((ret == 0) || (ret == -EALREADY)) {
		spi3_pm_suspended = true;
		LOG_DBG("spi3 PM suspend fallback applied");
	} else {
		LOG_DBG("spi3 PM suspend fallback failed: %d", ret);
	}
#endif
}

/**
 * @brief Resume SPI3 through PM API after fallback suspend.
 *
 * Resumes only when the fallback suspend path previously marked SPI3 as
 * suspended.
 */
static void power_ctrl_spi3_resume_fallback(void)
{
#if IS_ENABLED(CONFIG_PM_DEVICE)
	if ((spi3_dev == NULL) || !device_is_ready(spi3_dev)) {
		return;
	}

	if (!spi3_pm_suspended) {
		return;
	}

	int ret = pm_device_action_run(spi3_dev, PM_DEVICE_ACTION_RESUME);
	if ((ret == 0) || (ret == -EALREADY)) {
		spi3_pm_suspended = false;
		LOG_DBG("spi3 PM resume fallback applied");
	} else {
		LOG_DBG("spi3 PM resume fallback failed: %d", ret);
	}
#endif
}

/**
 * @brief Put a GPIO pin into disconnected (high-Z) mode.
 *
 * @param spec GPIO specification to update.
 * @param name Human-readable GPIO name used for logging.
 */
static void power_ctrl_gpio_disconnect(const struct gpio_dt_spec *spec, const char *name)
{
	if ((spec == NULL) || (spec->port == NULL) || !gpio_is_ready_dt(spec)) {
		return;
	}

	int ret = gpio_pin_configure_dt(spec, GPIO_DISCONNECTED);
	if (ret < 0) {
		LOG_DBG("%s disconnect failed: %d", name, ret);
	}
}

/**
 * @brief Drive a GPIO pin low as a push-pull output.
 *
 * @param spec GPIO specification to update.
 * @param name Human-readable GPIO name used for logging.
 */
static void power_ctrl_gpio_force_low(const struct gpio_dt_spec *spec, const char *name)
{
	if ((spec == NULL) || (spec->port == NULL) || !device_is_ready(spec->port)) {
		return;
	}

	int ret = gpio_pin_configure(spec->port, spec->pin, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_DBG("%s force low failed: %d", name, ret);
	}
}

/**
 * @brief Drive a GPIO low and configure STM32U5 low-power pull-down hold.
 *
 * @param spec GPIO specification to update.
 * @param name Human-readable GPIO name used for logging.
 */
static void power_ctrl_gpio_force_low_and_hold_lp(const struct gpio_dt_spec *spec,
						   const char *name)
{
	power_ctrl_gpio_force_low(spec, name);

#if defined(CONFIG_SOC_SERIES_STM32U5X)
	if ((spec == NULL) || (spec->port == NULL) || !device_is_ready(spec->port)) {
		return;
	}

	const struct gpio_stm32_config *cfg = spec->port->config;
	if (cfg == NULL) {
		return;
	}

	uint32_t pwr_port = ((uint32_t)LL_PWR_GPIO_PORTA) + (cfg->port * 8U);
	uint32_t pin_mask = BIT(spec->pin);

	LL_PWR_EnablePUPDConfig();
	LL_PWR_DisableGPIOPullUp(pwr_port, pin_mask);
	LL_PWR_EnableGPIOPullDown(pwr_port, pin_mask);

	LOG_DBG("%s LP hold configured: port=%u pin=%u pulldown=1",
		name,
		(unsigned int)cfg->port,
		(unsigned int)spec->pin);
#else
	ARG_UNUSED(spec);
	ARG_UNUSED(name);
#endif
}

/**
 * @brief Restore a GPIO pin as input mode.
 *
 * @param spec GPIO specification to update.
 * @param name Human-readable GPIO name used for logging.
 */
static void power_ctrl_gpio_restore_input(const struct gpio_dt_spec *spec, const char *name)
{
	if ((spec == NULL) || (spec->port == NULL) || !gpio_is_ready_dt(spec)) {
		return;
	}

	int ret = gpio_pin_configure_dt(spec, GPIO_INPUT);
	if (ret < 0) {
		LOG_DBG("%s input restore failed: %d", name, ret);
	}
}

/**
 * @brief Restore a GPIO pin as inactive output mode.
 *
 * @param spec GPIO specification to update.
 * @param name Human-readable GPIO name used for logging.
 */
static void power_ctrl_gpio_restore_output_inactive(const struct gpio_dt_spec *spec,
						    const char *name)
{
	if ((spec == NULL) || (spec->port == NULL) || !gpio_is_ready_dt(spec)) {
		return;
	}

	int ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_DBG("%s output restore failed: %d", name, ret);
	}
}

/**
 * @brief Park v_periph-related GPIOs into low-leakage states.
 *
 * Applies strong-low parking on selected lines and disconnects selected input
 * lines to reduce leakage during suspend.
 */
static void power_ctrl_park_vperiph_gpio(void)
{
	/* Arduino-style strong-low parking for known leakage paths.
	 * Keep the old disconnect strategy below as commented reference.
	 */
	power_ctrl_gpio_force_low_and_hold_lp(&led0_gpio, "led0");
	power_ctrl_gpio_force_low_and_hold_lp(&spi3_sck_gpio, "spi3_sck");
	power_ctrl_gpio_force_low_and_hold_lp(&spi3_mosi_gpio, "spi3_mosi");
	power_ctrl_gpio_force_low_and_hold_lp(&spi3_cs_lora, "spi3_cs_lora");
	power_ctrl_gpio_force_low_and_hold_lp(&spi3_cs_sd, "spi3_cs_sd");
	power_ctrl_gpio_force_low_and_hold_lp(&lora_reset, "lora_reset");
	power_ctrl_gpio_force_low_and_hold_lp(&lora_rxen, "lora_rxen");

	/* Previous high-Z parking kept for comparison/debugging.
	power_ctrl_gpio_disconnect(&spi3_cs_lora, "spi3_cs_lora");
	power_ctrl_gpio_disconnect(&spi3_cs_sd, "spi3_cs_sd");
	power_ctrl_gpio_disconnect(&lora_reset, "lora_reset");
	power_ctrl_gpio_disconnect(&lora_busy, "lora_busy");
	power_ctrl_gpio_disconnect(&lora_rxen, "lora_rxen");
	power_ctrl_gpio_disconnect(&lora_dio1, "lora_dio1");
	*/

	power_ctrl_gpio_disconnect(&lora_busy, "lora_busy");
	power_ctrl_gpio_disconnect(&lora_dio1, "lora_dio1");
}

/**
 * @brief Restore v_periph-related GPIOs to their runtime directions.
 *
 * Multiplexed peripheral pins are restored by pinctrl default state, not by
 * direct GPIO mode changes.
 */
static void power_ctrl_restore_vperiph_gpio(void)
{
	power_ctrl_gpio_restore_output_inactive(&led0_gpio, "led0");
	/* spi3_sck/spi3_mosi are muxed peripheral pins.
	 * They may be forced low during parking, but must be restored by
	 * spi3 default pinctrl instead of GPIO reconfiguration.
	 */
	/* power_ctrl_gpio_restore_output_inactive(&spi3_sck_gpio, "spi3_sck"); */
	/* power_ctrl_gpio_restore_output_inactive(&spi3_mosi_gpio, "spi3_mosi"); */
	power_ctrl_gpio_restore_output_inactive(&spi3_cs_lora, "spi3_cs_lora");
	power_ctrl_gpio_restore_output_inactive(&spi3_cs_sd, "spi3_cs_sd");
	power_ctrl_gpio_restore_output_inactive(&lora_reset, "lora_reset");
	power_ctrl_gpio_restore_input(&lora_busy, "lora_busy");
	power_ctrl_gpio_restore_output_inactive(&lora_rxen, "lora_rxen");
	power_ctrl_gpio_restore_input(&lora_dio1, "lora_dio1");
}

/**
 * @brief Restore only I2C1 default pinctrl state.
 */
static void power_ctrl_restore_i2c1_only(void)
{
	power_ctrl_try_apply_default_pinctrl(i2c1_pcfg, "i2c1");
}

/**
 * @brief Restore only I2C2 default pinctrl state.
 */
static void power_ctrl_restore_i2c2_only(void)
{
	power_ctrl_try_apply_default_pinctrl(i2c2_pcfg, "i2c2");
}

/**
 * @brief Restore all parked v_periph IO resources for normal runtime.
 *
 * Restores device PM state, pinctrl default states, and GPIO directions, then
 * clears the parked flag.
 */
static void power_ctrl_restore_full_vperiph_io(void)
{
	if (!vperiph_io_parked) {
		return;
	}

	power_ctrl_spi3_resume_fallback();
	power_ctrl_try_apply_default_pinctrl(spi3_pcfg, "spi3");
	power_ctrl_try_apply_default_pinctrl(i2c2_pcfg, "i2c2");
	power_ctrl_try_apply_default_pinctrl(i2c1_pcfg, "i2c1");
	power_ctrl_try_apply_default_pinctrl(sai1b_pcfg, "sai1_b");
	power_ctrl_restore_vperiph_gpio();
	vperiph_io_parked = false;
}

/**
 * @brief Common helper to enable v_periph with selective IO restoration.
 *
 * @param restore_full_io Restore all parked IO resources when true.
 * @param restore_i2c1_only Restore only I2C1 pinctrl when true.
 * @param restore_i2c2_only Restore only I2C2 pinctrl when true.
 * @return 0 on success, negative errno on failure.
 */
static int power_ctrl_vperiph_enable_common(bool restore_full_io, bool restore_i2c1_only,
					   bool restore_i2c2_only)
{
	if (!vperiph_en.port || !gpio_is_ready_dt(&vperiph_en)) {
		LOG_WRN("v_periph enable GPIO not ready");
		return -ENODEV;
	}

	if (restore_full_io) {
		power_ctrl_restore_full_vperiph_io();
	} else {
		if (restore_i2c1_only) {
			power_ctrl_restore_i2c1_only();
		}
		if (restore_i2c2_only) {
			power_ctrl_restore_i2c2_only();
		}
	}

	int ret = gpio_pin_configure_dt(&vperiph_en, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("v_periph GPIO configure failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&vperiph_en, 1);
	if (ret < 0) {
		LOG_ERR("v_periph GPIO set HIGH failed: %d", ret);
		return ret;
	}

	/* Codec (ADC3101) 等外设在上电后需要更长一点时间稳定，避免刚上电即访问 I2C */
	k_msleep(250);

	LOG_INF("v_periph enabled");
	return 0;
}

/**
 * @brief Disable GPS backup supply by forcing VBCKP low.
 */
static void power_ctrl_gps_vbckp_low(void)
{
	power_ctrl_gpio_force_low_and_hold_lp(&gps_vbckp, "gps_vbckp");
}

/**
 * @brief Enable GPS backup supply by driving VBCKP high.
 */
static void power_ctrl_gps_vbckp_high(void)
{
	if (!gps_vbckp.port || !gpio_is_ready_dt(&gps_vbckp)) {
		return;
	}

	int ret = gpio_pin_configure_dt(&gps_vbckp, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_DBG("gps_vbckp configure high failed: %d", ret);
		return;
	}

	ret = gpio_pin_set_dt(&gps_vbckp, 1);
	if (ret < 0) {
		LOG_DBG("gps_vbckp set high failed: %d", ret);
	}
}

/**
 * @brief Apply the sleep pinctrl state for a peripheral when available.
 *
 * @param pcfg Pinctrl device configuration.
 * @param name Human-readable peripheral name used for logging.
 */
static void power_ctrl_try_apply_sleep_pinctrl(const struct pinctrl_dev_config *pcfg,
					 const char *name)
{
	if (pcfg == NULL) {
		return;
	}

	int ret = pinctrl_apply_state(pcfg, PINCTRL_STATE_SLEEP);
	if (ret < 0) {
		LOG_DBG("%s sleep pinctrl not applied: %d", name, ret);
	}
}

/**
 * @brief Apply the default pinctrl state for a peripheral when available.
 *
 * @param pcfg Pinctrl device configuration.
 * @param name Human-readable peripheral name used for logging.
 */
static void power_ctrl_try_apply_default_pinctrl(const struct pinctrl_dev_config *pcfg,
					   const char *name)
{
	if (pcfg == NULL) {
		return;
	}

	int ret = pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_DBG("%s default pinctrl not applied: %d", name, ret);
	}
}

/**
 * @brief Park all v_periph-related IO resources for low-power operation.
 *
 * Applies SPI3 fallback suspend, GPIO parking, and peripheral sleep pinctrl.
 */
static void power_ctrl_park_vperiph_io(void)
{
	if (vperiph_io_parked) {
		return;
	}

	power_ctrl_spi3_suspend_fallback();
	power_ctrl_park_vperiph_gpio();
	power_ctrl_try_apply_sleep_pinctrl(sai1b_pcfg, "sai1_b");
	power_ctrl_try_apply_sleep_pinctrl(i2c1_pcfg, "i2c1");
	power_ctrl_try_apply_sleep_pinctrl(i2c2_pcfg, "i2c2");
	power_ctrl_try_apply_sleep_pinctrl(spi3_pcfg, "spi3");
	vperiph_io_parked = true;
}

/**
 * @brief Enable v_periph and restore full runtime IO configuration.
 *
 * @return 0 on success, negative errno on failure.
 */
int power_ctrl_vperiph_on(void)
{
	return power_ctrl_vperiph_enable_common(true, false, false);
}

/**
 * @brief Enable v_periph and restore only resources needed by I2C1.
 *
 * @return 0 on success, negative errno on failure.
 */
int power_ctrl_vperiph_on_for_i2c1(void)
{
	return power_ctrl_vperiph_enable_common(false, true, false);
}

/**
 * @brief Enable v_periph and restore only resources needed by I2C2.
 *
 * @return 0 on success, negative errno on failure.
 */
int power_ctrl_vperiph_on_for_i2c2(void)
{
	return power_ctrl_vperiph_enable_common(false, false, true);
}

/**
 * @brief Disable v_periph and park related IO to minimize leakage.
 *
 * @return 0 on success, negative errno on failure.
 */
int power_ctrl_vperiph_off(void)
{
	/*
	if ((vperiph_dev == NULL) || !device_is_ready(vperiph_dev)) {
		LOG_WRN("v_periph regulator not ready");
		return -ENODEV;
	}

	int ret = regulator_disable(vperiph_dev);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("v_periph disable failed: %d", ret);
		return ret;
	}
	*/

	if (!vperiph_en.port || !gpio_is_ready_dt(&vperiph_en)) {
		LOG_WRN("v_periph enable GPIO not ready");
		return -ENODEV;
	}

	power_ctrl_park_vperiph_io();
	power_ctrl_gpio_force_low_and_hold_lp(&vperiph_en, "v_periph_en");

	LOG_INF("v_periph disabled");
	return 0;
}

/**
 * @brief Power on GPS main and backup supplies.
 *
 * Ensures v_periph is enabled for required interfaces before asserting GPS
 * main power.
 *
 * @return 0 on success, negative errno on failure.
 */
int power_ctrl_gps_on(void)
{
	int ret = power_ctrl_vperiph_on_for_i2c1();
	if ((ret < 0) && (ret != -ENODEV)) {
		LOG_ERR("v_periph enable before GPS main power failed: %d", ret);
		return ret;
	}

	power_ctrl_gps_vbckp_high();
	LOG_INF("GPS backup power enabled");

	if (!gps_en.port || !gpio_is_ready_dt(&gps_en)) {
		LOG_WRN("gps_on_off GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&gps_en, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("gps_on_off configure failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&gps_en, 1);
	if (ret < 0) {
		LOG_ERR("gps_on_off set HIGH failed: %d", ret);
		return ret;
	}

	LOG_INF("GPS main power enabled");
	return 0;
}

/**
 * @brief Power off GPS main supply and disable backup supply.
 *
 * @return Always returns 0.
 */
int power_ctrl_gps_off(void)
{
	power_ctrl_gpio_force_low_and_hold_lp(&gps_en, "gps_on_off");
	power_ctrl_gps_vbckp_low();
	LOG_INF("GPS main power disabled");
	return 0;
}

/**
 * @brief Prepare all dependent tasks and peripherals for suspend.
 *
 * @return 0 on success, negative errno on failure.
 */
int power_ctrl_prepare_suspend(void)
{
	task_storage_prepare_sleep();
	task_microphone_prepare_sleep();
	task_inference_prepare_sleep();
	task_lorawan_prepare_sleep();
	task_gps_prepare_sleep();

	int ret = power_ctrl_vperiph_off();
	if ((ret < 0) && (ret != -ENODEV)) {
		return ret;
	}

	return 0;
}

/**
 * @brief Prepare the system for deep sleep with all major peripherals off.
 *
 * @return Always returns 0.
 */
int power_ctrl_prepare_deep_sleep_all(void)
{
	power_ctrl_prepare_suspend();
	power_ctrl_gps_vbckp_low();
	task_sound_wakeup_prepare_sleep();
	return 0;
}
