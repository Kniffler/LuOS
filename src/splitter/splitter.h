#ifndef __LUOS_SPLITTER__
#define __LUOS_SPLITTER__

#include <stdlib.h>
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

typedef union entry_value {
	void* p;
	void(*action)(void);
} entry_value_t;

typedef struct entry_struct_t
{
	char* name;
	entry_type_t type;
	bool exits;

	// All possible contents
	entry_value_t value;
} entry_t;


extern void splitter_free_everything(void);
extern uint16_t splitter_init(int rIDGiven, int maxDepth);
extern void splitter_start(void);
extern void set_status_message(char *to);


extern int create_entry_return_ID(char *name, entry_type_t type, bool exits, entry_value_t value);
extern void delete_entry_by_ID(int ID);
extern void set_entry_function(int entryID, void(*func)(void));

extern bool append_entry_to_root(int entryID);
extern bool append_entry_to_branch(int parentID, int childID);

extern entry_t* get_entry_by_ID(int ID);
extern entry_t* get_currently_selected_entry();
extern void execute_on_branches(int parentID, void(*func)(entry_t*));
extern char* get_past_entries_filepath_style(size_t charsToAllocate);

// extern bool finalize_entry_structure();

#endif // __LUOS_SPLITTER__
