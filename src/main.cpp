
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/smf.h>
#include <zephyr/sys/poweroff.h>

#include "etl/array.h"
#include "combination.hpp"
#include "leds.hpp"
#include "buttons.hpp"
#include "ble.hpp"
#include "buzzer.hpp"
#include "app_cfg.hpp"

#define LOG_LEVEL 4

LOG_MODULE_REGISTER(main);

enum state
{
	STATE_START,
	STATE_CHECK_INPUT,
	STATE_CHECK_CMD,
	STATE_CLUES,
	STATE_END_WIN,
	STATE_END_LOST,
	STATE_OFF
};

static void state_start_run(void *o);
static void state_check_input_run(void *o);
static void state_check_cmd_run(void *o);
static void state_clues_run(void *o);
static void state_end_win_run(void *o);
static void state_end_lost_run(void *o);
static void state_off_run(void *o);

// FSM state variables
static led_strip leds;
static buzzer buzzer;
static buttons buts;
static combination code;
static etl::array<combination, MAX_TRY> tentatives;
static uint8_t try_id;
static bool manual_mode;
static struct smf_ctx ctx;

static const struct smf_state states[] = {
	[STATE_START] = SMF_CREATE_STATE(NULL, state_start_run, NULL, NULL, NULL),
	[STATE_CHECK_INPUT] = SMF_CREATE_STATE(NULL, state_check_input_run, NULL, NULL, NULL),
	[STATE_CHECK_CMD] = SMF_CREATE_STATE(NULL, state_check_cmd_run, NULL, NULL, NULL),
	[STATE_CLUES] = SMF_CREATE_STATE(NULL, state_clues_run, NULL, NULL, NULL),
	[STATE_END_WIN] = SMF_CREATE_STATE(NULL, state_end_win_run, NULL, NULL, NULL),
	[STATE_END_LOST] = SMF_CREATE_STATE(NULL, state_end_lost_run, NULL, NULL, NULL),
	[STATE_OFF] = SMF_CREATE_STATE(NULL, state_off_run, NULL, NULL, NULL),
};

static void state_start_run(void *o)
{
	try_id = 0;
	tentatives[try_id].unset_all();
	leds.reset();

	if (!manual_mode)
	{
		// If manual mode is disabled, generate a random code
		code.random_fill();
	}

	ble_update_status(tentatives, code, try_id);
	smf_set_state(&ctx, &states[STATE_CHECK_CMD]);
}

static void state_check_cmd_run(void *o)
{
	etl::bitset<BT_COMMAND_COUNT> &cmds = ble_get_commands();
	etl::array<uint8_t, BT_COMMAND_BUF_SIZE> &buf = ble_get_command_buf();
	const struct smf_state *next_state = &states[STATE_CHECK_INPUT];
	uint8_t pos = 0;

	while (cmds.any())
	{
		pos = cmds.find_first(true);
		switch (pos)
		{
		case BT_COMMAND_RESET:
			LOG_INF("Executing 'Reset' command");
			manual_mode = false;
			next_state = &states[STATE_START];
			break;
		case BT_COMMAND_OFF:
			LOG_INF("Executing 'Off' command");
			next_state = &states[STATE_OFF];
			break;
		case BT_COMMAND_CODE:
			LOG_INF("Executing 'Code' command");
			manual_mode = true;
			for (uint8_t i = 0; i < code.slots.size(); i++)
			{
				code.set_slot(i, static_cast<slot_value>(buf[i]));
			}
			next_state = &states[STATE_START];
			break;
		default:
			LOG_ERR("Unknown command");
			break;
		}
		cmds.set(pos, false);
	}

	smf_set_state(&ctx, next_state);
}

static void state_check_input_run(void *o)
{
	uint8_t slot_left = 0;
	button_val val = buts.wait_for_input(K_MSEC(1000));
	switch (val)
	{
	case button_val::BUTTON_VAL_1:
	case button_val::BUTTON_VAL_2:
	case button_val::BUTTON_VAL_3:
	case button_val::BUTTON_VAL_4:
	case button_val::BUTTON_VAL_5:
	case button_val::BUTTON_VAL_6:
		buzzer.buzzer_play_button();
		slot_left = tentatives[try_id].set_slot_next(static_cast<slot_value>(val));
		break;
	case button_val::BUTTON_VAL_NONE:
		smf_set_state(&ctx, &states[STATE_CHECK_CMD]);
		return;
	default:
		LOG_ERR("Unknown button pressed");
		break;
	}

	if (slot_left == 0)
	{
		smf_set_state(&ctx, &states[STATE_CLUES]);
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

	buzzer.buzzer_play_clues();

	ble_update_status(tentatives, code, try_id);

	if (guessed)
	{
		smf_set_state(&ctx, &states[STATE_END_WIN]);
	}
	else if (try_id >= tentatives.size())
	{
		smf_set_state(&ctx, &states[STATE_END_LOST]);
	}
	else
	{
		tentatives[try_id].unset_all();
		smf_set_state(&ctx, &states[STATE_CHECK_CMD]);
	}
}

static void state_end_win_run(void *o)
{
	LOG_INF("WIN !");
	buzzer.buzzer_play_win();

	k_sleep(K_SECONDS(5));
	manual_mode = false;
	smf_set_state(&ctx, &states[STATE_START]);
}

static void state_end_lost_run(void *o)
{
	LOG_INF("LOST !");
	buzzer.buzzer_play_lose();
	leds.update_combination(code);
	leds.refresh();

	k_sleep(K_SECONDS(5));
	manual_mode = false;
	smf_set_state(&ctx, &states[STATE_START]);
}

static void state_off_run(void *o)
{
	LOG_INF("Powering off");
	leds.reset();
	sys_poweroff();
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

	if(!buzzer.init())
	{
		return 1;
	}

	manual_mode = false;
	smf_set_initial(&ctx, &states[STATE_START]);

	while (1)
	{
		smf_run_state(&ctx);
	}

	return 1;
}
