#ifndef __LUOS_SPLITTER__
#define __LUOS_SPLITTER__

#include "pico/stdlib.h"
#include "i2ckbd.h"
#include "lcdspi.h"
#include <stdbool.h>
// #include "fonts/LuOS_System_Font.h"

// #define MAX_OPTIONS_PER_LIST (LCD_HEIGHT/(LuOS_System_Font_data[1]))
// #define MAX_OPTION_NAME_LENGTH (LCD_WIDTH/(2*LuOS_System_Font_data[0]))-2

/* This interface splits the screen into two, and reserves space for a line in the middle
    as such,
*/
// typedef struct list
// {
//     unsigned char* options;
//     uint8_t max_option_name_length; // Max of 2⁸ = 256 characters. A 320 display will only be able to handle 320/8 = 40 max.
//     uint16_t isRightSideOffset; // Offset for the side on the right. If this is set to 0, we are on the left side.
// } list_t;

extern uint32_t splitter_init(int rIDGiven);
extern void lcd_print_string_c1(int rID, char* str);

#endif // __LUOS_SPLITTER__
