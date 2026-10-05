#include <hardware/timer.h>
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
	DEBUG_PRINT("Inited SPI at %dMhz, while requested at %dMhz\n", rate/(1000*1000), LCD_SPI_FREQ/(1000*1000));
	lcd_clear(RGB565(0b11111, 0b000000, 0b00000));
	busy_wait_ms(1000);
	lcd_clear(RGB565(0b00000, 0b000000, 0b11111));
	busy_wait_ms(1000);
	lcd_clear(RGB565(0b00000, 0b111111, 0b00000));
	busy_wait_ms(1000);
	lcd_draw_rect(0, 160, 320, 160, RGB565(0b11111, 0b000000, 0b00000));
	lcd_draw_rect(0, 320, 320, 160, RGB565(0b11111, 0b000000, 0b11111));


	busy_wait_ms(1000);
	lcd_clear(0);
	// lcd_scroll(160);
	// font_t meFont = (unsigned char*)font1_data;
	font_t meFont = (unsigned char*)LuOS_System_Font_data;
	unsigned char str[] =  "\017 \016Hello W\bWorld. Hello W\bWorld. Hello W\bWorld. Hello W\bWorld. Hello W\bWorld. Hello W\bWorld.";
	unsigned char str2[] = " ";
	// char c = 'B';
	// lcd_draw_string_on_line(str, strlen(str), 0, 0, meFont, RGB565(0b11111, 0b000000, 0b11111), false);
	int x = 0, y = 0, c = -1;
	lcd_draw_string(str, strlen(str), x, y, meFont, 3, RGB565(0b11111, 0b000000, 0b11111), 0);
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
			case KEY_RIGHT: x++; break;C
		}
		if(y<0) { y += LCD_RES_V; }
		if(x<0) { x += LCD_RES_H; }
		y %= LCD_RES_V;
		x %= LCD_RES_H;
		DEBUG_PRINT("X: %d, Y: %d\n", x, y);
		lcd_clear(0);
		lcd_draw_string(str, strlen(str), x, y, meFont, 3, RGB565(0b11111, 0b000000, 0b11111), 0);
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

	for(;;) { tight_loop_contents(); }
}
