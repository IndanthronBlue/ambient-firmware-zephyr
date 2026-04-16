#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include "tasks.h"

LOG_MODULE_REGISTER(sound_wakeup_task, LOG_LEVEL_INF);

#if DT_NODE_HAS_STATUS(DT_ALIAS(sound_wakeup_en), okay) && DT_NODE_HAS_STATUS(DT_ALIAS(sound_wakeup_irq), okay)
static const struct gpio_dt_spec sound_en_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(sound_wakeup_en), gpios);
static const struct gpio_dt_spec sound_irq_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(sound_wakeup_irq), gpios);
static struct gpio_callback sound_irq_cb_data;

static void sound_irq_isr(const struct device *dev,
			 struct gpio_callback *cb,
			 uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	power_fsm_request_wakeup(POWER_WAKE_SRC_SOUND);
}

int task_sound_wakeup_init(void)
{
	if (!gpio_is_ready_dt(&sound_en_gpio) || !gpio_is_ready_dt(&sound_irq_gpio)) {
		LOG_ERR("Sound wake GPIO not ready (EN/IRQ)");
		return -ENODEV;
	}

	int ret = gpio_pin_configure_dt(&sound_en_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Sound EN configure failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&sound_en_gpio, 1);
	if (ret < 0) {
		LOG_ERR("Sound EN set failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&sound_irq_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Sound IRQ configure failed: %d", ret);
		return ret;
	}

#if IS_ENABLED(CONFIG_PM_DEVICE)
	bool wake_capable = pm_device_wakeup_is_capable(sound_irq_gpio.port);
	bool wake_enabled = false;
	if (wake_capable) {
		wake_enabled = pm_device_wakeup_enable(sound_irq_gpio.port, true);
	}
	LOG_INF("Sound IRQ wakeup capability: capable=%d enabled=%d",
		wake_capable ? 1 : 0,
		wake_enabled ? 1 : 0);
#endif

	ret = gpio_pin_interrupt_configure_dt(&sound_irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Sound IRQ interrupt configure failed: %d", ret);
		return ret;
	}

	gpio_init_callback(&sound_irq_cb_data, sound_irq_isr, BIT(sound_irq_gpio.pin));
	ret = gpio_add_callback(sound_irq_gpio.port, &sound_irq_cb_data);
	if (ret < 0) {
		LOG_ERR("Sound IRQ callback add failed: %d", ret);
		return ret;
	}

	LOG_INF("Sound wakeup ready: EN=%s pin=%u set=1, IRQ=%s pin=%u edge=to_active",
		sound_en_gpio.port->name,
		(unsigned int)sound_en_gpio.pin,
		sound_irq_gpio.port->name,
		(unsigned int)sound_irq_gpio.pin);

	return 0;
}

void task_sound_wakeup_prepare_sleep(void)
{
	if (sound_irq_gpio.port && gpio_is_ready_dt(&sound_irq_gpio)) {
		(void)gpio_pin_interrupt_configure_dt(&sound_irq_gpio, GPIO_INT_DISABLE);
		(void)gpio_pin_configure_dt(&sound_irq_gpio, GPIO_DISCONNECTED);
	}

	if (sound_en_gpio.port && gpio_is_ready_dt(&sound_en_gpio)) {
		(void)gpio_pin_set_dt(&sound_en_gpio, 0);
		(void)gpio_pin_configure_dt(&sound_en_gpio, GPIO_OUTPUT_INACTIVE);
	}

	LOG_INF("Sound wakeup sleep prep: IRQ disabled+disconnected, EN=0");
}
#else
int task_sound_wakeup_init(void)
{
	LOG_WRN("sound-wakeup-en/sound-wakeup-irq alias not found; sound wakeup disabled");
	return -ENODEV;
}

void task_sound_wakeup_prepare_sleep(void)
{
}
#endif
