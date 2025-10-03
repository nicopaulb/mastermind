#ifndef COMBINATION_H
#define COMBINATION_H

#include <cstdint>

#include "etl/array.h"

#define SLOT_NB 4

enum class slot_value : uint8_t
{
    SLOT_VAL_1 = 0,
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
};

class combination
{
    
public:
    combination();
    etl::array<slot, SLOT_NB> slots;
    uint8_t clues_correct;
    uint8_t clues_present;
    void random_fill(void);
    void unset_all(void);
    void set_slot(int index, slot_value value);
    int set_slot_next(slot_value value);
    bool compute_clues(combination code);
};
#endif
