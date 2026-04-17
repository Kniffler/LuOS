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

#define USE_DEAL_VOID_P
// #define DEAL_FREE_DATA() (free(node->data.value_p)) // This is a surprise tool that'll help us later!
#include "DEAL.h"

#include "keyboard_define.h"
#include "lcdspi_core1_wrap.h"

#define CENTER_EACH_ENTRY 1

// Display stuff
static uint8_t max_entry_name_length = 0;
static uint8_t max_entries_to_show = 0;
static uint8_t max_entry_depth = 0;
static char *empty_entry_name;

static uint16_t right_side_offset = 0;
static uint8_t divider_width = 0;

// Display stuff, but this time for functionality
static deal_t root_entries;
static deal_t *shown_entries_right;
static deal_t *shown_entries_left;
static deal_t past_entries;

static uint16_t right_indexing_limit_upper = 0;
static uint16_t right_indexing_limit_lower = 0;

static uint16_t left_indexing_limit_upper = 0;
static uint16_t left_indexing_limit_lower = 0;

static bool selector_on_right = 0; // Can only be on the left or right
static uint8_t selector_y = 0;

static int16_t current_depth = 0;

static bool in_setting_editor = 0;
static uint16_t message_y_start = 0;
static char *edit_print;

// Misc.
static int rID;

const unsigned char *mainFont = (unsigned char*)LuOS_System_Font_data;
static bool inited = 0;

// Functions

static void clean_everything()
{
	free(empty_entry_name);
	free(edit_print);
	deal_free(&root_entries);
}

static void clean_message(char *msg, size_t size_limit)
{
	for(int i = 0; i < strlen(msg); i++)
	{
		if(i >= size_limit) { msg[i] = '\0'; }
		switch(msg[i])
		{
			case '\n':
			case '\v':
			case '\f':
			case '\r':
				for(int k = i; k < strlen(msg)-1; k++)
				{
					if(!msg[i+1]) { break; }
					msg[i] = msg[i+1];
				}
			break;
		}
	}
}

void set_status_message(char *to)
{
	if(in_setting_editor) { return; }

	int16_t temp_x = lcdc1_get_current_x(rID);
	int16_t temp_y = lcdc1_get_current_y(rID);

	lcdc1_region_set_current(rID, 0, message_y_start);

	int16_t name_limit = lcdc1_get_region_width(rID)/mainFont[0];

	clean_message(to, name_limit);

	for(uint8_t i = 0; i < name_limit-strlen(to)-1; i++)
	{
		lcdc1_putc(rID, ' ');
	}

	lcdc1_region_set_current(rID, temp_x, temp_y);
}

static bool validate_input_for_setting(char * input, entry_type_t setting)
{
	if(setting==LEAF || setting==BRANCH || setting==FUNCTIONABLE) { return false; }
	return true;
}

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
		validate_input_for_setting(edit_print, to_edit->type);
	}
}

static void draw_middle_line()
{
	// We, most literally, draw a line here. \xDB maps to character 219, which maps to a fully filled bitmap.
	// \xCD basically just maps to a pipe that shows us at what height the selector is
	// The space then clears any overflow.
	// lcdc1_region_set_current(rID, right_side_offset-divider_width, 0);

	uint16_t lower_index_limit = (selector_on_right) ? right_indexing_limit_lower : left_indexing_limit_lower;
	deal_t *relative_side = (selector_on_right) ? shown_entries_right : shown_entries_left;
	entry_t *selected_entry = (entry_t*)deal_get(relative_side, lower_index_limit+selector_y);

	for(int i = 0; i < max_entries_to_show; i++)
	{
		lcdc1_region_set_current(rID, right_side_offset-divider_width, i*mainFont[1]);

		if(divider_width<8)
		{
			lcdc1_putc(rID, (i==selector_y) ? '\xCD' : '\xDB'); // ASCII(219)
			lcdc1_region_set_current(rID, right_side_offset, i*mainFont[1]);
			lcdc1_putc(rID, ' ');

		}else if (i!=selector_y) { lcdc1_putc(rID, '\xBA'); }
		// Alternative options, in case we have a perfect 1 character space to play with:
		// 254 FE
		// 186 BA <-
		// 179 B3

		// If the selector is currently on our height: \xCD
		else
		{
			if(selected_entry->type==BRANCH) { lcdc1_putc(rID, (selector_on_right) ? '\xF2' : '\xF3'); }
			else { lcdc1_putc(rID, (selector_on_right) ? '>' : '<'); }
		}

	}
	lcdc1_reset_coords(rID);
}

