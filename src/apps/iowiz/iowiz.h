#ifndef __APP_IOWIZ__
#define __APP_IOWIZ__

#include <string.h>
#include <ctype.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "lcdspi.h"
#include "keyboard_define.h"

#define CHAR_AREA ((LCD_WIDTH/FONT_WIDTH)*((LCD_HEIGHT/FONT_HEIGHT)-2))

#define UART_CON uart0
#define UART_TX 0
#define UART_RX 1

extern int iowiz_open(int draw_rID);
extern void iowiz_close();
extern void iowiz_directive_sent(char *dir);
extern void iowiz_key_press(int key);	// Received every time a key is pressed

#endif // __APP_IOWIZ__
