#ifndef __LUOS_APP_MANAGER__
#define __LUOS_APP_MANAGER__

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "uthash/include/uthash.h"
// #include "uthash.h"

struct app
{
	char call_name[16];	// Name used to start the app
	void (*app_open)();	// The function to start the app
	void (*app_directive_sent)(char *);	// Received every time a directive is executed during active usage of the app
	void (*app_key_press)(int);	// Received every time a key is presse

	UT_hash_handle hh;
};

struct app *apps;
static uint8_t open_app_exists;

extern uint8_t directive_write_referal;
extern void manager_init();

//	Functions that apps will be able to use
extern void app_clear();
extern void print_string();
extern void set_write_coords();
extern void get_write_coords();

// All apps have to be #include'ed here
#include "iowiz.h"


//	------------------------------------

#endif	//	__LUOS_APP_MANAGER__
