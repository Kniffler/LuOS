#ifndef __LUOS_USER_INTERFACE__
#define __LUOS_USER_INTERFACE__

#include <pico/stdio.h>
#include "src/include/debug.h"
#include "buffy.h"

typedef struct Point2D {
	uint16_t x;
	uint16_t y;
} point_t;

typedef struct ASCII_Grid {
	font_t font;
	unsigned char demo_glyph;
	uint16_t glyph_count_horizontal;
	uint16_t glyph_count_vertical;
	uint16_t origin_offset_x;
	uint16_t origin_offset_y;
	int16_t grid_coord_offset_x;
	int16_t grid_coord_offset_y;
} AGrid_t;

static point_t grid_to_pixel_coord(point_t coordinates);
static point_t pixel_coord_to_grid(point_t grid_point);



#endif //__LUOS_USER_INTERFACE__
