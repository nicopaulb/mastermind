#ifndef COMBINATION_H
#define COMBINATION_H

#include <cstdint>

#include "etl/array.h"

#define SLOT_NB 4

enum class slot_value : uint8_t
{
    SLOT_VAL_1,
    SLOT_VAL_2,
    SLOT_VAL_3,
    SLOT_VAL_4,
    SLOT_VAL_5,
    SLOT_VAL_6,
    SLOT_VAL_MAX,
};

struct slot
{
    slot_value value;
    bool set;
    bool correct;
};

class combination
{

public:
    etl::array<slot, SLOT_NB> values;
    bool guessed = false;
    void random_fill(void);
    void unset(void);
    void set_slot(int index, slot_value value);
};
#endif
