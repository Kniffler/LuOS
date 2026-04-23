#include "splitter.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "fonts/LuOS_System_Font.h"
#include "i2ckbd.h"
#include "lcdspi.h"
#include <pico/platform/common.h>
#include <hardware/watchdog.h>
#include <sys/unistd.h>
#include <pico/malloc.h>

#include "src/include/debug.h"

#define USE_DEAL_VOID_P 1
// #define DEAL_FREE_DATA() (free(node->data.value_p)) // This is a surprise tool that'll help us later!
#include "DEAL.h"

#include "keyboard_define.h"
#include "lcdspi_core1_wrap.h"

// Misc.
static int rID;

const unsigned char *mainFont = (unsigned char*)LuOS_System_Font_data;
// When changing this, please modify the draw_middle_line_segment function according to your specified font

static bool inited = 0;

// Display variables
static uint8_t max_entry_name_length = 0;
static uint8_t max_entries_to_show = 0;
static uint8_t max_entry_depth = 0;
static char *empty_entry_name;

static uint16_t right_side_offset = 0;
static uint8_t divider_width = 0;


static int32_t right_indexing_limit_upper = 0;
static int32_t right_indexing_limit_lower = 0;

static int32_t left_indexing_limit_upper = 0;
static int32_t left_indexing_limit_lower = 0;

// Entry variables
static deal_t root_entries;
static deal_t *shown_entries_right;
static deal_t *shown_entries_left;
static deal_t offscreen_entries_left;

// Tracking
static deal_t entry_path;
static entry_t *last_branch;

// Selector/cursor stuff
static bool selector_on_right = 0; // Can only be on the left (0) or right (1)
static bool prev_selector_right = 0;

static int16_t selector_y = 0;
static int16_t prev_selector_y = 0;

static int16_t current_depth = 0;

// The editor and status message
static bool in_setting_editor = 0;
static uint16_t message_y_start = 0;
static char *edit_print;
static char *last_msg;


// Customization
static deal_t created_entries;

// Functions
static void free_all_entries(deal_t *src)
{
	if(!src) { DEBUG_PRINT_ERR("Tried to use free_all_entries with NULL pointer\n"); return; }
	for(size_t i = 0; i < deal_get_length(src); i++)
	{
		entry_t *subject = (entry_t*)deal_get(src, i);

		free((char*)subject->name);
		if(subject->type == BRANCH)
		{
			free_all_entries((deal_t *)subject->value.p);
			deal_free((deal_t *)subject->value.p);
		}
		free(subject);
	}
	return;
}

void splitter_free_everything()
{
	DEBUG_PRINT("Freeing everything\n");
	free((char*)empty_entry_name);
	free(edit_print);
	free(last_msg);
	free_all_entries(&root_entries);
	DEBUG_PRINT("DONE\n");
}

static inline int min(int a, int b) { return (a>b) ? b : a; }
static inline int max(int a, int b) { return (a>b) ? a : b; }

static void clean_string(char *msg, size_t size_limit)
{
	for(int i = 0; i < strlen(msg); i++)
	{
		if(i >= size_limit) { msg[i] = '\0'; break; }
		switch(msg[i])
		{
			case '\n':
			case '\v':
			case '\f':
			case '\r':
			case '\xDB':
				// if(msg[i]<32) {  }
				for(int k = i; k < strlen(msg); k++)
				{
					if(!msg[k+1]) { break; }
					msg[k] = msg[k+1];
				}
			break;
			default: break;
		}
	}
}

void set_status_message(char *to)
{
	if(in_setting_editor || strcmp(last_msg, to)==0) { return; }
	DEBUG_PRINT("Setting status message to \"%s\"\n", to);

	int16_t temp_x = lcdc1_get_current_x(rID);
	int16_t temp_y = lcdc1_get_current_y(rID);


	int16_t name_limit = lcdc1_get_region_width(rID)/mainFont[0];
	name_limit = (name_limit>1) ? name_limit : 3;

	clean_string(to, name_limit);

	lcdc1_region_set_current(rID, 0, message_y_start);
	for(uint8_t i = 0; i < name_limit; i++)
	{
		lcdc1_putc(rID, ' ');
	}

	lcdc1_region_set_current(rID, 0, message_y_start);
	lcdc1_print_string(rID, to);

	lcdc1_region_set_current(rID, temp_x, temp_y);

	strncpy(last_msg, to, name_limit);
}

