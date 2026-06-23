#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "tasks.h"

LOG_MODULE_REGISTER(status_led_task, LOG_LEVEL_INF);

#if DT_NODE_HAS_PROP(DT_ALIAS(led0), gpios)
static const struct gpio_dt_spec status_led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
#else
static const struct gpio_dt_spec status_led = {0};
#endif

struct status_led_pattern {
	uint8_t pulses;
	uint16_t on_ms;
	uint16_t off_ms;
};

static struct k_work_delayable status_led_work;
K_MUTEX_DEFINE(status_led_lock);

static bool status_led_ready;
static bool status_led_enabled;
static bool status_led_sleeping;
static bool status_led_on;
static uint8_t status_led_pulses_left;
static uint16_t status_led_on_ms;
static uint16_t status_led_off_ms;

static struct status_led_pattern status_led_pattern_for_event(status_led_event_t event)
{
	switch (event) {
	case STATUS_LED_BOOT_OK:
		return (struct status_led_pattern){ .pulses = 1U, .on_ms = 120U, .off_ms = 80U };
	case STATUS_LED_ACTIVE_ENTER:
		return (struct status_led_pattern){ .pulses = 1U, .on_ms = 40U, .off_ms = 40U };
	case STATUS_LED_CAPTURE_START:
		return (struct status_led_pattern){ .pulses = 1U, .on_ms = 60U, .off_ms = 40U };
	case STATUS_LED_CODEC_RECORDING_OK:
		return (struct status_led_pattern){ .pulses = 1U, .on_ms = 300U, .off_ms = 80U };
	case STATUS_LED_INFER_DONE:
		return (struct status_led_pattern){ .pulses = 1U, .on_ms = 80U, .off_ms = 60U };
	case STATUS_LED_UPLINK_READY:
		return (struct status_led_pattern){ .pulses = 2U, .on_ms = 70U, .off_ms = 90U };
	case STATUS_LED_GPS_WAIT:
		return (struct status_led_pattern){ .pulses = 2U, .on_ms = 120U, .off_ms = 180U };
	case STATUS_LED_LORA_OK:
		return (struct status_led_pattern){ .pulses = 2U, .on_ms = 50U, .off_ms = 70U };
	case STATUS_LED_ERROR:
	default:
		return (struct status_led_pattern){ .pulses = 3U, .on_ms = 50U, .off_ms = 70U };
	}
}

static void status_led_drive(bool on)
{
	if (!status_led_ready) {
		return;
	}

	(void)gpio_pin_set_dt(&status_led, on ? 1 : 0);
	app_state.led_state = on;
}

static void status_led_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	bool drive_on = false;
	bool schedule_next = false;
	uint16_t delay_ms = 0U;

	k_mutex_lock(&status_led_lock, K_FOREVER);

	if (!status_led_ready || !status_led_enabled || status_led_sleeping) {
		status_led_on = false;
		status_led_pulses_left = 0U;
		k_mutex_unlock(&status_led_lock);
		status_led_drive(false);
		return;
	}

	if (status_led_on) {
		status_led_on = false;
		if (status_led_pulses_left > 0U) {
			status_led_pulses_left--;
		}
		if (status_led_pulses_left > 0U) {
			schedule_next = true;
			delay_ms = status_led_off_ms;
		}
	} else if (status_led_pulses_left > 0U) {
		status_led_on = true;
		drive_on = true;
		schedule_next = true;
		delay_ms = status_led_on_ms;
	}

	k_mutex_unlock(&status_led_lock);

	status_led_drive(drive_on);
	if (schedule_next) {
		(void)k_work_reschedule(&status_led_work, K_MSEC(delay_ms));
	}
}

int task_status_led_init(void)
{
	if (!IS_ENABLED(CONFIG_APP_STATUS_LED)) {
		return 0;
	}

	if (!status_led.port || !gpio_is_ready_dt(&status_led)) {
		LOG_WRN("status LED led0 not ready; LED indications disabled");
		return -ENODEV;
	}

	int ret = gpio_pin_configure_dt(&status_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_WRN("status LED configure failed: %d", ret);
		return ret;
	}

	k_work_init_delayable(&status_led_work, status_led_work_handler);

	k_mutex_lock(&status_led_lock, K_FOREVER);
	status_led_ready = true;
	status_led_enabled = true;
	status_led_sleeping = false;
	status_led_on = false;
	status_led_pulses_left = 0U;
	k_mutex_unlock(&status_led_lock);

	app_state.led_state = false;
	LOG_INF("status LED initialized on %s pin %u",
		status_led.port->name,
		(unsigned int)status_led.pin);
	return 0;
}

void task_status_led_event(status_led_event_t event)
{
	if (!IS_ENABLED(CONFIG_APP_STATUS_LED)) {
		return;
	}

	struct status_led_pattern pattern = status_led_pattern_for_event(event);
	if (pattern.pulses == 0U) {
		return;
	}

	k_mutex_lock(&status_led_lock, K_FOREVER);

	if (!status_led_ready || !status_led_enabled) {
		k_mutex_unlock(&status_led_lock);
		return;
	}

	if ((event == STATUS_LED_ACTIVE_ENTER) || (event == STATUS_LED_BOOT_OK)) {
		status_led_sleeping = false;
	}

	if (status_led_sleeping) {
		k_mutex_unlock(&status_led_lock);
		return;
	}

	status_led_on = true;
	status_led_pulses_left = pattern.pulses;
	status_led_on_ms = pattern.on_ms;
	status_led_off_ms = pattern.off_ms;

	k_mutex_unlock(&status_led_lock);

	status_led_drive(true);
	(void)k_work_reschedule(&status_led_work, K_MSEC(pattern.on_ms));
}

void task_status_led_prepare_sleep(void)
{
	if (!IS_ENABLED(CONFIG_APP_STATUS_LED)) {
		return;
	}

	k_mutex_lock(&status_led_lock, K_FOREVER);
	status_led_sleeping = true;
	status_led_on = false;
	status_led_pulses_left = 0U;
	k_mutex_unlock(&status_led_lock);

	(void)k_work_cancel_delayable(&status_led_work);
	status_led_drive(false);
}

void task_status_led_set_enabled(bool enabled)
{
	if (!IS_ENABLED(CONFIG_APP_STATUS_LED)) {
		return;
	}

	k_mutex_lock(&status_led_lock, K_FOREVER);
	status_led_enabled = enabled;
	if (!enabled) {
		status_led_on = false;
		status_led_pulses_left = 0U;
	}
	k_mutex_unlock(&status_led_lock);

	if (!enabled) {
		(void)k_work_cancel_delayable(&status_led_work);
		status_led_drive(false);
	}
}