static void advance_print_y(bool on_left_side) { lcdc1_region_set_current(rID, lcdc1_get_current_x(rID), lcdc1_get_current_y(rID)+mainFont[1]); }

static void print_and_cut_entry_name(char *entry_name, bool isOnRight, bool highlight)
{
	if(!entry_name) { return; }

	// if(strlen(entry_name)<=max_entry_name_length)
	// {
	// 	if(highlight) { lcdc1_putc(rID, '\017'); }
	// 	lcdc1_print_string(rID, entry_name);
	// 	if(highlight) { lcdc1_putc(rID, '\016'); }
 //
	// 	return;
	// }

	char limited_name[max_entry_name_length+1];
	strncpy(limited_name, entry_name, max_entry_name_length);
	limited_name[max_entry_name_length] = '\0';
	#ifndef CENTER_EACH_ENTRY
	lcdc1_region_set_current(rID, (isOnRight) ? right_side_offset : 0, lcdc1_get_current_y(rID));
	#else
	lcdc1_region_set_current(rID,
			((isOnRight) ? right_side_offset : 0) + ( ((right_side_offset-divider_width) - strlen(limited_name)*mainFont[0])/2 ),
			lcdc1_get_current_y(rID));
	#endif
	if(highlight) { lcdc1_putc(rID, '\017'); } // These highlight characters do not affect the current print coords
	lcdc1_print_string(rID, limited_name);
	if(highlight) { lcdc1_putc(rID, '\016'); }
}

static void update_indexing_limits()
{
	right_indexing_limit_lower = MAX(right_indexing_limit_lower, 0);
	left_indexing_limit_lower = MAX(left_indexing_limit_lower, 0);

	right_indexing_limit_upper = MIN(right_indexing_limit_lower+max_entries_to_show, deal_get_length(shown_entries_right));
	left_indexing_limit_upper = MIN(right_indexing_limit_lower+max_entries_to_show, deal_get_length(shown_entries_left));
}

static void set_entry_name(uint16_t from_top_index, bool leftSide, char* to)
{
	lcdc1_region_set_current(rID, (leftSide) ? 0 : right_side_offset, from_top_index*mainFont[1]);
	lcdc1_print_string(rID, to);
}
static void clear_respective_list(bool leftSide)
{
	for(uint16_t i = 0; i < max_entries_to_show; i++)
	{
		set_entry_name(i, leftSide, empty_entry_name);
	}
	lcdc1_reset_coords(rID);
}

static void print_all_entries_on_side(deal_t* root, uint16_t depth, bool on_left_side)
{
	clear_respective_list(on_left_side);
	update_indexing_limits();
	uint16_t upper_index_limit = (!on_left_side) ? right_indexing_limit_upper : left_indexing_limit_upper;
	uint16_t lower_index_limit = (!on_left_side) ? right_indexing_limit_lower : left_indexing_limit_lower;
	if(!root || depth > max_entry_depth) { return; }

	for(int16_t i = lower_index_limit; i < upper_index_limit; i++)
	{
		entry_t *entry = (entry_t*)deal_get(root, i);
		if(!entry) { print_and_cut_entry_name("--Null entry--", !on_left_side, selector_y+lower_index_limit==i); continue; }
		print_and_cut_entry_name(entry->name, !on_left_side, selector_y+lower_index_limit==i && selector_on_right==!on_left_side );
		advance_print_y(on_left_side);
	}
}
static void rewrite_entry(bool on_left_side, uint16_t index)
{
	deal_t *relevant_side = (on_left_side) ? shown_entries_left : shown_entries_right;
	uint16_t lower_index_limit = (on_left_side) ? left_indexing_limit_lower : right_indexing_limit_lower;

	set_entry_name(index, on_left_side, empty_entry_name);
	lcdc1_region_set_current(rID, 0, index*mainFont[1]);
	// X does not interest us here, the print_and_cut_entry_name funtions sets it automatically, but we do have to specify a y value

	char *name_indx = ((entry_t*)deal_get(relevant_side, index+lower_index_limit))->name;
	print_and_cut_entry_name(name_indx, !on_left_side, (index==selector_y&&on_left_side==!selector_on_right));
}

