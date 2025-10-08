#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

#include "buzzer.hpp"

#define PWM_PERIOD  PWM_USEC(4500)
#define LOG_LEVEL 4

LOG_MODULE_REGISTER(buzzer);

static const struct pwm_dt_spec pwm_buzzer = PWM_DT_SPEC_GET(DT_NODELABEL(buzzer));

bool buzzer_init(void)
{
    int err = 0;

    LOG_INF("Setting initial motor");
    if (!pwm_is_ready_dt(&pwm_buzzer)) {
        LOG_ERR("Error: PWM device %s is not ready", pwm_buzzer.dev->name);
        return false;
	}

    err = pwm_set_dt(&pwm_buzzer, PWM_PERIOD, PWM_PERIOD/2);
    if (err) {
        LOG_ERR("pwm_set_dt returned %d", err);
        return false;
    }

    return true;
}