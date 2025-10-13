#ifndef BUZZER_H
#define BUZZER_H

#include <zephyr/drivers/pwm.h>

#include "etl/array_view.h"

struct note_duration
{
    uint16_t note;
    uint16_t duration;
};

class buzzer
{
public:
    buzzer();
    bool init();
    void buzzer_play_button(void);
    void buzzer_play_win(void);
    void buzzer_play_lose(void);
    void buzzer_play_clues(void);
    void buzzer_play_start(void);

private:
    const struct pwm_dt_spec pwm_buzzer = PWM_DT_SPEC_GET(DT_NODELABEL(buzzer));
    k_tid_t threadId;
    struct k_thread kthread;
    struct k_sem initialized;
    etl::array_view<note_duration> song;
    static void thread(void *object, void *d1, void *d2);
};

#endif