extern int create_entry_return_ID(char *name, entry_type_t type, bool exits, entry_value_t value)
{
	if(!name) { DEBUG_PRINT_ERR("Tried to create entry without name\n"); return -1; }

	entry_t *new_entry = calloc(1, sizeof(entry_t));
	if(!new_entry) { return -2; }

	new_entry->name = calloc(strlen(name)+1, sizeof(char));
	if(!new_entry->name) { DEBUG_PRINT_ERR("Unable to allocate new entry name\n"); return -3; }

	strcpy(new_entry->name, name);

	// if(strlen(new_entry->name)>max_entry_name_length)
	// {
	// 	new_entry->name[max_entry_name_length] = '\0';
	// 	new_entry->name = realloc(new_entry->name, sizeof(char)*(max_entry_name_length));
	// 	if(!new_entry->name) { DEBUG_PRINT_ERR("Unable to reallocate new entry name\n"); return -4; }
	// }
	new_entry->name[strlen(new_entry->name)] = '\0';

	new_entry->type = type;
	new_entry->exits = exits;
	new_entry->value = value;

	deal_add(&created_entries, (void*)new_entry);

	int ID = deal_get_length(&created_entries);
	DEBUG_PRINT("Created entry \"%s\" ID #%d\n", new_entry->name, ID);
	return ID;
}

extern void delete_entry_by_ID(int ID)
{
	entry_t *entry = deal_get(&created_entries, ID-1);
	if(!entry) { DEBUG_PRINT_ERR("Unable to delete invalid entry\n"); return; }
	if(entry->type==BRANCH && entry->value.p)
	{
		free_all_entries((deal_t*)entry->value.p);
	}
	deal_pop_cascading(&created_entries, ID-1);
}

extern void set_entry_function(int entryID, void(*func)(void))
{
	entry_t *entry = deal_get(&created_entries, entryID-1);
	if(!entry) { DEBUG_PRINT_ERR("Unable to set_entry_function due to invalid index\n"); return; }
	entry->value.action = func;
	return;
}

extern bool append_entry_to_root(int entryID)
{
	if(entryID<=0) { return false; }
	entry_t *entry = deal_get(&created_entries, entryID-1);
	if(!entry) { DEBUG_PRINT_ERR("Unable to append invalid entry to root\n"); return false; }
	deal_add(&root_entries, (void*)entry);
	return true;
}

extern bool append_entry_to_branch(int parentID, int childID)
{
	entry_t *parent = deal_get(&created_entries, parentID-1);
	entry_t *child = deal_get(&created_entries, childID-1);

	if(parentID==0) { return append_entry_to_root(childID); }

	if(!parent || !child || parentID == childID)
	{
		DEBUG_PRINT_ERR("Invalid indecies passed to function append_entry_to_branch, %d (parent) and %d (child)\n", parentID, childID);
		return false;
	}
	if(parent->type != BRANCH) { DEBUG_PRINT_ERR("Unable to append to non-branch entry\n"); return false; }

	deal_add((deal_t*)parent->value.p, (void*)child);
	return true;
}

entry_t* get_entry_by_ID(int ID)
{
	return (entry_t*)deal_get(&created_entries, ID-1);
}

entry_t* get_currently_selected_entry()
{
	deal_t *relative_side = (selector_on_right) ? shown_entries_right : shown_entries_left;
	uint16_t lower_index_limit = (selector_on_right) ? right_indexing_limit_lower : left_indexing_limit_lower;

	entry_t *selected_entry = (entry_t*)deal_get(relative_side, lower_index_limit+selector_y);
	return selected_entry;
}

void execute_on_branches(int parentID, void(*func)(entry_t*))
{
	entry_t *entry = deal_get(&created_entries, parentID-1);
	if(entry->type!=BRANCH || !entry->value.p) { DEBUG_PRINT_ERR("Invalid to execute function on non-compatible entry\n"); return; }
	for(size_t i = 0; i < deal_get_length((deal_t*)entry->value.p); i++)
	{
		entry_t *current = deal_get((deal_t*)entry->value.p, i);
		func(current);
	}
}

extern char* get_past_entries_filepath_style(size_t charsToAllocate)
{
	size_t len = deal_get_length(&entry_path);
	char *out = calloc(charsToAllocate, sizeof(char));

	size_t write_index = 0;
	for(size_t i = 0; i < len; i++)
	{
		entry_t *entry = (entry_t*)deal_get(&entry_path, i);
		strcat(out, "/");
		strcat(out, (char*)entry->name);
	}
	return out;
}


