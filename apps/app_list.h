#ifndef __LUOS_APP_LIST__
#define __LUOS_APP_LIST__

#include <stdint.h>
// All apps have to be #include'ed here


//	------------------------------------

typedef struct app
{
	uint16_t id;	// Cannot be 0, 0 means that the focus is on the directive terminal
	void (*keyEnter)(int);	// Received every time a key is pressed
	void (*print_app)( void (*printl)(char *), void (*clear)(void), void (*set_write_coords)(int x, int y) );
	void (*directive_sent)(char *);
	long long timeout; //	Set to zero if the app will be printed after pressing a key
} app;


app apps[] =
{
	
};

#endif	//	__LUOS_APP_LIST__
