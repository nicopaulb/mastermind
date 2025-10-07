#include <zephyr/logging/log.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include "buttons.hpp"

#define LOG_LEVEL 4

LOG_MODULE_REGISTER(buttons);

static struct k_poll_signal signal;

/**
 * @brief Raise a signal based on the pressed button.
 *
 * @param dev The device that triggered the callback.
 * @param cb The callback structure.
 * @param pins The pin number of the pressed button.
 */
static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_poll_signal_raise(&signal, pins);
}

/**
 * @brief Wait for a button press event with a given timeout.
 *
 * @param timeout The timeout in milliseconds.
 * @return The button value of the pressed button, or BUTTON_VAL_NONE if no event was received.
 */
button_val buttons::wait_for_input(k_timeout_t timeout)
{
	unsigned int signaled;
	int result;
	int index = 0;
	button_val ret = button_val::BUTTON_VAL_NONE;
	struct k_poll_event events[1] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
								 K_POLL_MODE_NOTIFY_ONLY,
								 &signal),
	};

	k_poll(events, 1, timeout);
	k_poll_signal_check(&signal, &signaled, &result);

	if (signaled)
	{
		for (const auto &i : specs)
		{
			if (BIT(i.pin) == result)
			{
				ret = static_cast<button_val>(index);
				break;
			}
			index++;
		}
	}
	else
	{
		ret = button_val::BUTTON_VAL_NONE;
	}

	k_poll_signal_reset(&signal);
	events[0].state = K_POLL_STATE_NOT_READY;
	return ret;
}

/**
 * @brief Initialise the buttons module.
 *
 * @return true if the initialization was successful, false otherwise.
 */
bool buttons::init(void)
{
	int ret;
	gpio_port_pins_t pin_mask;

	LOG_INF("Initializing buttons");

	// All buttons are on the same port
	if (!device_is_ready(specs[0].port))
	{
		LOG_ERR("Device is not ready");
		return false;
	}

	for (const auto &i : specs)
	{
		ret = gpio_pin_configure_dt(&i, GPIO_INPUT);
		if (ret < 0)
		{
			LOG_ERR("Couldn't configure pin: %d", ret);
			return false;
		}

		ret = gpio_pin_interrupt_configure_dt(&i, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0)
		{
			LOG_ERR("Couldn't configure pin interrupt: %d", ret);
			return false;
		}

		WRITE_BIT(pin_mask, i.pin, 1);
	}

	// Initialize polling signal
	k_poll_signal_init(&signal);

	// Adding interrupt callback
	gpio_init_callback(&cb_data, button_pressed, pin_mask);
	ret = gpio_add_callback_dt(&specs[0], &cb_data);
	if (ret < 0)
	{
		LOG_ERR("Couldn't add callback: %d", ret);
		return false;
	}

	return true;
}