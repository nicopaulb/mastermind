
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/smf.h>

#include "combination.hpp"
#include "leds.hpp"
#include "buttons.hpp"
#include "ble.hpp"
#include "app_cfg.hpp"

#define LOG_LEVEL 4

LOG_MODULE_REGISTER(main);

enum state
{
	STATE_START,
	STATE_INPUT,
	STATE_CLUES,
	STATE_END_WIN,
	STATE_END_LOST
};

static void state_start_run(void *o);
static void state_input_run(void *o);
static void state_clues_run(void *o);
static void state_end_win_run(void *o);
static void state_end_lost_run(void *o);

static led_strip leds;
static buttons buts;
static combination code;
static etl::array<combination, MAX_TRY> tentatives;
static uint8_t try_id;

static const struct smf_state states[] = {
	[STATE_START] = SMF_CREATE_STATE(NULL, state_start_run, NULL, NULL, NULL),
	[STATE_INPUT] = SMF_CREATE_STATE(NULL, state_input_run, NULL, NULL, NULL),
	[STATE_CLUES] = SMF_CREATE_STATE(NULL, state_clues_run, NULL, NULL, NULL),
	[STATE_END_WIN] = SMF_CREATE_STATE(NULL, state_end_win_run, NULL, NULL, NULL),
	[STATE_END_LOST] = SMF_CREATE_STATE(NULL, state_end_lost_run, NULL, NULL, NULL),
};

struct s_object
{
	struct smf_ctx ctx;
} s_obj;

static void state_start_run(void *o)
{
	try_id = 0;
	code.random_fill();
	tentatives[try_id].unset_all();
	leds.update_combination(tentatives[try_id]);
	leds.refresh();
	smf_set_state(SMF_CTX(&s_obj), &states[STATE_INPUT]);
}

static void state_input_run(void *o)
{
	uint8_t slot_left = 0;
	button_val val = buts.wait_for_input(K_FOREVER);
	switch (val)
	{
	case button_val::BUTTON_VAL_1:
	case button_val::BUTTON_VAL_2:
	case button_val::BUTTON_VAL_3:
	case button_val::BUTTON_VAL_4:
	case button_val::BUTTON_VAL_5:
	case button_val::BUTTON_VAL_6:
		slot_left = tentatives[try_id].set_slot_next(static_cast<slot_value>(val));
		break;
	case button_val::BUTTON_VAL_NONE:
		LOG_ERR("None button pressed");
		break;
	default:
		LOG_ERR("Unknown button pressed");
		break;
	}

	if (slot_left == 0)
	{
		smf_set_state(SMF_CTX(&s_obj), &states[STATE_CLUES]);
	}
	else
	{
		leds.update_combination(tentatives[try_id]);
		leds.refresh();
	}
}

static void state_clues_run(void *o)
{
	LOG_INF("[Combi %d] All slot filled, showing clues", try_id);
	bool guessed = tentatives[try_id].compute_clues(code);
	leds.update_combination(tentatives[try_id++]);
	leds.refresh();

	ble_update_status(tentatives, code, try_id);

	if (guessed)
	{
		smf_set_state(SMF_CTX(&s_obj), &states[STATE_END_WIN]);
	}
	else if (try_id >= tentatives.size())
	{
		smf_set_state(SMF_CTX(&s_obj), &states[STATE_END_LOST]);
	}
	else
	{
		tentatives[try_id].unset_all();
		smf_set_state(SMF_CTX(&s_obj), &states[STATE_INPUT]);
	}
}

static void state_end_win_run(void *o)
{
	LOG_INF("WIN !");
	smf_set_state(SMF_CTX(&s_obj), &states[STATE_START]);
}

static void state_end_lost_run(void *o)
{
	LOG_INF("LOST !");
	leds.update_combination(code);
	leds.refresh();
	smf_set_state(SMF_CTX(&s_obj), &states[STATE_START]);
}

int main(void)
{
	if (!buts.init())
	{
		return 1;
	}

	if (!leds.init())
	{
		return 1;
	}

	if (!ble_init())
	{
		return 1;
	}

	smf_set_initial(SMF_CTX(&s_obj), &states[STATE_START]);

	while (1)
	{
		smf_run_state(SMF_CTX(&s_obj));
	}

	return 1;
}
