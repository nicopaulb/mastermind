#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

#include "etl/array.h"
#include "buzzer.hpp"

#define BUZZER_STACK 1024
#define LOG_LEVEL 4

LOG_MODULE_REGISTER(buzzer);

static const etl::array<note_duration, 2> beep{{{200, 1000},
                                                {500, 300}}};

K_THREAD_STACK_DEFINE(threadStack, BUZZER_STACK);
buzzer::buzzer()
{
    k_sem_init(&initialized, 0, 1);
    threadId = k_thread_create(&kthread,
                               threadStack,
                               K_THREAD_STACK_SIZEOF(threadStack),
                               buzzer::thread,
                               this, NULL, NULL,
                               K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
}

/**
 * @brief Thread function for buzzer.
 *
 * This function is responsible for playing all the notes.
 */
void buzzer::thread(void *object, void *d1, void *d2)
{
    buzzer *buzzer_obj = reinterpret_cast<buzzer *>(object);
    k_sem_take(&buzzer_obj->initialized, K_FOREVER);

    while (1)
    {
        for (auto elem : buzzer_obj->song)
        {
            if (elem.note == 0)
            {
                // Silence
                pwm_set_pulse_dt(&buzzer_obj->pwm_buzzer, 0);
            }
            else
            {
                pwm_set_dt(&buzzer_obj->pwm_buzzer, PWM_HZ(elem.note),
                           PWM_HZ((elem.note)) / 2);
            }
            k_msleep(elem.duration);
        }

        pwm_set_pulse_dt(&buzzer_obj->pwm_buzzer, 0);
        k_sleep(K_FOREVER);
    }
}

/**
 * @brief Initialises the buzzer.
 *
 * Checks if the PWM device is ready and initializes the
 * semaphore used to signal thread initialization.
 *
 * @return true if the initialization was successful, false otherwise.
 */
bool buzzer::init(void)
{
    int err = 0;

    if (!pwm_is_ready_dt(&pwm_buzzer))
    {
        LOG_ERR("Error: PWM device %s is not ready", pwm_buzzer.dev->name);
        return false;
    }

    k_sem_give(&initialized);
    return true;
}

/**
 * @brief Play the beep sound.
 *
 */
void buzzer::buzzer_play_input(void)
{
    song.assign(beep.begin(), beep.end());
    k_wakeup(threadId);
}

void buzzer::buzzer_play_start(void)
{
    song.assign(beep.begin(), beep.end());
    k_wakeup(threadId);
}

void buzzer::buzzer_play_win(void)
{
    song.assign(beep.begin(), beep.end());
    k_wakeup(threadId);
}

void buzzer::buzzer_play_lose(void)
{
    song.assign(beep.begin(), beep.end());
    k_wakeup(threadId);
}
void buzzer::buzzer_play_clues(void)
{
    song.assign(beep.begin(), beep.end());
    k_wakeup(threadId);
}