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
	SETTING,		// Can be set
} entry_type_t;

typedef struct entry_struct_t
{
	char* name;

	entry_type_t type;
	// bool exits;

	// All possible contents
	union {
		void* value_p;
		bool(*action)(void);
	};
} entry_t;


extern uint16_t splitter_init(int rIDGiven, int max_depth);
extern void start_splitter(void);
extern void set_status_message(char *to);

#endif // __LUOS_SPLITTER__
