#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include "tasks.h"

LOG_MODULE_REGISTER(button_task, LOG_LEVEL_INF);

#if DT_NODE_HAS_STATUS(DT_ALIAS(sw0), okay)
static const struct gpio_dt_spec user_button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback button_cb_data;
static struct k_work_delayable button_release_work;

#define BUTTON_DEBOUNCE_MS 30

static bool button_press_seen;
static bool button_release_pending;

static bool button_is_pressed(void)
{
	int raw_level = gpio_pin_get(user_button.port, user_button.pin);
	if (raw_level < 0) {
		return false;
	}

	bool active_low = (user_button.dt_flags & GPIO_ACTIVE_LOW) != 0U;
	return active_low ? (raw_level == 0) : (raw_level != 0);
}

static void button_release_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!button_release_pending) {
		return;
	}

	if (button_is_pressed()) {
		return;
	}

	button_release_pending = false;

	if (button_press_seen) {
		button_press_seen = false;
		LOG_INF("Button released (debounced), requesting wakeup");
		power_fsm_request_wakeup(POWER_WAKE_SRC_BUTTON);
	}
}

static void user_button_isr(const struct device *dev,
			    struct gpio_callback *cb,
			    uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	if (button_is_pressed()) {
		button_press_seen = true;
		button_release_pending = true;
		return;
	}

	if (button_release_pending) {
		(void)k_work_reschedule(&button_release_work, K_MSEC(BUTTON_DEBOUNCE_MS));
	}
}

int task_button_init(void)
{
	if (!gpio_is_ready_dt(&user_button)) {
		LOG_ERR("Button GPIO not ready");
		return -ENODEV;
	}

#if IS_ENABLED(CONFIG_PM_DEVICE)
	bool wake_capable = pm_device_wakeup_is_capable(user_button.port);
	bool wake_enabled = false;
	if (wake_capable) {
		wake_enabled = pm_device_wakeup_enable(user_button.port, true);
	}
	LOG_INF("Button wakeup capability: capable=%d enabled=%d",
		wake_capable ? 1 : 0,
		wake_enabled ? 1 : 0);
#endif

	int ret = gpio_pin_configure_dt(&user_button, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Button configure failed: %d", ret);
		return ret;
	}

	k_work_init_delayable(&button_release_work, button_release_work_handler);

	ret = gpio_pin_interrupt_configure_dt(&user_button, GPIO_INT_EDGE_BOTH);
	if (ret < 0) {
		LOG_ERR("Button IRQ configure failed: %d", ret);
		return ret;
	}

	gpio_init_callback(&button_cb_data, user_button_isr, BIT(user_button.pin));
	ret = gpio_add_callback(user_button.port, &button_cb_data);
	if (ret < 0) {
		LOG_ERR("Button callback add failed: %d", ret);
		return ret;
	}

	LOG_INF("Button IRQ wakeup enabled on sw0 (trigger on release, debounce=%d ms)",
		BUTTON_DEBOUNCE_MS);
	return 0;
}
#else
int task_button_init(void)
{
	LOG_WRN("sw0 alias not found; button wakeup disabled");
	return -ENODEV;
}
#endif
