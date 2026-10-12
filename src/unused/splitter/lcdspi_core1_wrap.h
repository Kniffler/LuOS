#ifndef __LCDSPI_CORE1_WRAP__
#define __LCDSPI_CORE1_WRAP__

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "lcdspi.h"
#include <pico/multicore.h>
#include <pico/time.h>

#include "src/include/debug.h"

#define CMD_PRINT_STRING		1
#define CMD_PRINT_CHAR			2
#define CMD_GET_CURRENT_X		3
#define CMD_GET_CURRENT_Y		4
#define CMD_SET_CURRENT_XY		5
#define CMD_RESET_CURRENT_XY	6
#define CMD_GET_REGION_W		7
#define CMD_GET_REGION_H		8
#define CMD_CLEAR_REGION		9
#define CMD_CLEAR_ALL			10


/* Any and all text displaying will be done on core 1.
 * This is done in order to have faster printing times and essentially just not block code execution - which
 * may slow us down in the long run if we are trying to do calculations in the meantime. So long story short,
 * this approach lets us use both cores in order to optimize speed and performance.
 */

static inline void stall_until_fifo_Wready() { while(!multicore_fifo_wready()) { tight_loop_contents(); } }
static inline void stall_until_fifo_Rvalid() { while(!multicore_fifo_rvalid()) { tight_loop_contents(); } }

static volatile bool handshakeComplete = 0;

static void fifo_send_uint64(uint64_t data)
{
	stall_until_fifo_Wready();
	// The inbuilt fifo only lets us send 32 bits at a time, so here I use 2 fifo entries to negate that
	multicore_fifo_push_blocking( (uint32_t)( (uint64_t)data&UINT32_MAX) );
	multicore_fifo_push_blocking( (uint32_t)( ((uint64_t)data&((uint64_t)UINT32_MAX<<32))>>32 ) );
}

static uint64_t fifo_receive_uint64(void)
{
	uint64_t data = (uint64_t)multicore_fifo_pop_blocking();
	data |= (uint64_t)multicore_fifo_pop_blocking()<<32;
	return data;
}

static void core1_lcd_receiver(void)
{
	handshakeComplete = 1;
	DEBUG_PRINT("Handshake concluded | Starting FIFO read loop\n");
	static uint32_t cmd = 0;
	static int32_t rID = 0;
	static char *str;
	for(;;) {
		stall_until_fifo_Rvalid();
		cmd = multicore_fifo_pop_blocking();
		rID = (int32_t)multicore_fifo_pop_blocking();
		switch(cmd)
		{
			case CMD_PRINT_STRING:
				str = (char*)fifo_receive_uint64();
				DEBUG_ASSERT(str);
				lcd_print_string(rID, (char*)str);
				free((char*)str);
				str = NULL;
			break;
			case CMD_PRINT_CHAR: lcd_putc(rID, 0, (char)(multicore_fifo_pop_blocking()&UINT8_MAX)); break;
			case CMD_SET_CURRENT_XY: lcd_region_set_current(rID, multicore_fifo_pop_blocking(), multicore_fifo_pop_blocking()); break;
			case CMD_RESET_CURRENT_XY: lcd_reset_coords(rID); break;
			case CMD_CLEAR_REGION: lcd_region_clear(rID); break;
			case CMD_CLEAR_ALL: lcd_clear(); break;
			case CMD_GET_REGION_W: multicore_fifo_push_blocking(lcd_get_region_width(rID)); break;
			case CMD_GET_REGION_H: multicore_fifo_push_blocking(lcd_get_region_height(rID)); break;
			case CMD_GET_CURRENT_X: multicore_fifo_push_blocking((uint32_t)lcd_get_current_x(rID)); break;
			case CMD_GET_CURRENT_Y: multicore_fifo_push_blocking((uint32_t)lcd_get_current_y(rID)); break;
		}
	}
}

extern void enable_core1_lcdspi()
{
	multicore_launch_core1(core1_lcd_receiver);
	while(!handshakeComplete) { tight_loop_contents(); } // Allow for the core to lauch
}

extern void lcdc1_print_string(int32_t rID, char* str)
{
	char *send = malloc(sizeof(char)*(strlen(str)+1));
	DEBUG_ASSERT(send && str);
	strcpy(send, str);

	stall_until_fifo_Wready();
	multicore_fifo_push_blocking( (uint32_t)(CMD_PRINT_STRING) );
	multicore_fifo_push_blocking( (uint32_t)(rID) );

	fifo_send_uint64((uint64_t)send);
}
extern void lcdc1_putc(int32_t rID, char c)
{
	stall_until_fifo_Wready();
	multicore_fifo_push_blocking( (uint32_t)(CMD_PRINT_CHAR) );
	multicore_fifo_push_blocking( (uint32_t)(rID) );

	multicore_fifo_push_blocking((uint32_t)c);
}
extern void lcdc1_region_set_current(int32_t rID, int x, int y)
{
	stall_until_fifo_Wready();
	multicore_fifo_push_blocking( (uint32_t)(CMD_SET_CURRENT_XY) );
	multicore_fifo_push_blocking( (uint32_t)(rID) );

	multicore_fifo_push_blocking((uint32_t)x);
	multicore_fifo_push_blocking((uint32_t)y);
}
extern void lcdc1_reset_coords(int32_t rID)
{
	stall_until_fifo_Wready();
	multicore_fifo_push_blocking( (uint32_t)(CMD_RESET_CURRENT_XY) );
	multicore_fifo_push_blocking( (uint32_t)(rID) );
}
extern void lcdc1_region_clear(int32_t rID)
{
	stall_until_fifo_Wready();
	multicore_fifo_push_blocking( (uint32_t)(CMD_CLEAR_REGION) );
	multicore_fifo_push_blocking( (uint32_t)(rID) );
}
extern void lcdc1_clear()
{
	stall_until_fifo_Wready();
	multicore_fifo_push_blocking( (uint32_t)(CMD_CLEAR_ALL) );
	multicore_fifo_push_blocking( 0 ); // All commands expect a region ID, so we still need to send it
}
extern uint16_t lcdc1_get_region_width(int32_t rID)
{
	stall_until_fifo_Wready();
	multicore_fifo_push_blocking( (uint32_t)(CMD_GET_REGION_W) );
	multicore_fifo_push_blocking( rID );
	return (uint16_t)(multicore_fifo_pop_blocking()&UINT16_MAX);
}
extern uint16_t lcdc1_get_region_height(int32_t rID)
{
	stall_until_fifo_Wready();
	multicore_fifo_push_blocking( (uint32_t)(CMD_GET_REGION_H) );
	multicore_fifo_push_blocking( rID );
	return (uint16_t)(multicore_fifo_pop_blocking()&UINT16_MAX);
}
extern int lcdc1_get_current_x(int32_t rID)
{
	stall_until_fifo_Wready();
	multicore_fifo_push_blocking( (uint32_t)(CMD_GET_CURRENT_X) );
	multicore_fifo_push_blocking( rID );
	return multicore_fifo_pop_blocking();
}
extern int lcdc1_get_current_y(int32_t rID)
{
	stall_until_fifo_Wready();
	multicore_fifo_push_blocking( (uint32_t)(CMD_GET_CURRENT_Y) );
	multicore_fifo_push_blocking( rID );
	return multicore_fifo_pop_blocking();
}
#endif // __LCDSPI_CORE1_WRAP__