// static bool validate_input_for_setting(char * input, entry_type_t setting)
// {
// 	if(setting==LEAF || setting==BRANCH || setting==FUNCTIONABLE) { return false; }
// 	return true;
// }

static void open_editor(entry_t *to_edit)
{
	in_setting_editor = true;
	static int key;
	uint8_t cursor_pos = 0;
	int16_t name_limit = lcdc1_get_region_width(rID)/mainFont[0];
	memset(edit_print, ' ', name_limit);
	bool exit = false;
	bool set_setting = true;

	for(;;)
	{
		key = read_i2c_kbd();
		if(key == -1 || key < 0) { sleep_ms(40); continue; }
		switch(key)
		{
			case KEY_ESC: set_setting = false; exit = true; break;
			case KEY_ENTER: exit = true; break;

			default:
				if(cursor_pos+1 > name_limit) { exit = true; break; }
				if(key >= 32 && key < 127) { edit_print[cursor_pos++] = key; }
			break;
		}
		if(exit) { break; }
	}
	if(set_setting)
	{
		// validate_input_for_setting(edit_print, to_edit->type);
	}
}

static inline void draw_middle_line_segment(int from_top_index)
{
	if(from_top_index<0) { DEBUG_PRINT_ERR("Invalid index, used %d\n", from_top_index); return; }

	int16_t temp_x = lcdc1_get_current_x(rID);
	int16_t temp_y = lcdc1_get_current_y(rID);

	lcdc1_region_set_current(rID, right_side_offset-divider_width, from_top_index*mainFont[1]);

	if(divider_width<mainFont[0])
	{
		// We, most literally, draw a line here. \xDB maps to character 219, which maps to a fully filled bitmap.
		lcdc1_putc(rID, (from_top_index==selector_y) ? '\xCD' : '\xDB'); // ASCII(219)
		// If the selector is currently on our height: \xCD (thick line)

		lcdc1_region_set_current(rID, right_side_offset, from_top_index*mainFont[1]);
		lcdc1_putc(rID, ' '); // The space then clears any overflow.

	}else if (from_top_index!=selector_y) {lcdc1_putc(rID, '\xBA'); }
	// Alternative options, in case we have a perfect 1 character space to play with:
	// 254 FE
	// 186 BA <-
	// 179 B3

	else
	{
		if((get_currently_selected_entry())->type==BRANCH) { lcdc1_putc(rID, (selector_on_right) ? '\xF2' : '\xF3'); }
		else { lcdc1_putc(rID, (selector_on_right) ? '>' : '<'); }
	}

	lcdc1_region_set_current(rID, temp_x, temp_y);
}
static void draw_middle_line()
{
	DEBUG_PRINT("Started\n");
	lcdc1_reset_coords(rID);
	for(int i = 0; i < max_entries_to_show; i++)
	{
		draw_middle_line_segment(i);
	}
}

static inline void advance_print_y(bool on_left_side) { lcdc1_region_set_current(rID, lcdc1_get_current_x(rID), lcdc1_get_current_y(rID)+mainFont[1]); }

static void print_and_cut_entry_name(char *entry_name, bool is_on_right, bool highlight)
{
	if(!entry_name) { return; }

	char limited_name[max_entry_name_length+1];
	strncpy(limited_name, entry_name, max_entry_name_length);
	limited_name[max_entry_name_length] = '\0';

	#ifndef SPLITTER_CENTER_EACH_ENTRY
	lcdc1_region_set_current(rID, (is_on_right) ? right_side_offset : 0, lcdc1_get_current_y(rID));
	#else
	lcdc1_region_set_current(rID,
			((is_on_right) ? right_side_offset : 0) + ( ((right_side_offset - divider_width) - strlen((char*)limited_name)*mainFont[0])/2 ),
			lcdc1_get_current_y(rID));
	#endif

	if(highlight) { lcdc1_putc(rID, '\017'); } // These highlight characters do not affect the current print coords
	lcdc1_print_string(rID, limited_name);
	if(highlight) { lcdc1_putc(rID, '\016'); }
}

static void index_limits_bounds_check()
{
	right_indexing_limit_lower = max(right_indexing_limit_lower, 0);
	left_indexing_limit_lower = max(left_indexing_limit_lower, 0);

	right_indexing_limit_upper = min(right_indexing_limit_lower+max_entries_to_show, deal_get_length(shown_entries_right));
	left_indexing_limit_upper = min(left_indexing_limit_lower+max_entries_to_show, deal_get_length(shown_entries_left));

	// right_indexing_limit_lower = min(right_indexing_limit_lower, right_indexing_limit_upper);
	// left_indexing_limit_lower = min(left_indexing_limit_lower, left_indexing_limit_upper);
}

