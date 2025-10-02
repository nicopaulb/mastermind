#include <cstdint>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include "etl/array.h"
#include "combination.hpp"

#define LOG_LEVEL 4

LOG_MODULE_REGISTER(combination);

combination::combination(void)
{
	unset_all();
}

void combination::random_fill(void)
{
	for (auto &slot : slots)
	{
		slot.value = static_cast<slot_value>(sys_rand8_get() % int(slot_value::SLOT_VAL_MAX));
		slot.set = true;
		slot.correct = false;
	}
}

void combination::unset_all(void)
{
	for (auto &slot : slots)
	{
		slot.set = false;
		slot.correct = false;
	}

	clues_correct = 0;
	clues_present = 0;
}

void combination::set_slot(int index, slot_value new_value)
{
	slots[index].value = new_value;
	slots[index].set = true;
	slots[index].correct = false;
}

int combination::set_slot_next(slot_value new_value)
{
	int slot_left = slots.size();

	for (auto &slot : slots)
	{
		slot_left--;
		if (!slot.set)
		{
			slot.value = new_value;
			slot.set = true;
			break;
		}
	}

	return slot_left;
}

bool combination::compute_clues(combination code)
{
	clues_correct = 0;
	clues_present = 0;

	// First loop for checking correct value and position
	for (uint8_t i = 0; i < slots.size(); i++)
	{
		if (code.slots[i].value == slots[i].value)
		{
			clues_correct++;
			slots[i].correct = true;
		}
	}

	// Second loop for checking correct value but wrong position
	for (uint8_t i = 0; i < slots.size(); i++)
	{
		if (!slots[i].correct)
		{
			for (uint8_t j = 0; j < code.slots.size(); j++)
			{
				if (slots[j].correct)
				{
					// Skip if position is already correct
					continue;
				}
				
				if (code.slots[j].value == slots[i].value)
				{
					clues_present++;
					break;
				}
			}
		}
	}

	LOG_INF("Correct : %d - Present : %d", clues_correct, clues_present);
	return clues_present == slots.size();
}
