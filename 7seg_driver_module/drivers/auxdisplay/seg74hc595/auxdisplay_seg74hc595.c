/*
 * 74HC595 7-segment display driver
 * Copyright (c) 2025 Nicolas BESNARD
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_seg74hc595

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/auxdisplay.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(seg74hc595_auxdisplay, CONFIG_AUXDISPLAY_LOG_LEVEL);

/* Segment bit definitions */
#define MINUS_BIT BIT(6) /* Segment G only */
#define DP_BIT BIT(7)    /* Decimal point */
#define BLANK (0)        /* No segments lit */

/* Segment mapping: A=bit0, B=bit1, C=bit2, D=bit3, E=bit4, F=bit5, G=bit6; DP=bit7 */
static const uint8_t digit_segment_codes[] = {
    0x3F, /* 0 */
    0x06, /* 1 */
    0x5B, /* 2 */
    0x4F, /* 3 */
    0x66, /* 4 */
    0x6D, /* 5 */
    0x7D, /* 6 */
    0x07, /* 7 */
    0x7F, /* 8 */
    0x6F, /* 9 */
};

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "7 segments 74HC595 driver enabled without any devices"
#endif

struct seg74hc595_config
{
    struct spi_dt_spec spi;
    struct gpio_dt_spec latch_pin;
    struct auxdisplay_capabilities capabilities;
};

struct seg74hc595_data
{
    uint8_t display_buf[4];
};

static int seg74hc595_update_display(const struct device *dev)
{
    const struct seg74hc595_config *cfg = dev->config;
    struct seg74hc595_data *data = dev->data;
    int rv = -1;

    struct spi_buf tx_spi_buf = {.buf = data->display_buf, .len = cfg->capabilities.columns};
    struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};

    gpio_pin_set_dt(&cfg->latch_pin, 0);
    rv = spi_write_dt(&cfg->spi, &tx_spi_buf_set);
    if (rv)
    {
        LOG_ERR("spi_write_dt() failed, err %d", rv);
    }
    gpio_pin_set_dt(&cfg->latch_pin, 1);

    return rv;
}

static int seg74hc595_auxdisplay_write(const struct device *dev, const uint8_t *buf, uint16_t len)
{
    const struct seg74hc595_config *cfg = dev->config;
    struct seg74hc595_data *data = dev->data;
    uint32_t pos = len - 1;
    uint16_t i = 0;

    if (len > cfg->capabilities.columns)
    {
        return -EINVAL;
    }

    memset(data->display_buf, 0, sizeof(data->display_buf));

    while (i < len && pos >= 0)
    {
        char c = buf[i];
        uint8_t segment_code = 0;

        if (c >= '0' && c <= '9')
        {
            segment_code = digit_segment_codes[c - '0'];
        }
        else if (c == '-')
        {
            segment_code = MINUS_BIT;
        }
        else
        {
            segment_code = BLANK;
        }

        data->display_buf[pos] = ~segment_code; // TODO Make it work with common cathode

        /* Check if next character is a decimal point */
        if (i + 1 < len && buf[i + 1] == '.')
        {
            data->display_buf[pos] |= ~DP_BIT; /* Add decimal point to current digit */
            i += 2;                            /* Skip both the character and the '.' */
        }
        else
        {
            i++; /* Just move to next character */
        }
        pos--;
    }

    return seg74hc595_update_display(dev);
}

static int seg74hc595_auxdisplay_clear(const struct device *dev)
{
    struct seg74hc595_data *data = dev->data;

    memset(data->display_buf, 0xFF, sizeof(data->display_buf)); // TODO support common cathode

    return seg74hc595_update_display(dev);
}

static int seg74hc595_initialize(const struct device *dev)
{
    const struct seg74hc595_config *cfg = dev->config;

    if (cfg->capabilities.columns > 4 || !gpio_is_ready_dt(&cfg->latch_pin) || !spi_is_ready_dt(&cfg->spi))
    {
        return -ENODEV;
    }

    gpio_pin_configure_dt(&cfg->latch_pin, GPIO_OUTPUT_ACTIVE);

    return seg74hc595_auxdisplay_clear(dev);
}

static int seg74hc595_auxdisplay_display_on(const struct device *dev)
{
    return seg74hc595_update_display(dev);
}

static int seg74hc595_auxdisplay_display_off(const struct device *dev)
{
    uint8_t buffer_save[4];
    const uint8_t buffer_empty[4] = {0};
    struct seg74hc595_data *data = dev->data;
    int rv = -1;

    memcpy(buffer_save, data->display_buf, sizeof(data->display_buf));
    rv = seg74hc595_auxdisplay_write(dev, buffer_empty, sizeof(buffer_empty));
    memcpy(data->display_buf, buffer_save, sizeof(data->display_buf));
    return rv;
}

static int seg74hc595_auxdisplay_capabilities_get(const struct device *dev,
                                                  struct auxdisplay_capabilities *cap)
{
    const struct seg74hc595_config *cfg = dev->config;

    memcpy(cap, &cfg->capabilities, sizeof(*cap));
    return 0;
}

static const struct auxdisplay_driver_api seg74hc595_auxdisplay_api = {
    .write = seg74hc595_auxdisplay_write,
    .clear = seg74hc595_auxdisplay_clear,
    .display_on = seg74hc595_auxdisplay_display_on,
    .display_off = seg74hc595_auxdisplay_display_off,
    .capabilities_get = seg74hc595_auxdisplay_capabilities_get,
};

#define SEG74HC595_DEFINE(inst)                                                                                  \
    static const struct seg74hc595_config seg74hc595_config_##inst = {                                           \
        .spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0),                                \
        .latch_pin = GPIO_DT_SPEC_INST_GET(inst, latch_gpios),                                                   \
        .capabilities =                                                                                          \
            {                                                                                                    \
                .columns = DT_INST_PROP(inst, digits),                                                           \
                .rows = 1,                                                                                       \
            },                                                                                                   \
    };                                                                                                           \
    static struct seg74hc595_data seg74hc595_data_##inst;                                                        \
    DEVICE_DT_INST_DEFINE(inst, seg74hc595_initialize, NULL, &seg74hc595_data_##inst, &seg74hc595_config_##inst, \
                          POST_KERNEL, CONFIG_AUXDISPLAY_INIT_PRIORITY,                                          \
                          &seg74hc595_auxdisplay_api);

DT_INST_FOREACH_STATUS_OKAY(SEG74HC595_DEFINE)