static inline void sel_y_bounds_check()
{
	index_limits_bounds_check();
	int32_t lower_index_limit_cy = (selector_on_right) ? right_indexing_limit_lower : left_indexing_limit_lower;
	int32_t upper_index_limit_cy = (selector_on_right) ? right_indexing_limit_upper : left_indexing_limit_upper;

	int32_t lower_index_limit_py = (prev_selector_right) ? right_indexing_limit_lower : left_indexing_limit_lower;
	int32_t upper_index_limit_py = (prev_selector_right) ? right_indexing_limit_upper : left_indexing_limit_upper;

	int32_t limit_cy = min(upper_index_limit_cy, max_entries_to_show);
	int32_t limit_py = min(upper_index_limit_py, max_entries_to_show);

	bool above_cy = selector_y+lower_index_limit_cy>=limit_cy;
	bool below_cy = selector_y<0;

	bool above_py = prev_selector_y+lower_index_limit_py>=limit_py;
	bool below_py = prev_selector_y<0;

	if(above_cy || below_cy)
	{ selector_y = (below_cy) ? max(0, selector_y) : min(limit_cy-1, selector_y); }
	if(above_py || below_py)
	{ prev_selector_y = (below_py) ? max(0, prev_selector_y) : min(limit_py-1, prev_selector_y); }
}


static void set_entry_name(uint16_t from_top_index, bool on_left_side, char* to)
{
	lcdc1_region_set_current(rID, (on_left_side) ? 0 : right_side_offset, from_top_index*mainFont[1]);
	lcdc1_print_string(rID, (char*)to);
}
static void clear_respective_list(bool on_left_side)
{
	if(!on_left_side) { empty_entry_name[max_entry_name_length] = '\0'; }

	for(size_t i = 0; i < max_entries_to_show; i++)
	{
		set_entry_name(i, on_left_side, empty_entry_name);
		if(on_left_side) { draw_middle_line_segment(i); }
	}
	if(!on_left_side) { empty_entry_name[max_entry_name_length] = ' '; }

	lcdc1_reset_coords(rID);
}

static void print_all_entries_on_side(deal_t* root, bool on_left_side)
{
	static volatile entry_t *entry;

	if(!root) { return; }

	clear_respective_list(on_left_side);
	index_limits_bounds_check();
	int32_t upper_index_limit = (!on_left_side) ? right_indexing_limit_upper : left_indexing_limit_upper;
	int32_t lower_index_limit = (!on_left_side) ? right_indexing_limit_lower : left_indexing_limit_lower;

	for(int16_t i = lower_index_limit; i < upper_index_limit; i++)
	{
		entry = (entry_t*)deal_get(root, i);
		if(!entry || !entry->name)
		{
			print_and_cut_entry_name("--Null entry--", !on_left_side, selector_y+lower_index_limit==i);
			DEBUG_PRINT_ERR("Unable to get entry at index %d\n", i);
			continue;
		}
		print_and_cut_entry_name(entry->name, !on_left_side, selector_y+lower_index_limit==i && selector_on_right!=on_left_side );
		advance_print_y(on_left_side);
	}
}
static void rewrite_entry(bool on_left_side, uint16_t index)
{
	static char *name_indx; // Has to be static so the other core can access it
	deal_t *relevant_side = (on_left_side) ? shown_entries_left : shown_entries_right;
	uint16_t lower_index_limit = (on_left_side) ? left_indexing_limit_lower : right_indexing_limit_lower;

	if(!on_left_side) { empty_entry_name[max_entry_name_length] = '\0'; }
	set_entry_name(index, on_left_side, empty_entry_name);
	if(!on_left_side) { empty_entry_name[max_entry_name_length] = ' '; }

	lcdc1_region_set_current(rID, 0, index*mainFont[1]);
	// X does not interest us here, the print_and_cut_entry_name funtion sets it automatically, but we do have to specify a y value

	name_indx = ((entry_t*)deal_get(relevant_side, index+lower_index_limit))->name;
	DEBUG_PRINT("Rewriting entry \"%s\"\n", name_indx);
	print_and_cut_entry_name(name_indx, !on_left_side, (index==selector_y&&on_left_side==!selector_on_right));
	if(on_left_side) { draw_middle_line_segment(index); }
}