static void rewrite_2_entries_on_side(bool on_left_side, uint16_t index1, uint16_t index2)
{
	rewrite_entry(on_left_side, index1);
	rewrite_entry(on_left_side, index2);
}

static bool can_expand_entry_on_right(entry_t *candidate_expander)
{
	if(current_depth+1 > max_entry_depth) { return false; }
	if(!selector_on_right) { return false; } // Since we would only switch sides, not scroll

	if(!candidate_expander) { return false; }
	if(candidate_expander->type!=BRANCH) { return false; }
	if(!candidate_expander->value_p) { return false; }

	return true;
}

static void expand_entry_on_right(entry_t *subject)
{
	if(!can_expand_entry_on_right(subject)) { return; }

	deal_push(&past_entries, (void *)shown_entries_left);
	shown_entries_left = shown_entries_right;
	shown_entries_right = (deal_t *)subject->value_p;
	current_depth++;

	left_indexing_limit_lower = right_indexing_limit_lower;
	left_indexing_limit_upper = right_indexing_limit_upper;
	right_indexing_limit_lower = 0;
	right_indexing_limit_upper = deal_get_length(shown_entries_right);
	update_indexing_limits();
}
static bool can_return_entry_on_left()
{
	if(current_depth-1 < 0) { return false; }
	if(selector_on_right) { return false; } // Since we would only switch sides, not scroll

	const size_t length_past = deal_get_length(&past_entries);
	if(length_past <= 0) { return false; }

	if(!deal_get(&past_entries, length_past-1)) { return false; }

	return true;
}
static void return_entry_on_left()
{
	if(!can_return_entry_on_left()) { return; }

	shown_entries_right = shown_entries_left;
	shown_entries_left = deal_pop_end(&past_entries);
	current_depth--;

	right_indexing_limit_lower = left_indexing_limit_lower;
	right_indexing_limit_upper = left_indexing_limit_upper;
	left_indexing_limit_lower = 0;
	left_indexing_limit_upper = deal_get_length(shown_entries_left);
	update_indexing_limits();
}

static void handle_selector_side_change(deal_t *relative_side, deal_t *relative_opposite_side, uint16_t lower_index_limit, uint8_t prev_selector_y)
{
	current_depth += (selector_on_right) ? -1 : 1; // Adjust the depth based on the side we switch to
	if(current_depth<0) { current_depth = MAX(current_depth, 0); return; }

	selector_y = (deal_get(relative_opposite_side, lower_index_limit+selector_y)) ?
		selector_y :
		(deal_get_length(relative_opposite_side)>lower_index_limit+selector_y) ?
			selector_y :
			deal_get_length(relative_opposite_side)-1
	;

	selector_on_right = !selector_on_right;
}

