#include "test_suite.h"
#include "buffy.h"
#include "ui.h"

extern void enter_ui_index_tester(MBNA AGrid_t *grid, size_t x_step, size_t y_step)
{
	static unsigned char str[] =  "sphinx of black quartz judge my vow SPHINX OF BLACK QUARTZ JUDGE MY VOW 012345789 !\" #$%&/()=?'@§{[]}+*-_.:,;~^<> \017sphinx of black quartz judge my vow SPHINX OF BLACK QUARTZ JUDGE MY VOW 012345789 !\" #$%&/()=?'@§{[]}+*-_.:,;~^<>\016\0";
	static unsigned char str2[] = " ";

	static unsigned char command[512];

	int x = 0, y = 0, c = -1;
	bool grid_shift_mode = 0, term_test = 0;

	ui_print_string(MBNAP grid, x, str, strlen(str));
	lcd_refresh(lcd_buffer);
	init_i2c_kbd();
	for(;;)
	{
		c = read_i2c_kbd();
		if(c==-1) { continue; }

		if(grid_shift_mode)
		{
			switch(c)
			{
				case KEY_UP:	grid->pixel_offset_y--; break;
				case KEY_DOWN:	grid->pixel_offset_y++; break;
				case KEY_LEFT:	grid->pixel_offset_x--; break;
				case KEY_RIGHT:	grid->pixel_offset_x++; break;
				case 'g': grid_shift_mode = false; continue; break;
			}
			ui_show_grid_demo(MBNAP grid);
			lcd_refresh(lcd_buffer);
			continue;
		}
		if(term_test)
		{

			continue;
		}

		switch(c)
		{
			case KEY_UP:	y -= y_step; break;
			case KEY_DOWN:	y += y_step; break;
			case KEY_LEFT:	x -= x_step; break;
			case KEY_RIGHT:	x += x_step; break;
			case 'g':
				grid_shift_mode = true;
				ui_show_grid_demo(MBNAP grid);
				lcd_refresh(lcd_buffer);
				continue; break;

			case 't':
				ui_draw(MBNAP grid, true);

				break;
		}
		if(y<0) { y += grid->glyph_count_vertical; }
		if(x<0) { x += grid->glyph_count_horizontal; }
		y %= grid->glyph_count_vertical;
		x %= grid->glyph_count_horizontal;
		lcd_clear(MBNAP 0);
		// DEBUG_PRINT("X: %d, Y: %d | Index = %d\n", x, y, (y*grid->glyph_count_horizontal) + x);
		ui_print_string(MBNAP grid, (y*grid->glyph_count_horizontal) + x, str, strlen(str));
		// lcd_refresh(lcd_buffer);
	}
}
