#ifndef __LUOS_SPLITTER__
#define __LUOS_SPLITTER__

#include <stdint.h>
#include <stdbool.h>
// #include "fonts/LuOS_System_Font.h"

// #define MAX_OPTIONS_PER_LIST (LCD_HEIGHT/(LuOS_System_Font_data[1]))
// #define MAX_OPTION_NAME_LENGTH (LCD_WIDTH/(2*LuOS_System_Font_data[0]))-2

/* This interface splits the screen into two, and reserves space for a line in the middle
	as such,
*/

typedef enum entry_type {
	BRANCH,			// A folder-type option, encompasses multiple entries
	LEAF,			// Purely decorative, no interaction possible
	FUNCTIONABLE,	// Mapped to a function
	SETTING_INT,	// Has integer value
	SETTING_FLOAT,	// Has float value
	SETTING_RANGE,	// A value inside a set range
	SETTING_STRING,	// A string
	SETTING_TOGGLE,	// A true or false setting
	SETTING_LIST,	// A list of string-type options - useful for enums
} entry_type_t;

typedef struct entry_t
{
	entry_type_t type;
	char* name;
	bool selected;

	// All possible contents
	union {
		struct entry_t* sub_entries;
		bool(*action)(void);

		int32_t value_int32;
		float value_float;
		double value_double;
		uint64_t range_32per;
		// Since a union can only be one of the given types, we use the upper and lower half of a 64 bit number to define a maximum and a minimum, respectively.
		char *value_string;
		bool toggle_on;
		char *name_list;
	};
} entry_t;

extern uint16_t splitter_init(int rIDGiven);
extern void lcd_print_string_core1(int rID, char* str);

#endif // __LUOS_SPLITTER__
