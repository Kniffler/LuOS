#include "test_suite.h"
#include "buffy.h"
#include "i2ckbd.h"
#include "ui.h"

extern void suite_TestGrid(MBNA AGrid_t *grid, size_t x_step, size_t y_step)
{
	static unsigned char str[] =  "sphinx of black quartz judge my vow SPHINX OF BLACK QUARTZ JUDGE MY VOW 012345789 !\" #$%&/()=?'@§{[]}+*-_.:,;~^<> \017sphinx of black quartz judge my vow SPHINX OF BLACK QUARTZ JUDGE MY VOW 012345789 !\" #$%&/()=?'@§{[]}+*-_.:,;~^<>\016\0";
	static unsigned char str2[] = " ";

	static unsigned char command[512];

	int x = 0, y = 0, c = -1;
	bool grid_shift_mode = 0;

	ui_PrintString(MBNAP grid, x, str, strlen(str));
	// lcd_refresh(lcd_buffer);
	kbd_Init();
	for(;;)
	{
		c = kbd_Read();
		if(c==-1) { continue; }

		if(c=='u') { lcd_ShowDisplayTop(); continue; }
		if(c=='j') { lcd_ShowDisplayBot(); continue; }
		if(c=='1') { lcd_Clear(MBNAP BLACK); }
		if(c=='2') { lcd_Clear(MBNAP WHITE);}
		if(c=='3') { lcd_Clear(MBNAP RED); }
		if(c=='4') { lcd_Clear(MBNAP GREEN); }
		if(c=='5') { lcd_Clear(MBNAP BLUE); }
		if(c=='6') { lcd_Clear(MBNAP VIOLET); }
		if(c=='7') { lcd_Clear(MBNAP YELLOW); }
		if(c=='8') { lcd_Clear(MBNAP ORANGE); }
		if(grid_shift_mode)
		{
			switch(c)
			{
				case KEY_UP:	grid->pixelOffsetY--; break;
				case KEY_DOWN:	grid->pixelOffsetY++; break;
				case KEY_LEFT:	grid->pixelOffsetX--; break;
				case KEY_RIGHT:	grid->pixelOffsetX++; break;
				case 'g': grid_shift_mode = false; continue; break;
			}
			ui_ShowDemo(MBNAP grid);
			// lcd_refresh(lcd_buffer);
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
				ui_ShowDemo(MBNAP grid);
				// lcd_refresh(lcd_buffer);
				continue; break;
		}
		if(y<0) { y += grid->glyph_count_vertical; }
		if(x<0) { x += grid->glyph_count_horizontal; }
		y %= grid->glyph_count_vertical;
		x %= grid->glyph_count_horizontal;
		lcd_Clear(MBNAP BLACK);
		ui_PrintString(MBNAP grid, (y*grid->glyph_count_horizontal) + x, str, strlen(str));
		// lcd_Refresh(lcd_buffer);
	}
}


extern void suite_Terminal(MBNA AGrid_t *grid)
{

}
