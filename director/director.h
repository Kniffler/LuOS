#ifndef __LUOS_DIRECTOR__
#define __LUOS_DIRECTOR__

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include "lcdspi.h"
#include "keyboard_define.h"

#define CHAR_AREA ((LCD_WIDTH/FONT_WIDTH)*((LCD_HEIGHT/FONT_HEIGHT)-2))

#define DIR_LEN ((LCD_WIDTH/FONT_WIDTH)-1+2)


extern uint8_t app_focused;


static char directive_history[64][DIR_LEN] = {'\0'};
static char directive_changed_history[64][DIR_LEN] = {'\0'};
static short history_pointer = 0;
static short preview_pointer = 0;

static void copy_directive_history();

static char directive[DIR_LEN] = {'\0'};
static short cursor_pos = 0;

static uint8_t directive_execute();
static char directive_spacer[DIR_LEN];

static void print_directive(uint8_t deb_num);

static void directive_char_insert(int i, char c);
static void directive_char_delete(int i);


extern void directive_key_press(int key);
extern void dir_init();
extern void dir_clear();

#endif // __LUOS_DIRECTOR__
