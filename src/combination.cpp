#include <cstdint>
#include <zephyr/random/random.h>
#include "etl/array.h"
#include "combination.hpp"

void combination::random_fill(void)
{
	for (auto &slot : values)
	{
		slot.value = static_cast<slot_value>(sys_rand8_get() % int(slot_value::SLOT_VAL_MAX));
		slot.set = true;
		slot.correct = false;
	}
};

void combination::unset(void)
{
	for (auto &slot : values)
	{
		slot.set = false;
	}
}

void combination::set_slot(int index, slot_value value)
{
	values[index].value = value;
	values[index].set = true;
}