#include "ui.h"
#include "buffy.h"
#include <stdint.h>


static point_t grid_index_to_pixel_coord(AGrid_t grid, uint16_t index)
{
	uint16_t x = ( (index-grid.index_offset) % grid.glyph_count_horizontal ) * grid.font[0] + grid.pixel_offset_x;
	uint16_t y = ( (index-grid.index_offset) / grid.glyph_count_vertical ) * grid.font[1] + grid.pixel_offset_y;

	return (point_t){x%LCD_RES_H, y%LCD_RES_V};
}
static uint16_t pixel_coord_to_grid_index(AGrid_t grid, point_t coordinates)
{
	uint16_t grid_x = (coordinates.x-grid.pixel_offset_x)/grid.font[0];
	uint16_t grid_y = (coordinates.y-grid.pixel_offset_y)/grid.font[1];
	return grid_y*grid.glyph_count_horizontal + grid_x + grid.index_offset;
}

extern void ui_print_char(MBNA AGrid_t grid, uint16_t index, unsigned char c)
{
#ifdef UI_CHARACTER_MEMORY_ENABLE
	grid.chars[index] = c;
#else
	point_t draw_point = grid_index_to_pixel_coord(grid, index);
	lcd_draw_char(MBNAP c, draw_point.x, draw_point.y, grid.font, grid.foreground, grid.background);
#endif // UI_CHARACTER_MEMORY_ENABLE
}

extern void ui_print_string(MBNA AGrid_t grid, uint16_t index, unsigned char *str, size_t len)
{
	size_t max = grid.glyph_count_horizontal*grid.glyph_count_vertical;
	if(index >= max)
	{
		DEBUG_PRINT_ERR("index is out of bounds\n");
		return;
	}else if(index+len >= max)
	{
		DEBUG_PRINT_ERR("index and/or len out of bounds; Ajdusting via modulo; Avoid triggering this error.");
		len %= max;
	}
#ifdef UI_CHARACTER_MEMORY_ENABLE
	memcpy(grid.chars, str, len);
	/*for(size_t i = 0; i < len; i++)
	{
		grid.chars[index+i] = str[i];
	}*/
#else

	point_t draw_point = grid_index_to_pixel_coord(grid, index);

	// The following sizes are all in-terms of glyphs/chars

	size_t first_line = grid.glyph_count_horizontal-(index%grid.glyph_count_horizontal);
	// size_t line_count = full_lines + (final_line>0);
	if(first_line!=grid.glyph_count_horizontal)
	{
		lcd_draw_string(MBNAP str, first_line, draw_point.x, draw_point.y, grid.font, grid.tabsize, grid.foreground, grid.background);
		index += first_line;
	}else{
		first_line = 0;
	}
	size_t final_line = (len-first_line)%grid.glyph_count_horizontal;
	size_t full_lines = (len-final_line - first_line)/grid.glyph_count_horizontal;


	for(size_t i = 0; i < full_lines; i++)
	{
		draw_point = grid_index_to_pixel_coord(grid, index);
		lcd_draw_string(MBNAP str+first_line+(i*grid.glyph_count_horizontal), grid.glyph_count_horizontal,
						draw_point.x, draw_point.y, grid.font, grid.tabsize, grid.foreground, grid.background);
		index += grid.glyph_count_horizontal;
	}
	if(final_line>0)
	{
		draw_point = grid_index_to_pixel_coord(grid, index);
		lcd_draw_string(MBNAP str+full_lines+first_line, final_line, draw_point.x, draw_point.y, grid.font, grid.tabsize, grid.foreground, grid.background);
	}
	/*
	for(size_t i = 0; i < len; i++)
	{
		ui_print_char(MBNAP grid, index+i, str[i]);
	}
	*/
#endif // UI_CHARACTER_MEMORY_ENABLE
}
