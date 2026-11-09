#ifndef __LUOS_DEBUG_H__
#define __LUOS_DEBUG_H__

#include <stdio.h>
#include "hardware/watchdog.h"
#include "pico/time.h"
#include "config.h"

#ifdef USE_DEBUG

// Stole some of this code from
// https://github.com/adwuard/Picocalc_SD_Boot/blob/main/src/debug.h

#define DEBUG_PRINT(fmt, ...) printf("%s => ", __func__); printf(fmt, ##__VA_ARGS__)
#define DEBUG_PRINT_ERR(fmt, ...) fprintf(stderr, "ERROR in func %s at line #%d => ", __func__, __LINE__); \
	fprintf(stderr, fmt, ##__VA_ARGS__)
#define DEBUG_REBOOT() watchdog_reboot(0, 0, 0)
#define DEBUG_ASSERT(cond) \
if(!cond) {					\
	fprintf(stderr, "Assertion failure: \"%s\" failed in file %s at line %d\n\t\t | Now rebooting...", #cond, __FILE__, __LINE__); \
	sleep_ms(800);			\
	DEBUG_REBOOT();			\
}

#define DEBUG_PRINT_UNDER_CONDITION(cond, toPrint, ...) \
if(cond) { DEBUG_PRINT(toPrint, ##__VA_ARGS__); }
#define DEBUG_PRINT_ERR_UNDER_CONDITION(cond, toPrint, ...) \
if(cond) { DEBUG_PRINT_ERR(toPrint, ##__VA_ARGS__); }

#else

#define DEBUG_PRINT(fmt, ...)
#define DEBUG_PRINT_ERR(fmt, ...)
#define DEBUG_REBOOT() watchdog_reboot(0, 0, 0)
#define DEBUG_ASSERT(cond)
#define DEBUG_PRINT_UNDER_CONDITION(cond, toPrint, ...)
#define DEBUG_PRINT_ERR_UNDER_CONDITION(cond, toPrint, ...)

#endif // USE_DEBUG


// --- Independent of debug, the following error codes have to be defined ---
/* NOTE: Non-zero is always true in c,
 * so negative values are more convenient
 * when returning an index or otherwise.
 */

#define LUOS_SUCCESS		0  // return on success
#define LUOS_NONDIFF		0  // return on success or error so long as the result makes no fatal difference
#define LUOS_ERROR			-1  // return on generic error
#define LUOS_ERROR_MEM		-2  // return on memory error - allocation or access
#define LUOS_ERROR_GRAPH	-3  // return on graphical error - e.g. an impossible character
#define LUOS_ERROR_BOUNDS	-4  // return on bounds error - a variable/parameter is out of bounds
#define LUOS_ERR_ERR_COND	-5  // return on any combination of other errors
#define LUOS_ERROR_COMM		-6  // return on communication error - peripherals or memory
#define LUOS_ERROR_EXT		-7  // return on external error - fault lies outside the machine, e.g. display driver)
#define LUOS_ERROR_FATAL	-8  // return on fatal error - !!! it is advised to restart the device if this occurs !!!

// Error stages, to be interpreted by the user
#define LUOS_ERROR_S0	-12  // Stage 0
#define LUOS_ERROR_S1	-13  // Stage 1
#define LUOS_ERROR_S2	-14  // Stage 2
#define LUOS_ERROR_S3	-15  // Stage 3
#define LUOS_ERROR_S4	-16  // Stage 4
#define LUOS_ERROR_S5	-17  // Stage 5
#define LUOS_ERROR_S6	-18  // Stage 6
#define LUOS_ERROR_S7	-19  // Stage 7



#endif // __LUOS_DEBUG_H__
