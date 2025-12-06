#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "i2ckbd.h"
#include "lcdspi.h"
#include "keyboard_define.h"
// #include "manager.h"
#include "director.h"


#define KBD_TASK_PRIORITY	12

const unsigned int LEDPIN = 25;


void all_init();

static void kbd_handler_task(void *data);
bool is_printable_key(int key) { return key>31&&key<127; };

void main(void)
{
	app_focused = 0;
	all_init();
	
	/*TaskHandle_t kbd_handler = NULL;
	BaseType_t xReturned;
	xReturned =*/
	xTaskCreate(kbd_handler_task, "kbd_handler", 128, (void *) NULL, KBD_TASK_PRIORITY, NULL);
	//, &kbd_handler);

	dir_clear();

	vTaskStartScheduler();
}

static void kbd_handler_task(void *data)
{
	int key;
	for(;;)
	{
		key = read_i2c_kbd();
		if(key == -1) { continue; }
		
		// if(!(app_focused)||directive_write_referal)
		if(!(app_focused))
		{
			directive_key_press(key); 
		}
	}
	vTaskDelete(NULL);
}



void all_init()
{
	set_sys_clock_khz(133000, true);
	stdio_init_all();

	init_i2c_kbd();
	lcd_init();
	
	gpio_init(LEDPIN);
	gpio_set_dir(LEDPIN, GPIO_OUT);

	gpio_put(LEDPIN, 1);
	sleep_ms(1600);
	
	lcd_set_colours(WHITE, BLACK);
	// lcd_set_colours(RGB(128, 0, 32), BLACK);
	sleep_ms(200);
	lcd_clear();
	sleep_ms(200);
	
	dir_init();
}




/*
* These functions are requried for FreeRTOS to work in static memory mode.
*/

#if configSUPPORT_STATIC_ALLOCATION
static StaticTask_t idle_task_tcb;
static StackType_t idle_task_stack[mainIDLE_TASK_STACK_DEPTH];
void vApplicationGetIdleTaskMemory(
    StaticTask_t **ppxIdleTaskTCBBuffer,
    StackType_t **ppxIdleTaskStackBuffer,
    uint32_t *pulIdleTaskStackSize
) {
    *ppxIdleTaskTCBBuffer = &idle_task_tcb;
    *ppxIdleTaskStackBuffer = idle_task_stack;
    *pulIdleTaskStackSize = mainIDLE_TASK_STACK_DEPTH;
}

static StaticTask_t timer_task_tcb;
static StackType_t timer_task_stack[configMINIMAL_STACK_SIZE];
void vApplicationGetTimerTaskMemory(
    StaticTask_t **ppxTimerTaskTCBBuffer,
    StackType_t **ppxTimerTaskStackBuffer,
    uint32_t *pulTimerTaskStackSize
) {
    *ppxTimerTaskTCBBuffer = &timer_task_tcb;
    *ppxTimerTaskStackBuffer = timer_task_stack;
    *pulTimerTaskStackSize = configMINIMAL_STACK_SIZE;
}
#endif
