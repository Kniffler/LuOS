#ifndef __APP_IOWIZ__
#define __APP_IOWIZ__
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

static TaskHandle_t uart_task;
extern void iowiz_open();
extern void iowiz_directive_sent(char *dir);
extern void iowiz_key_press(int key);	// Received every time a key is pressed

#endif // __APP_IOWIZ__
