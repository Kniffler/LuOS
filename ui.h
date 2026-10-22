#ifndef __LUOS_USER_INTERFACE__
#define __LUOS_USER_INTERFACE__

#include <pico/stdio.h>
#include "src/include/debug.h"
#include "buffy.h"

#ifdef BUFFY_USE_LOCAL_MEMORY_BUFFER
#ifdef UI_CHARACTER_MEMORY_ENABLE
#error Please constrain yourself to one buffer at a time.
#endif
#endif

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

#ifdef UI_CHARACTER_MEMORY_ENABLE
	unsigned char *chars;
#endif
} AGrid_t;

// Clarification: coordinates refer to pixels and indecies refer to an index into the grid.x and grid.y

static point_t grid_index_to_pixel_coord(AGrid_t grid, uint16_t index);
static uint16_t pixel_coord_to_grid_index(AGrid_t grid, point_t coordinates);

extern void ui_print_char(MBNA AGrid_t grid, uint16_t index, unsigned char c);
extern void ui_print_string(MBNA AGrid_t grid, uint16_t index, unsigned char *str, size_t len);


#endif //__LUOS_USER_INTERFACE__
