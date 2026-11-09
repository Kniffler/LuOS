#ifndef __LUOS_USER_INTERFACE__
#define __LUOS_USER_INTERFACE__

#include <pico/stdio.h>
#include <stdint.h>
#include <string.h>
#include "src/include/debug.h"
#include "buffy.h"

#define ABS(x) (((x)<0) ? (x)*-1 : (x))
#define ABS_ADD_Y(x, y) (((x)<0) ? (x)+(y) : (x))

typedef struct Point2D {
	uint16_t x;
	uint16_t y;
} point_t;

typedef struct ASCII_Grid {
	font_t font;
	uint8_t tabsize;
	uint16_t foreground;
	uint16_t background;
	unsigned char demo_glyph;
	uint16_t glyph_count_horizontal;
	uint16_t glyph_count_vertical;
	uint16_t pixel_offset_x;
	uint16_t pixel_offset_y;
	int16_t index_offset;

} AGrid_t;

// Clarification: coordinates refer to pixels and indecies refer to an index into the grid.x and grid.y

static point_t grid_index_to_pixel_coord(AGrid_t *grid, uint16_t index);
static uint16_t pixel_coord_to_grid_index(AGrid_t *grid, point_t coordinates);

static size_t printable_enumerate(unsigned char *str, size_t norm_length);

extern void ui_print_char(MBNA AGrid_t *grid, uint16_t index, unsigned char c, bool invert);
extern void ui_print_string(MBNA AGrid_t *grid, uint16_t index, unsigned char *str, size_t len);

extern void ui_print_horizontal_line(MBNA AGrid_t *grid, uint16_t y, unsigned char c);
// extern void ui_print_vertical(MBNA AGrid_t *grid, uint16_t x, unsigned char c);

extern void ui_configure_grid(AGrid_t *grid, uint16_t width, uint16_t height, font_t font, uint8_t tabsize, uint16_t foreground, uint16_t background);
extern void ui_show_grid_demo(MBNA AGrid_t *grid);


extern void ui_draw(MBNA AGrid_t *grid, bool topside);
extern void ui_update_status(MBNA AGrid_t *grid, unsigned char *status_text);
extern int ui_terminal_print(MBNA AGrid_t *grid, unsigned char *string, ...);
extern void ui_terminal_query(MBNA AGrid_t *grid, unsigned char *input, size_t input_max, size_t line);

#endif //__LUOS_USER_INTERFACE__
