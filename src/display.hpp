#ifndef DISPLAY_H
#define DISPLAY_H

#include <zephyr/device.h>

class display
{
public:
    bool init();
    bool show_number(uint8_t num);
    bool clear(void);

private:
    const struct device *segment_display = DEVICE_DT_GET(DT_NODELABEL(seg_display));
};

#endif
