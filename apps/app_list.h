#ifndef __LUOS_APP_LIST__
#define __LUOS_APP_LIST__

#include <stdint.h>
// All apps have to be #include'ed here


//	------------------------------------

extern typedef struct app
{
	uint16_t id;	// Cannot be 0, 0 means that the focus is on the directive terminal
	void (*keyEnter)(int);	// Received every time a key is pressed
	void (*print_app)( (*printl)(char *), (*clear)(void), (*set_write_coords)(int x, int y) );
} app;

const app apps[] =
{
	
};
extern uint16_t

#endif	//	__LUOS_APP_LIST__