static void kbd_task(void)
{
	static int key;
	static int bat;
	char bat_buf[LCD_WIDTH/mainFont[0]];
	for(;;)
	{
		key = read_i2c_kbd();
		bat = read_battery();
		if(key == -1 || key < 0) { sleep_ms(40); continue; }

		deal_t *relative_side = (selector_on_right) ? shown_entries_right : shown_entries_left;
		deal_t *relative_opposite_side = (!selector_on_right) ? shown_entries_right : shown_entries_left;

		uint16_t upper_index_limit = (selector_on_right) ? right_indexing_limit_upper : left_indexing_limit_upper;
		uint16_t lower_index_limit = (selector_on_right) ? right_indexing_limit_lower : left_indexing_limit_lower;

		uint8_t prev_selector_y = selector_y;
		uint8_t prev_selector_right = selector_on_right;

		entry_t *selected_entry = (entry_t*)deal_get(relative_side, lower_index_limit+selector_y);

		char buf[512];

evaluate_key:
		switch(key)
		{
			case KEY_DOWN:
				// lcdc1_reset_coords(rID);
				// lcdc1_print_string(rID, "Found me keys\n");
				if(selector_y+lower_index_limit+1<deal_get_length(relative_side))
				{
					selector_y++;
					rewrite_2_entries_on_side(!selector_on_right, selector_y, selector_y-1);
				}
				if(selector_y+lower_index_limit>=upper_index_limit)
				{
					selector_y = upper_index_limit-1;
					(selector_on_right) ? right_indexing_limit_upper++ : left_indexing_limit_upper++;
					(selector_on_right) ? right_indexing_limit_lower++ : left_indexing_limit_lower++;
					update_indexing_limits();
				}
			break;

			case KEY_UP:
				if(selector_y+lower_index_limit-1<deal_get_length(relative_side) && selector_y+lower_index_limit-1>=0)
				{
					selector_y--;
					rewrite_2_entries_on_side(!selector_on_right, selector_y, selector_y+1);
				}
				if(selector_y<0)
				{
					selector_y = 0;
					(selector_on_right) ? right_indexing_limit_upper-- : left_indexing_limit_upper--;
					(selector_on_right) ? right_indexing_limit_lower-- : left_indexing_limit_lower--;
					update_indexing_limits();
				}
			break;

			case KEY_LEFT:
				if(can_return_entry_on_left())
				{
					return_entry_on_left();
					selector_on_right = !selector_on_right;
					print_all_entries_on_side(shown_entries_right, current_depth, false);
					print_all_entries_on_side(shown_entries_left, current_depth, true);
				}else if(selector_on_right)
				{
					handle_selector_side_change(relative_side, relative_opposite_side, lower_index_limit, prev_selector_y);
					rewrite_entry(!selector_on_right, selector_y);
					rewrite_entry(selector_on_right, prev_selector_y);
				}
				// selector_y = MIN(selector_y, upper_index_limit-1);
			break;

			case KEY_RIGHT:
				if(can_expand_entry_on_right(selected_entry))
				{
					expand_entry_on_right(selected_entry);
					selector_on_right = !selector_on_right;
					print_all_entries_on_side(shown_entries_right, current_depth, false);
					print_all_entries_on_side(shown_entries_left, current_depth, true);
				}else if(!selector_on_right)
				{
					handle_selector_side_change(relative_side, relative_opposite_side, lower_index_limit, prev_selector_y);
					rewrite_entry(!selector_on_right, selector_y);
					rewrite_entry(selector_on_right, prev_selector_y);
				}

				// selector_y = MIN(selector_y, upper_index_limit-1);
			break;

			case KEY_ENTER:
				switch(selected_entry->type)
				{
					case LEAF: break;
					case BRANCH:
						if(selector_on_right) { expand_entry_on_right(selected_entry); }
						else // if(selected_entry->value_p)
						{
							// shown_entries_right = (deal_t *)selected_entry->value_p;
							// This case will be handled automatically at the end of the loop.
						}
					break;
					case FUNCTIONABLE:
						if(selected_entry->exits) { clean_everything(); };
						selected_entry->action();
					break;
					case SETTING: break;
				}
				// if(selected_entry->exits) { clean_everything(); }
			break;
		}
		if(bat != -1)
		{
			bat >>= 8;
			int bat_charging = (bat>>7)&0x01;
			bat &= 0b01111111; // 0x7F;

			if(bat_charging == 0)
				snprintf(bat_buf, sizeof(bat_buf), "Battery at %d%% (not charging)", bat);
			else
				snprintf(bat_buf, sizeof(bat_buf), "Battery at %d%% (charging)", bat);
			set_status_message(bat_buf);
		}
		draw_middle_line();

		// We need to resync the relative sides in case that they have changed
		relative_side = (selector_on_right) ? shown_entries_right : shown_entries_left;
		relative_opposite_side = (!selector_on_right) ? shown_entries_right : shown_entries_left;
		selected_entry = (entry_t*)deal_get(relative_side, lower_index_limit+selector_y);

		if(selected_entry && selected_entry->type==BRANCH && !selector_on_right
			&& (
				(prev_selector_y==selector_y && prev_selector_right!=selector_on_right) ||
				(prev_selector_y!=selector_y && prev_selector_right==selector_on_right)
				// Could also be written as: (prev_selector_y==selector_y)^(prev_selector_right==selector_on_right)
			) // Only if we actually selected something new on the left side

			&& ((deal_t*)selected_entry->value_p!=shown_entries_right||!shown_entries_right)
			// Only if the new list is actually new
		)
		{
			shown_entries_right = (deal_t*)(selected_entry->value_p);
			relative_opposite_side = (!selector_on_right) ? shown_entries_right : shown_entries_left;
			print_all_entries_on_side(relative_opposite_side, current_depth+1, selector_on_right);
		}
		update_indexing_limits();
	}
	watchdog_reboot(0, 0, 0);
}




