#include "splitter.h"
#include <stdbool.h>
#include <string.h>
#include "fonts/LuOS_System_Font.h"
#include "lcdspi.h"
#include <pico/platform/common.h>
#include <pico/multicore.h>
#include <sys/unistd.h>
#include <pico/malloc.h>

#include <stdio.h>

static uint8_t max_entry_name_length = 0;
static uint8_t max_entries_per_list = 0;
static uint16_t right_side_offset = 0;

static uint8_t divider_width = 0;

static int rID;

static unsigned char *mainFont = (unsigned char*)LuOS_System_Font_data;
static bool inited = 0;

static void core1_lcd_print_receiver(void)
{
	// We, most literally, draw a line here. \xDB maps to character 219, which maps to a fully filled bitmap.
	// The space then clears any overflow.
	lcd_region_set_current(rID, right_side_offset-divider_width, 0);
	for(int i = 0; i < max_entries_per_list; i++)
	{
		lcd_region_set_current(rID, right_side_offset-divider_width, i*mainFont[1]);

		if(divider_width>0)
		{
			lcd_putc(rID, 0, '\xDB'); // ASCII(219)
			lcd_region_set_current(rID, right_side_offset, i*mainFont[1]);
			lcd_putc(rID, 0, ' ');
		}else { lcd_putc(rID, 0, '\xBA'); }
		// Alternative options, in case we have a perfect 1 character space to play with:
		// 254 FE
		// 186 BA <-
		// 179 B3
	}
	for(;;) {
		uint64_t strInt = (uint64_t)multicore_fifo_pop_blocking();
		strInt |= (uint64_t)multicore_fifo_pop_blocking()<<32;
		lcd_print_string(rID, (char*)strInt); // We will only ever print in this region.
	}
}

uint16_t splitter_init(int rIDgiven)
{
	if(lcd_get_region_height(rIDgiven)<mainFont[1]*2 || lcd_get_region_width(rIDgiven)<(mainFont[0]*2)+1)
	{ return 0; }

	lcd_region_set_asthetics(rIDgiven, ORANGE, BLACK, 1, SHIFT_DOWNWARDS, (unsigned char *)LuOS_System_Font_data);
	lcd_region_clear(rIDgiven);
	rID = rIDgiven;

	divider_width = (int) (lcd_get_region_width(rIDgiven)%mainFont[0]);

	right_side_offset = (int)(  (lcd_get_region_width(rIDgiven) - divider_width) / 2 ) + divider_width;

	max_entry_name_length = (int)( (right_side_offset - divider_width) / mainFont[0] ) - 2;
	// The -2 is only there so we can add spacing on both ends. It is purely asthetic and may be removed if you deem it ugly.
	max_entries_per_list = (int)(lcd_get_region_height(rIDgiven)/mainFont[1])-1;
	// The -1 is such that we have a space at the bottom for a little bit of a status message

	multicore_launch_core1(core1_lcd_print_receiver);
	/* Any and all text displaying will be done on core 1.
	 * This is done in order to have faster printing times and essentially just not block code execution - which
	 * may slow us down in the long run if we are trying to do calculations in the meantime. So long story short,
	 * this approach lets us use both cores in order to optimize speed and performance.
	*/
	inited = 1;
	sleep_ms(200);
	char s[256];
	lcd_reset_coords(rIDgiven);
	sprintf(s, "Size of a single struct here is: %d\n", sizeof(entry_t));
	lcd_print_string(rID, s);
	return (max_entry_name_length<<16)|max_entries_per_list;
}

void lcd_print_string_core1(int rID, char* str)
{
	if(!inited) { return; }

	// The inbuilt fifo only lets us send 32 bits at a time, so here I use 2 fifo entries to negate that
	// since the sizeof char* is roughly double that of a uint32_t. So a uint64_t.
	multicore_fifo_push_blocking( (uint32_t)( (uint64_t)str&UINT32_MAX) );
	multicore_fifo_push_blocking( (uint32_t)( ((uint64_t)str&((uint64_t)UINT32_MAX<<32))>>32 ) );
}

static void print_and_cut_entry_name(char entry[max_entry_name_length], bool isOnRight)
{
	if(strlen(entry)<=max_entry_name_length) { return; }
}

void print_all_entries()
{
}