static inline void rewrite_2_entries_on_side(bool on_left_side, uint16_t index1, uint16_t index2)
{
	if(index1==index2) { return; }
	rewrite_entry(on_left_side, index1);
	rewrite_entry(on_left_side, index2);
}

static void move_selector_vertical_updown(bool up)
{
	deal_t *relative_side = (selector_on_right) ? shown_entries_right : shown_entries_left;

	int32_t lower_index_limit = (selector_on_right) ? right_indexing_limit_lower : left_indexing_limit_lower;
	int32_t upper_index_limit = (selector_on_right) ? right_indexing_limit_upper : left_indexing_limit_upper;

	int32_t limit = min(upper_index_limit, max_entries_to_show);

	bool threshold_passed_above = (selector_y<=0 && up);
	bool threshold_passed_below = (selector_y>=limit-1 && !up);

	bool can_scroll_up = lower_index_limit > 0;
	bool can_scroll_down = upper_index_limit < deal_get_length(relative_side);

	if(
		(
			(threshold_passed_above && can_scroll_up) ||
			(threshold_passed_below && can_scroll_down)
		) && !(lower_index_limit+max_entries_to_show > deal_get_length(relative_side))
	)
	{
		if(selector_on_right) {
			right_indexing_limit_lower += (threshold_passed_above && can_scroll_up) ? -1 : +1;
			right_indexing_limit_upper += (threshold_passed_above && can_scroll_up) ? -1 : +1;
		}else
		{
			left_indexing_limit_lower += (threshold_passed_above && can_scroll_up) ? -1 : +1;
			left_indexing_limit_upper += (threshold_passed_above && can_scroll_up) ? -1 : +1;
		}
		index_limits_bounds_check();
		sel_y_bounds_check();
		print_all_entries_on_side(shown_entries_right, !selector_on_right);
		return;
	}else if(!threshold_passed_above && up)
	{
		selector_y--;
	}else if(!threshold_passed_below && !up)
	{
		selector_y++;
	}
	sel_y_bounds_check();
	rewrite_2_entries_on_side(!selector_on_right, selector_y, prev_selector_y);
}

static inline void handle_selector_side_change(deal_t *relative_side, deal_t *relative_opposite_side, uint16_t lower_index_limit, bool is_expand)
{
	current_depth += (is_expand) ? 0 : ((selector_on_right) ? -1 : +1); // Adjust the depth based on the side we switch to
	// Basically the same as:
	// if(!selector_on_right && is_expand) { current_depth; }
	// if(selector_on_right && is_expand) { current_depth; }
	// if(!selector_on_right && !is_expand) { current_depth++; }
	// if(selector_on_right && !is_expand) { current_depth--; }

	if(current_depth<0) { current_depth = max(current_depth, 0); }
	if(current_depth>=max_entry_depth) { current_depth = min(current_depth, max_entry_depth); }

	// deal_t *side = (!is_expand) ? relative_opposite_side : relative_side;
	selector_y = (deal_get(relative_opposite_side, lower_index_limit+selector_y)) ?
		selector_y :
		(deal_get_length(relative_opposite_side)>lower_index_limit+selector_y) ?
			selector_y :
			deal_get_length(relative_opposite_side)-1
	;
	selector_on_right = !selector_on_right;
	sel_y_bounds_check();

	// Path tracking
	if(selector_on_right)
	{
		deal_pop_end(&entry_path);
		deal_add(&entry_path, last_branch);
		deal_add(&entry_path, get_currently_selected_entry());
	}else
	{
		deal_pop_end(&entry_path);
		deal_pop_end(&entry_path);
		deal_add(&entry_path, get_currently_selected_entry());
	}
}


static bool can_expand_entry_on_right(entry_t *candidate_expander)
{
	if(current_depth+1 > max_entry_depth) { return false; }
	if(!selector_on_right) { return false; } // Since we would only switch sides, not scroll

	if(!candidate_expander) { return false; }
	if(candidate_expander->type!=BRANCH) { return false; }
	if(!candidate_expander->value.p) { return false; }

	return true;
}