uint16_t splitter_init(int rIDgiven, int max_depth)
{
	if(lcd_get_region_height(rIDgiven)<mainFont[1]*2 || lcd_get_region_width(rIDgiven)<(mainFont[0]*2)+1)
	{ return 0; }

	enable_core1_lcdspi();

	lcd_region_set_asthetics(rIDgiven, ORANGE, BLACK, 1, SHIFT_DOWNWARDS, (unsigned char *)LuOS_System_Font_data);
	lcdc1_region_clear(rIDgiven);
	rID = rIDgiven;

	divider_width = (int) (lcdc1_get_region_width(rIDgiven)%mainFont[0]);
	divider_width = (int)( (divider_width==0) ? mainFont[0] : divider_width );

	right_side_offset = (int)(  (lcdc1_get_region_width(rIDgiven) - divider_width) / 2 ) + divider_width;

	max_entry_name_length = (int)( (right_side_offset - divider_width) / mainFont[0] );
	max_entries_to_show = (int)(lcdc1_get_region_height(rIDgiven)/mainFont[1])-1;
	// The -1 is such that we have a space at the bottom for a little bit of a status message

	max_entry_depth = (max_depth>0) ? max_depth : 2;
	message_y_start = (max_entries_to_show*mainFont[1]);

	empty_entry_name = calloc(max_entry_name_length+2, sizeof(char));
	memset(empty_entry_name, ' ', max_entry_name_length+1); // Plus one since there could be a few pixels left over
	empty_entry_name[max_entry_name_length+1] = '\0';

	edit_print = calloc(lcd_get_region_width(rID)/mainFont[0], sizeof(char));

	inited = 1;
	init_i2c_kbd();

	// set_status_message("Successfully created Splitter interface.\0");

	return (max_entry_name_length<<16)|max_entries_to_show;
}

static void w_reboot_wrap(void) { watchdog_reboot(0, 0, 0); return; }

void splitter_start(void)
{
	if(!inited) { return; }

	// static entry_t e1 = {"e1_entry entry entry entry", BRANCH}; deal_push(&root_entries, (void*)&e1);
	// static deal_t e1_ent; e1.value_p = (void*)&e1_ent;
	// 	static entry_t e1e1 = {"e1e1_entry", LEAF}; deal_push(&e1_ent, (void*)&e1e1);
	// 	static entry_t e1e2 = {"e1e2_entry", FUNCTIONABLE}; deal_push(&e1_ent, (void*)&e1e2); e1e2.action = w_reboot_wrap;
	// 	static entry_t e1e3 = {"e1e3_entry", LEAF}; deal_push(&e1_ent, (void*)&e1e3);
 //
	// static entry_t e2 = {"e2_entry", BRANCH}; deal_push(&root_entries, (void*)&e2);
	// static deal_t e2_ent; e2.value_p = (void*)&e2_ent;
	// 	static entry_t e2e1 = {"e2e1_entry", LEAF}; deal_push(&e2_ent, (void*)&e2e1);
 //
	// static entry_t e3 = {"e3_entry", LEAF}; deal_push(&root_entries, (void*)&e3);
	// static entry_t e4 = {"e4_entry", LEAF}; deal_push(&root_entries, (void*)&e4);
 //
	// shown_entries_left = &root_entries;
	// shown_entries_right = &e1_ent;

	if(!root_entries.data) { return; }
	// If no first entry exists, we don't update anything.

	update_indexing_limits();
	print_all_entries_on_side(shown_entries_left, current_depth, true);
	print_all_entries_on_side(shown_entries_right, current_depth+1, false);
	draw_middle_line();
	kbd_task();
	watchdog_reboot(0, 0, 0);
}

// TODO: Literally everything with settings.
