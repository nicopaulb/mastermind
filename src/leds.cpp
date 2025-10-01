#include <errno.h>
#include <string.h>
#include "etl/algorithm.h"

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(leds);

#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

#include "leds.hpp"

static const struct led_rgb code_colors[] = {
    RGB(0xFF, 0xFF, 0xFF), // white
    RGB(0xFF, 0x00, 0x00), // red
    RGB(0x00, 0xFF, 0x00), // green
    RGB(0x00, 0x00, 0xFF), // blue
    RGB(0xFF, 0xFF, 0x00), // yellow
    RGB(0xFF, 0x00, 0xFF)  // magenta
};

bool led_strip::init(void)
{
    LOG_INF("Initializing LEDs");
    if (!device_is_ready(strip))
    {
        LOG_ERR("Device is not ready");
        return false;
    }
    return true;
}

void led_strip::update_combination(combination &combi)
{
    for (int i = 0; i < combi.values.size(); i++)
    {
        leds[i] = code_colors[int(combi.values[i].value)];
    }
}

void led_strip::update_clues()
{
    for (int i = 0; i < SLOT_NB; i++)
    {
        leds[i + SLOT_NB] = code_colors[0];
    }
}

void led_strip::refresh(void)
{
    LOG_INF("Refreshing LEDs on strip");
    int rc = led_strip_update_rgb(strip, leds.data(), STRIP_NUM_LEDS);
    if (rc)
    {
        LOG_ERR("Couldn't update strip: %d", rc);
    }
}
