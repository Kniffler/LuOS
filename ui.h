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
	uint16_t pixelOffsetX;
	uint16_t pixelOffsetY;
	int16_t index_offset;

} AGrid_t;

typedef struct TerminalInstance {

} Terminal_t;
// Clarification: coordinates refer to pixels and indecies refer to an index into the grid.x and grid.y

static point_t gridToPixel(AGrid_t *grid, uint16_t index);
static uint16_t pixelToGrid(AGrid_t *grid, point_t coordinates);

static size_t printableEnumerate(unsigned char *str, size_t norm_length);

extern void ui_PrintChar(MBNA AGrid_t *grid, uint16_t index, unsigned char c, bool invert);
extern void ui_PrintString(MBNA AGrid_t *grid, uint16_t index, unsigned char *str, size_t len);

extern void v(MBNA AGrid_t *grid, uint16_t y, unsigned char c);
// extern void ui_print_vertical(MBNA AGrid_t *grid, uint16_t x, unsigned char c);

extern void ui_GridConfigure(AGrid_t *grid, uint16_t width, uint16_t height, font_t font, uint8_t tabsize, uint16_t foreground, uint16_t background);
extern void ui_ShowDemo(MBNA AGrid_t *grid);
extern void ui_Draw(MBNA AGrid_t *grid, bool topside);
extern void ui_UpdateStatus(MBNA AGrid_t *grid, unsigned char *status_text);

static size_t ui_TerminalPrintSingle(MBNA AGrid_t *grid, unsigned char *string);
extern int ui_TerminalPrint(MBNA AGrid_t *grid, unsigned char *string);
extern int ui_TerminalReprint(MBNA AGrid_t *grid, unsigned char *string, uint8_t order);
extern void ui_TerminalQuery(MBNA AGrid_t *grid, unsigned char *input, size_t input_max, size_t line);

#endif //__LUOS_USER_INTERFACE__
