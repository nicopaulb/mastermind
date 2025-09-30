#include <errno.h>
#include <string.h>

#define SLOT_NB 4

typedef enum slot_color {
	SLOT_COLOR_1,
	SLOT_COLOR_2,
	SLOT_COLOR_3,
	SLOT_COLOR_4,
	SLOT_COLOR_5,
	SLOT_COLOR_6,
} slot_color_t;

slot_color_t code[SLOT_NB];