#include <cstdint>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include "etl/array.h"
#include "combination.hpp"

#define LOG_LEVEL 4

LOG_MODULE_REGISTER(combination);

/**
 * @brief Combination object constructor.
 *
 * Resets all slots in the combination by calling unset_all().
 */
combination::combination(void)
{
	unset_all();
}

/**
 * @brief Randomly fill the combination slots.
 */
void combination::random_fill(void)
{
	for (auto &slot : slots)
	{
		slot.value = static_cast<slot_value>(sys_rand8_get() % int(slot_value::SLOT_VAL_MAX));
		slot.set = true;
	}
}

/**
 * @brief Unset all slots in the combination.
 */
void combination::unset_all(void)
{
	for (auto &slot : slots)
	{
		slot.set = false;
	}

	clues_correct = 0;
	clues_present = 0;
}

/**
 * @brief Set the value of the given slot in the combination.
 *
 * @param index The index of the slot to set.
 * @param new_value The value to set in the given slot.
 */
void combination::set_slot(int index, slot_value new_value)
{
	slots[index].value = new_value;
	slots[index].set = true;
}

/**
 * @brief Set the value of the next unset slot.
 *
 * @param new_value The value to set in the next unset slot.
 *
 * @return The number of unset slots left after the operation.
 */
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

/**
 * @brief Compute the correct and present clues from the given combination code
 *
 * @param code Combination code to check against
 *
 * @return True if all slots are correct, false otherwise
 */
bool combination::compute_clues(combination code)
{
	clues_correct = 0;
	clues_present = 0;

	// First loop for checking correct value and position
	for (uint8_t i = 0; i < code.slots.size(); i++)
	{
		if (code.slots[i].value == slots[i].value)
		{
			clues_correct++;
			code.slots[i].value = slot_value::SLOT_VAL_MAX;
		}
	}

	// Second loop for checking correct value but wrong position
	for (uint8_t i = 0; i < code.slots.size(); i++)
	{
		if (code.slots[i].value == slot_value::SLOT_VAL_MAX)
		{
			// Skip slot already used
			continue;
		}

		for (uint8_t j = 0; j < slots.size(); j++)
		{
			if (code.slots[j].value == slot_value::SLOT_VAL_MAX)
			{
				// Skip slot already used
				continue;
			}

			if (code.slots[j].value == slots[i].value)
			{
				clues_present++;
				break;
			}
		}
	}

	LOG_INF("Correct : %d - Present : %d", clues_correct, clues_present);
	return clues_correct == slots.size();
}

/**
 * @brief Serialize the object in the passed buffer.
 *
 * The combination data is serialized in the following order:
 * - The values of the slots, stored in the slots array.
 * - The number of present clues.
 * - The number of correct clues.
 *
 * @param buf The buffer to serialize the data to.
 *
 * @return A pointer to the end of the serialized data.
 */
uint8_t *combination::serialize(uint8_t *buf)
{
	memcpy(buf, slots.data(), sizeof(slots));
	buf += sizeof(slots);
	memcpy(buf, &clues_present, sizeof(clues_present));
	buf += sizeof(clues_present);
	memcpy(buf, &clues_correct, sizeof(clues_correct));
	return buf + sizeof(clues_correct);
}