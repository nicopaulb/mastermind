#include <errno.h>
#include <string.h>

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

#include "combination.hpp"
#include "leds.hpp"
#include "buttons.hpp"

static led_strip leds;
static buttons buts;

int main(void)
{
	combination code;
	combination tentative;

	if (!buts.init())
	{
		return 1;
	}

	if (!leds.init())
	{
		return 1;
	}

	code.random_fill();

	leds.update_combination(code);
	leds.update_clues();
	leds.refresh();
	struct k_poll_signal *signal = buttons::get_signal();
	struct k_poll_event events[1] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
								 K_POLL_MODE_NOTIFY_ONLY,
								 signal),
	};

	while (1)
	{
		k_poll(events, 1, K_FOREVER);
		unsigned int signaled;
		int result;
		k_poll_signal_check(signal, &signaled, &result);

		if (signaled && (result == 0x1337))
		{
			LOG_INF("Buttons event");
		}
	}

	return 1;
}