static void expand_entry_on_right(entry_t *subject)
{
	if(!can_expand_entry_on_right(subject)) { return; }

	deal_add(&offscreen_entries_left, (void *)shown_entries_left);
	shown_entries_left = shown_entries_right;
	shown_entries_right = (deal_t *)subject->value.p;

	left_indexing_limit_lower = right_indexing_limit_lower;
	left_indexing_limit_upper = right_indexing_limit_upper;

	right_indexing_limit_lower = 0;
	right_indexing_limit_upper = deal_get_length(shown_entries_right);

	index_limits_bounds_check();
}
static bool can_return_entry_on_left()
{
	if(current_depth-1 < 0) { return false; }
	if(selector_on_right) { return false; } // Since we would only switch sides, not scroll

	const size_t length_past = deal_get_length(&offscreen_entries_left);
	if(length_past <= 0) { return false; }

	if(!deal_get(&offscreen_entries_left, length_past-1)) { return false; }

	return true;
}
static void return_entry_on_left()
{
	if(!can_return_entry_on_left()) { return; }

	shown_entries_right = shown_entries_left;
	shown_entries_left = (deal_t*)deal_pop_end(&offscreen_entries_left);

	DEBUG_ASSERT(shown_entries_left);

	right_indexing_limit_lower = left_indexing_limit_lower;
	right_indexing_limit_upper = left_indexing_limit_upper;

	left_indexing_limit_lower = 0;
	left_indexing_limit_upper = deal_get_length(shown_entries_left);

	index_limits_bounds_check();
}

static void kbd_task(void)
{
	static int key;
	static int bat;
	char buf[LCD_WIDTH/mainFont[0]];
	char *msg = NULL;

	DEBUG_PRINT("Keyboard task launched | Starting loop\n");
	for(;;)
	{
		key = read_i2c_kbd();
		bat = read_battery();
		if(key == -1 || key < 0) { tight_loop_contents(); continue; }

		deal_t *relative_side = (selector_on_right) ? shown_entries_right : shown_entries_left;
		deal_t *relative_opposite_side = (!selector_on_right) ? shown_entries_right : shown_entries_left;

		int32_t upper_index_limit = (selector_on_right) ? right_indexing_limit_upper : left_indexing_limit_upper;
		int32_t lower_index_limit = (selector_on_right) ? right_indexing_limit_lower : left_indexing_limit_lower;

		prev_selector_y = selector_y;
		prev_selector_right = selector_on_right;

		sel_y_bounds_check();

		entry_t *selected_entry = (entry_t*)deal_get(relative_side, lower_index_limit+selector_y);

		if(selected_entry->type==BRANCH && selected_entry != last_branch) { last_branch = selected_entry; }

		bool is_expand = false;

		DEBUG_PRINT("Key pressed | Evaluating\n");
evaluate_key:
		switch(key)
		{
			case KEY_DOWN:
				move_selector_vertical_updown(false);

				deal_pop_end(&entry_path);
				deal_add(&entry_path, get_currently_selected_entry());
				break;

			case KEY_UP:
				move_selector_vertical_updown(true);

				deal_pop_end(&entry_path);
				deal_add(&entry_path, get_currently_selected_entry());
			break;

			case KEY_LEFT:
				if(!can_return_entry_on_left())
				{
					handle_selector_side_change(relative_side, relative_opposite_side, lower_index_limit, false);

					rewrite_entry(!selector_on_right, selector_y);
					rewrite_entry(!prev_selector_right, prev_selector_y);
					break;
				}
				return_entry_on_left();

				relative_side = (selector_on_right) ? shown_entries_right : shown_entries_left;
				relative_opposite_side = (!selector_on_right) ? shown_entries_right : shown_entries_left;

				index_limits_bounds_check();
				upper_index_limit = (selector_on_right) ? right_indexing_limit_upper : left_indexing_limit_upper;
				lower_index_limit = (selector_on_right) ? right_indexing_limit_lower : left_indexing_limit_lower;
				handle_selector_side_change(relative_side, relative_opposite_side, lower_index_limit, true);

				print_all_entries_on_side(shown_entries_left, true);
				print_all_entries_on_side(shown_entries_right, false);
			break;

			case KEY_RIGHT:
				if(!can_expand_entry_on_right(selected_entry))
				{
					handle_selector_side_change(relative_side, relative_opposite_side, lower_index_limit, false);

					rewrite_entry(!selector_on_right, selector_y);
					rewrite_entry(!prev_selector_right, prev_selector_y);
					break;
				}
				expand_entry_on_right(selected_entry);

				relative_side = (selector_on_right) ? shown_entries_right : shown_entries_left;
				relative_opposite_side = (!selector_on_right) ? shown_entries_right : shown_entries_left;

				index_limits_bounds_check();
				upper_index_limit = (selector_on_right) ? right_indexing_limit_upper : left_indexing_limit_upper;
				lower_index_limit = (selector_on_right) ? right_indexing_limit_lower : left_indexing_limit_lower;
				handle_selector_side_change(relative_side, relative_opposite_side, lower_index_limit, true);

				print_all_entries_on_side(shown_entries_right, false);
				print_all_entries_on_side(shown_entries_left, true);
			break;

			case KEY_ENTER:
				switch(selected_entry->type)
				{
					case LEAF: break;
					case BRANCH:
						if(selector_on_right) { key=KEY_RIGHT; goto evaluate_key; }
						else if(!selector_on_right) { key=KEY_LEFT; goto evaluate_key; }
					break;
					case FUNCTIONABLE:
						if(!selected_entry->value.action) { break; }
						if(selected_entry->exits)
						{
							if(msg) { free(msg); }
							splitter_free_everything();
						}
						selected_entry->value.action();
					break;
					case SETTING: break;
				}
			break;
		}

		// if(bat != -1)
		// {
		// 	bat >>= 8;
		// 	int bat_charging = (bat>>7)&0x01;
		// 	bat &= 0b01111111; // 0x7F;
  //
		// 	if(bat_charging == 0)
		// 		snprintf(bat_buf, sizeof(bat_buf), "Battery at %d%% (not charging)", bat);
		// 	else
		// 		snprintf(bat_buf, sizeof(bat_buf), "Battery at %d%% (charging)", bat);
		// 	set_status_message(bat_buf);
		// }

		// draw_middle_line();
		draw_middle_line_segment(selector_y);
		draw_middle_line_segment(prev_selector_y);

		// We need to resync the relative sides in case that they have changed
		relative_side = (selector_on_right) ? shown_entries_right : shown_entries_left;
		relative_opposite_side = (!selector_on_right) ? shown_entries_right : shown_entries_left;
		selected_entry = (entry_t*)deal_get(relative_side, lower_index_limit+selector_y);

		// snprintf(buf, sizeof(buf), "sel_Y|depth|on_right  %d | %d | %d", selector_y, current_depth, selector_on_right);
		// set_status_message(buf);

		// Fancy stuff
		if(selected_entry && selected_entry->type==BRANCH && !selector_on_right
			&& (
				(prev_selector_y==selector_y && prev_selector_right!=selector_on_right) ||
				(prev_selector_y!=selector_y && prev_selector_right==selector_on_right)
				// Could also be written as: " (prev_selector_y==selector_y)^(prev_selector_right==selector_on_right) " but nah
			) // Only if we actually selected something new on the left side

			&& ((deal_t*)selected_entry->value.p!=shown_entries_right||!shown_entries_right)
			// Only if the new list is actually new
		)
		{
			shown_entries_right = (deal_t*)(selected_entry->value.p);
			right_indexing_limit_lower = 0;
			right_indexing_limit_upper = min(deal_get_length(shown_entries_right), max_entries_to_show);

			// relative_opposite_side = (!selector_on_right) ? shown_entries_right : shown_entries_left;
			print_all_entries_on_side(shown_entries_right, selector_on_right);
		}
		index_limits_bounds_check();
		if(msg) { free(msg); }
		msg = get_past_entries_filepath_style(1024);
		set_status_message(msg);
	}
}


