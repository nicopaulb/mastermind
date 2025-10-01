#ifndef BUTTONS_H
#define BUTTONS_H

#include <cstdint>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include "etl/array.h"

class buttons
{
public:
    bool init(void);
    static struct k_poll_signal *get_signal(void);

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
