#ifndef __LUOS_APP_LIST__
#define __LUOS_APP_LIST__

#include <stdint.h>
// All apps have to be #include'ed here


//	------------------------------------

typedef struct app
{
	uint16_t id;	// Cannot be 0, 0 means that the focus is on the directive terminal
	void (*keyEnter)(int);	// Received every time a key is pressed
} app;

const app apps[] =
{
	
};

#endif	//	__LUOS_APP_LIST__