// Main code:

uint16_t splitter_init(int rIDgiven, int maxDepth)
{
	DEBUG_PRINT("INITED STARTED\n");
	if(lcd_get_region_height(rIDgiven)<mainFont[1]*2 || lcd_get_region_width(rIDgiven)<(mainFont[0]*3)+1)
	{ DEBUG_PRINT_ERR("Given area does not meet minimum criteria\n"); return 0; }

	DEBUG_PRINT("Enabling score1\n");
	enable_core1_lcdspi();

	DEBUG_PRINT("Setting region settings\n");
	lcd_region_set_asthetics(rIDgiven, ORANGE, BLACK, 1, SHIFT_DOWNWARDS, (unsigned char *)LuOS_System_Font_data);
	lcdc1_region_clear(rIDgiven);
	rID = rIDgiven;

	DEBUG_PRINT("Calculating character offsets\n");
	divider_width = (int) (lcdc1_get_region_width(rIDgiven)%mainFont[0]);
	divider_width = (int)( (divider_width==0) ? mainFont[0] : divider_width );

	right_side_offset = (int)(  (lcdc1_get_region_width(rIDgiven) - divider_width) / 2 ) + divider_width;

	max_entry_name_length = (int)( (right_side_offset - divider_width) / mainFont[0] );
	max_entries_to_show = (int)(lcdc1_get_region_height(rIDgiven)/mainFont[1])-1;
	// The -1 is such that we have a space at the bottom for a little bit of a status message

	max_entry_depth = (maxDepth>0) ? maxDepth : 2;
	message_y_start = (max_entries_to_show*mainFont[1]);

	DEBUG_PRINT("Setting empty entry\n");
	empty_entry_name = calloc(max_entry_name_length+2, sizeof(char));
	if(!empty_entry_name)
	{
		DEBUG_PRINT_ERR("Empty entry was not allocated - aborting\n");
		return 0;
	}
	memset((char*)empty_entry_name, ' ', max_entry_name_length+1);
	empty_entry_name[max_entry_name_length+1] = '\0';
	// clean_string((char*)empty_entry_name, max_entry_name_length);

	DEBUG_PRINT("Allocating misc. string buffers\n");
	edit_print = calloc(lcd_get_region_width(rID)/mainFont[0], sizeof(char));
	last_msg = calloc(lcd_get_region_width(rID)/mainFont[0], sizeof(char));
	if(!edit_print)
	{
		DEBUG_PRINT_ERR("edit_print was not allocated - aborting\n");
		return 0;
	}
	if(!last_msg)
	{
		DEBUG_PRINT_ERR("last_msg was not allocated - aborting\n");
		return 0;
	}

	inited = 1;
	DEBUG_PRINT("Initing i2ckbd\n");
	init_i2c_kbd();

	DEBUG_PRINT("Displaying first pixels\n");

	set_status_message("Splitter Inited\0");
	DEBUG_PRINT("INITED DONE\n");
	return ((uint16_t)max_entry_name_length<<8)|(uint16_t)max_entries_to_show;
}

