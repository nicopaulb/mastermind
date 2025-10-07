#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

#include "leds.hpp"

#define LOG_LEVEL 4

LOG_MODULE_REGISTER(leds);

#define LED_OFF RGB(0x00, 0x00, 0x00)
#define LED_CORRECT RGB(0x00, 0xFF, 0x00)
#define LED_WRONG_POS RGB(0xFF, 0xFF, 0x00)

static const struct led_rgb code_colors[] = {
    RGB(0xFF, 0xFF, 0xFF), // white
    RGB(0xFF, 0x00, 0x00), // red
    RGB(0x00, 0xFF, 0x00), // green
    RGB(0x00, 0x00, 0xFF), // blue
    RGB(0xFF, 0xFF, 0x00), // yellow
    RGB(0xFF, 0x00, 0xFF)  // magenta
};

/**
 * @brief Initialise the LEDs module.
 *
 * @return true if the initialization was successful, false otherwise.
 */
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

/**
 * @brief Update LEDs according to the given combination
 *
 * The first half of the strip is used to display the combination,
 * and the second half is used to display the clues.
 *
 * @param combi The combination to be displayed
 */
void led_strip::update_combination(combination &combi)
{
    uint8_t clues_nb = 0;

    // Set combi LEDs
    for (uint8_t i = 0; i < STRIP_NUM_LEDS / 2; i++)
    {
        if (combi.slots[i].set)
        {
            leds[i] = code_colors[int(combi.slots[i].value)];
        }
        else
        {
            leds[i] = LED_OFF;
        }
    }

    // Set clues LEDs
    for (uint8_t i = 0; i < STRIP_NUM_LEDS / 2; i++)
    {
        if (i < combi.clues_correct)
        {
            leds[STRIP_NUM_LEDS / 2 + i] = LED_CORRECT;
        }
        else if (i < combi.clues_correct + combi.clues_present)
        {
            leds[STRIP_NUM_LEDS / 2 + i] = LED_WRONG_POS;
        }
        else
        {
            leds[STRIP_NUM_LEDS / 2 + i] = LED_OFF;
        }
    }
}

/**
 * @brief Refresh the LEDs on the strip
 */
void led_strip::refresh(void)
{
    LOG_INF("Refreshing LEDs on strip");
    int rc = led_strip_update_rgb(strip, leds.data(), STRIP_NUM_LEDS);
    if (rc)
    {
        LOG_ERR("Couldn't update strip: %d", rc);
    }
}

void led_strip::reset(void)
{
    LOG_INF("Switch off LEDs on strip");
    for (uint8_t i = 0; i < STRIP_NUM_LEDS / 2; i++)
    {
        leds[i] = LED_OFF;
    }
    refresh();
}