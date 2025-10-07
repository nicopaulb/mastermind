#ifndef LEDS_H
#define LEDS_H

#include <cstdint>
#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>

#include "etl/array.h"
#include "combination.hpp"

#define STRIP_NUM_LEDS 8
#define RGB(_r, _g, _b) {.r = (_r), .g = (_g), .b = (_b)}

class led_strip
{
public:
    bool init(void);
    void update_combination(combination &combi);
    void refresh(void);
    void reset(void);

private:
    const struct device *const strip = DEVICE_DT_GET(DT_ALIAS(led_strip));
    etl::array<struct led_rgb, STRIP_NUM_LEDS> leds;
};

#endif
