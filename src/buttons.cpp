#include <errno.h>
#include <string.h>

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(buttons);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include "buttons.hpp"
#include "combination.hpp"

static struct k_poll_signal signal;

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_poll_signal_raise(&signal, 0x1337);
}

struct k_poll_signal *buttons::get_signal(void)
{
	return &signal;
}

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