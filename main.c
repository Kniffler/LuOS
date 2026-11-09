#include <hardware/timer.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <pico/stdio.h>
#include <pico/stdlib.h>
#include <stdbool.h>
#include <string.h>
// #include "hardware/gpio.h"
#include "fonts/font1.h"
// #include "fonts/LuOS_System_Font.h"
#include "fonts/Bahnschrift-8x10.h"

#include "i2ckbd.h"

#include "buffy.h"
#include "config.h"

#include <pico/platform/common.h>
#include <pico/stdlib.h>

#include "src/include/debug.h"
#include "src/keyboard_define.h"
#include "ui.h"
#include "test_suite.h"


int main()
{
	stdio_init_all();
#ifdef WAIT_ON_FIRST_SERIAL_INPUT
	char buf[512];
	for(;;)
	{
		DEBUG_PRINT("PRESS ANY SERIALLY SENT KEY TO START DEVICE\n");
		char c = fgetc(stdin);
		if(c) { break; }
	}
#endif
#ifdef WAIT_ON_FIRST_KBD_INPUT
	init_i2c_kbd();
	for(;;)
	{
		DEBUG_PRINT("PRESS ANY KEY ON KEYBOARD TO START DEVICE\n");
		int c = read_i2c_kbd();
		busy_wait_ms(50);
		if(c!=-1) { break; }
	}
#endif

	DEBUG_PRINT("Program started\n");
	DEBUG_PRINT_ERR("MESSAGE ERROR TEST\n");

	int rate = lcd_init();
	uint16_t *lcd_buffer = lcd_buffer_create();
	DEBUG_PRINT("Inited SPI at %dMhz, while requested at %dMhz\n", rate/(1000*1000), LCD_SPI_FREQ/(1000*1000));


	font_t meFont = (unsigned char*)Bahnschrift_8x10_font_data;

	AGrid_t grid;
	grid.font = meFont;
	grid.tabsize = 3;
	grid.foreground = RGB565(255, 255, 255);
	grid.background = 0;
	grid.demo_glyph = 255;
	grid.index_offset = 0;
	grid.pixel_offset_x = 0;
	grid.pixel_offset_y = 0;
	grid.glyph_count_horizontal = LCD_RES_H/meFont[0];
	grid.glyph_count_vertical = LCD_RES_V/meFont[1];

	DEBUG_PRINT("Allowing for %d horizontal glyps and %d vertical ones\n", grid.glyph_count_horizontal, grid.glyph_count_vertical);
	enter_ui_index_tester(lcd_buffer, &grid, 1, 1);
	for(;;) { tight_loop_contents(); }
}
