#ifndef BUTTONS_H
#define BUTTONS_H

#include <cstdint>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include "etl/array.h"

enum class button_val : uint8_t
{
    BUTTON_VAL_1 = 0,
    BUTTON_VAL_2,
    BUTTON_VAL_3,
    BUTTON_VAL_4,
    BUTTON_VAL_5,
    BUTTON_VAL_6,
    BUTTON_VAL_NONE,
    BUTTON_VAL_MAX,
};

class buttons
{
public:
    bool init(void);
    button_val wait_for_input(k_timeout_t timeout);

private:
    const etl::array<struct gpio_dt_spec, 6> specs = {{GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios),
                                                       GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios),
                                                       GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios),
                                                       GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios),
                                                       GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios),
                                                       GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios)}};
    struct gpio_callback cb_data;
};

#endif
