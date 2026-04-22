#ifndef __LUOS_DEBUG_H__
#define __LUOS_DEBUG_H__

// Code from
// https://github.com/adwuard/Picocalc_SD_Boot/blob/main/src/debug.h

#include <stdio.h>
#include "hardware/watchdog.h"
#include "pico/time.h"

#define DEBUG_PRINT(fmt, ...) printf("%s: ", __func__); printf(fmt, ##__VA_ARGS__)
#define DEBUG_PRINT_ERR(fmt, ...) fprintf(stderr, "ERROR in func %s at line #%d: ", __func__, __LINE__); \
	fprintf(stderr, fmt, ##__VA_ARGS__)
#define DEBUG_REBOOT() watchdog_reboot(0, 0, 0)
#define DEBUG_ASSERT(cond) \
if(!cond) {					\
	fprintf(stderr, "Assertion failure: \"%s\" failed in file %s at line %d\n\t\t | Now rebooting...", #cond, __FILE__, __LINE__); \
	sleep_ms(800);			\
	DEBUG_REBOOT();			\
}

#define DEBUG_PRINT_UNDER_CONDITION(cond, toPrint, ...) \
if(cond) { printf("%s: ", __func__); printf(toPrint, ##__VA_ARGS__; ) }
#define DEBUG_PRINT_ERR_UNDER_CONDITION(cond, toPrint, ...) \
if(cond) { printf("%s: ", __func__); fprintf(stderr, toPrint, ##__VA_ARGS__; ) }

#endif // __LUOS_DEBUG_H__
