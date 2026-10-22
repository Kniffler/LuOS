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
#include "fonts/LuOS_System_Font.h"

#include "i2ckbd.h"

#include "buffy.h"
#include "config.h"

#include <pico/platform/common.h>
#include <pico/stdlib.h>

#include "src/include/debug.h"
#include "src/keyboard_define.h"
#include "ui.h"


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
	// uint16_t *lcd_buffer = lcd_buffer_create();
	DEBUG_PRINT("Inited SPI at %dMhz, while requested at %dMhz\n", rate/(1000*1000), LCD_SPI_FREQ/(1000*1000));

	busy_wait_ms(1000);

	font_t meFont = (unsigned char*)LuOS_System_Font_data;

	AGrid_t grid;
	grid.font = meFont;
	grid.tabsize = 3;
	grid.foreground = RGB565(255, 255, 255);
	grid.background = 0;
	grid.demo_glyph = 'O';
	grid.index_offset = 0;
	grid.pixel_offset_x = 0;
	grid.pixel_offset_y = 0;
	grid.glyph_count_horizontal = LCD_RES_H/meFont[0];
	grid.glyph_count_vertical = LCD_RES_V/meFont[1];

	unsigned char str[] =  "\017 \016Hello W\bWorld. Hello W\bWorld. Hello W\bWorld. Hello W\bWorld. Hello W\bWorld. Hello W\bWorld.";
	unsigned char str2[] = " ";
	int x = 0, y = 0, c = -1;
	ui_print_string(grid, x, str, strlen(str));
	init_i2c_kbd();
	for(;;)
	{
		c = read_i2c_kbd();
		if(c==-1) { continue; }
		switch(c)
		{
			case KEY_UP: y--; break;
			case KEY_DOWN: y++; break;
			case KEY_LEFT: x--; break;
			case KEY_RIGHT: x++; break;
		}
		if(y<0) { y += grid.glyph_count_vertical; }
		if(x<0) { x += grid.glyph_count_horizontal; }
		y %= grid.glyph_count_vertical;
		x %= grid.glyph_count_horizontal;
		lcd_clear(0);
		DEBUG_PRINT("X: %d, Y: %d | Index = %d\n", x, y, y*grid.glyph_count_horizontal + x);
		ui_print_string(grid, y*grid.glyph_count_horizontal + x, str, strlen(str));
	}
	// int i, k;
	// for(i = 311; i < LCD_RES_V; i+=1)
	// {
	// 	for(k = 0; k < LCD_RES_H; k++)
	// 	{
	// 		lcd_draw_string(str2, strlen(str2), k-( (k>meFont[0]) ? meFont[0] : 0), i, meFont, 3, RGB565(0b11111, 0b000000, 0b11111), 0);
	// 		lcd_draw_string(str, strlen(str), k, i, meFont, 3, RGB565(0b11111, 0b000000, 0b11111), 0);
	// 		busy_wait_ms(25);
	// 	}
	// 	lcd_clear(0);
	// }
	// lcd_draw_char(c, 0, 0, meFont, RGB565(0b11111, 0b000000, 0b11111));

	// busy_wait_ms(1000);
	// lcd_scroll(-160);

	// for(int i = 0; i < 160; i++)
	// {
	// 	DEBUG_PRINT("Scrolling 1 line: %d                                                                                         ", i);
	// 	lcd_scroll(1);
	// 	busy_wait_ms(1000);
	// }

	// free(lcd_buffer);
	for(;;) { tight_loop_contents(); }
}