static void w_reboot_wrap(void) { watchdog_reboot(0, 0, 0); return; }

void splitter_start(void)
{
	DEBUG_PRINT("Starting splitter\n");
	if(!inited) { DEBUG_PRINT_ERR("Attempted to start splitter without initialization\n"); return; }

	// Some code I used to test/debug this whole library

	// static entry_t e1 = {"e1_entry entry entry entry", BRANCH}; deal_add(&root_entries, (void*)&e1);
	// static deal_t e1_ent; e1.value_p = (void*)&e1_ent;
	// 	static entry_t e1e1 = {"e1e1_entry", LEAF}; deal_add(&e1_ent, (void*)&e1e1);
	// 	static entry_t e1e2 = {"e1e2_entry", FUNCTIONABLE}; deal_add(&e1_ent, (void*)&e1e2); e1e2.action = w_reboot_wrap;
	// 	static entry_t e1e3 = {"e1e3_entry", LEAF}; deal_add(&e1_ent, (void*)&e1e3);
 //
	// static entry_t e2 = {"e2_entry", BRANCH}; deal_add(&root_entries, (void*)&e2);
	// static deal_t e2_ent; e2.value_p = (void*)&e2_ent;
	// 	static entry_t e2e1 = {"e2e1_entry", LEAF}; deal_add(&e2_ent, (void*)&e2e1);
 //
	// static entry_t e3 = {"e3_entry", LEAF}; deal_add(&root_entries, (void*)&e3);
	// static entry_t e4 = {"e4_entry", LEAF}; deal_add(&root_entries, (void*)&e4);
 //

	if(!root_entries.data) { DEBUG_PRINT_ERR("Splitter started without any entries, returning from function\n"); return; }
	// If no first entry exists, we don't start anything.

	shown_entries_left = &root_entries;
	entry_t *first_selected = deal_get(&root_entries, 0);

	if(!first_selected) { DEBUG_PRINT_ERR("First entry in root_entries does not exist, returning from function\n"); return; }
	if(first_selected->type==BRANCH)
	{
		shown_entries_right = (deal_t*) ( (entry_t*)deal_get(&root_entries, 0) )->value.p;
	}
	deal_add(&entry_path, first_selected);

	DEBUG_PRINT("Checking bounds and printing all entries\n");
	index_limits_bounds_check();
	print_all_entries_on_side(shown_entries_left, true);
	print_all_entries_on_side(shown_entries_right, false);
	draw_middle_line();

	DEBUG_PRINT("Splitter started | Launching keyboard task\n");
	kbd_task();

	// This point should not be reached.
	DEBUG_PRINT_ERR("Keyboard task returned - rebooting\n");
	splitter_free_everything();
	watchdog_reboot(0, 0, 0);
}

/* TODO:
 * Literally everything with settings.
 * Fix race conditions
 * Fix that random underline
*/
