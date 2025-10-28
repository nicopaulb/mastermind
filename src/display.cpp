#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/auxdisplay.h>

#include "display.hpp"

#define LOG_LEVEL 4

LOG_MODULE_REGISTER(display);

/**
 * @brief Initialize the display
 *
 * @return true if the initialization was successful, false otherwise.
 */
bool display::init(void)
{
    int err = device_is_ready(segment_display);
    if (!err)
    {
        LOG_INF("Error: SPI device is not ready, err: %d", err);
        return false;
    }

    return true;
}

/**
 * @brief Print a number on the display (0-99)
 *
 * @param num Integer to print on the display (0-99)
 * @return true if the operation was successful, false otherwise.
 */
bool display::show_number(uint8_t num)
{
    char str[3];

    if (num > 99)
    {
        LOG_ERR("Error: Cannot show number bigger than 99");
        return false;
    }

    sprintf(str, "%02u", num);
    if (!auxdisplay_write(segment_display, (uint8_t *)str, strlen(str)))
    {
        LOG_ERR("Error: Cannot write to display");
        return false;
    }

    return true;
}

/**
 * @brief Clear the content on the display
 *
 * @return true if the operation was successful, false otherwise.
 */
bool display::clear(void)
{
    if (!auxdisplay_clear(segment_display))
    {
        LOG_ERR("Error: Cannot write to display");
        return false;
    }

    return true;
}
