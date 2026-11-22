#include "ui.h"
#include "buffy.h"
#include "config.h"
#include "src/include/debug.h"
#include <stdint.h>


static point_t gridToPixel(AGrid_t *grid, uint16_t index)
{
	uint16_t x = ( (index-grid->index_offset) % grid->glyph_count_horizontal ) * grid->font[0] + grid->pixelOffsetX;
	uint16_t y = ( (index-grid->index_offset) / grid->glyph_count_horizontal ) * grid->font[1] + grid->pixelOffsetY;

	return (point_t){x%LCD_RES_H, y%LCD_MEM_HEIGHT};
}
static uint16_t pixelToGrid(AGrid_t *grid, point_t coordinates)
{
	uint16_t grid_x = (coordinates.x-grid->pixelOffsetX)/grid->font[0];
	uint16_t grid_y = (coordinates.y-grid->pixelOffsetY)/grid->font[1];
	return grid_y*grid->glyph_count_horizontal + grid_x + grid->index_offset;
}

static size_t printableEnumerate(unsigned char *str, size_t norm_length)
{
	size_t result = 0;
	for(size_t i = 0; i < norm_length; i++)
	{
		if(str[i] >= 32 && str[i] <= 126 ) { result++; }
	}
	return result;
}

extern void ui_PrintChar(MBNA AGrid_t *grid, uint16_t index, unsigned char c, bool invert)
{
	// DEBUG_PRINT("c: %c\n", c);
	index -= grid->index_offset;
	point_t draw_point = gridToPixel(grid, index);
	lcd_DrawChar(MBNAP c, draw_point.x, draw_point.y, MIN(grid->glyph_count_vertical*grid->font[1], LCD_MEM_HEIGHT), grid->font,
				  (invert) ? grid->background : grid->foreground, (invert) ? grid->foreground : grid->background);
}

extern void ui_PrintString(MBNA AGrid_t *grid, uint16_t index, unsigned char *str, size_t len)
{
	index -= grid->index_offset;
	size_t max = grid->glyph_count_horizontal*grid->glyph_count_vertical;

	// size_t print_length = printable_enumerate(str, len);
	// print_length %= max;
	bool invert = false;

	for(size_t i = 0; i < len; i++)
	{
		index %= max;
		unsigned char c = str[i];
		switch(c)
		{
			case '\0': return; break;
			case '\n':
				// Looks weird, but this is integer math
				index = (index+grid->glyph_count_horizontal)/grid->glyph_count_horizontal;
				index *= grid->glyph_count_horizontal;
				break;
			case '\r':
				index--;
				break;
			case '\t':
				while(index%grid->tabsize) { ui_PrintChar(MBNAP grid, index, ' ', invert); }
				break;
			case '\b':
				index--;
				ui_PrintChar(MBNAP grid, index, ' ', false);
				break;
			case '\017':
				invert = true;
				break;
			case '\016':
				invert = false;
				break;
			default:
				ui_PrintChar(MBNAP grid, index, c, invert);
				index++;
			break;
		}
	}
}
extern void ui_PrintHorLine(MBNA AGrid_t *grid, uint16_t y, unsigned char c)
{
	y %= grid->glyph_count_vertical;
	size_t index = y*grid->glyph_count_horizontal;
	for(size_t i = 0; i < grid->glyph_count_horizontal; i++)
	{
		ui_PrintChar(MBNAP grid, index, c, false);
		index++;
	}
}


extern void ui_GridConfigure(AGrid_t *grid, uint16_t width, uint16_t height, font_t font, uint8_t tabsize, uint16_t foreground, uint16_t background)
{
	grid->font = font;
	grid->tabsize = tabsize%LCD_RES_H;
	grid->foreground = foreground;
	grid->background = background;
	grid->demo_glyph = 255;
	grid->glyph_count_horizontal = width;
	grid->glyph_count_vertical = height;

}
extern void ui_ShowDemo(MBNA AGrid_t *grid)
{
	size_t max = grid->glyph_count_horizontal*grid->glyph_count_vertical;

	for(size_t i = 0; i < max; i++)
	{
		ui_PrintChar(MBNAP grid, i, grid->demo_glyph, false);
	}
}


extern void ui_Draw(MBNA AGrid_t *grid, bool topside)
{
	if(topside) { lcd_ShowDisplayTop(); }
	else { lcd_ShowDisplayBot(); }
	ui_UpdateStatus(MBNAP grid, "Fuck you this is just a status text\n");
	ui_PrintHorLine(MBNAP grid, grid->glyph_count_vertical-2, '-');
	ui_PrintChar(MBNAP grid, (grid->glyph_count_vertical-1)*grid->glyph_count_horizontal, '>', 0);
	ui_PrintString(MBNAP grid, 0, "\017 \016", 3);
}
extern void ui_update_status(MBNA AGrid_t *grid, unsigned char *status_text)
{
	ui_PrintString(MBNAP grid, 0, status_text, strlen(status_text));
	ui_PrintHorLine(MBNAP grid, 1, '-');
}


static size_t ui_TerminalPrintSingle(MBNA AGrid_t *grid, unsigned char *string)
{

}
extern int ui_TerminalPrint(MBNA AGrid_t *grid, unsigned char *string)
{
	static uint8_t screenline_occupancy = 0;
	static uint8_t indecie_stack[256] = {0};
	static uint8_t indecie_stack_top = 0;
}
extern void ui_TerminalQuery(MBNA AGrid_t *grid, unsigned char *input, size_t input_max, size_t line)
{
	static unsigned char cmd_history[UI_TERM_COMMAND_HISTORY_LENGTH][UI_TERM_MAX_COMMAND_SIZE];
	static size_t history_write_index = 0;
	static int history_direction = +1;
	return;
}
