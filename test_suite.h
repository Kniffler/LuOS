#ifndef __LUOS_TEST_SUITE__
#define __LUOS_TEST_SUITE__

#include <stdio.h>
#include <string.h>
#include "src/include/debug.h"
#include "src/keyboard_define.h"
#include "i2ckbd.h"
#include "ui.h"
#include "config.h"

extern void suite_TestGrid(MBNA AGrid_t *grid, size_t x_step, size_t y_step);
extern void suite_Terminal(MBNA AGrid_t *grid);

#endif //__LUOS_TEST_SUITE__
