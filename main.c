#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "blockdevice/sd.h"
#include "filesystem/fat.h"
#include "filesystem/vfs.h"

//	Interface pins for the SD card
#define SD_SPI0         0
#define SD_SCLK_PIN     18
#define SD_MOSI_PIN     19
#define SD_MISO_PIN     16
#define SD_CS_PIN       17
#define SD_DET_PIN			22


#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "i2ckbd.h"
#include "lcdspi.h"
#include "keyboard_define.h"
#include "manager.h"

const uint LEDPIN = 25;

uint8_t sd_status;	//0 no sdcard ,1 has sd card
bool sd_card_inserted(void)
{
	sd_status = !gpio_get(SD_DET_PIN);
	return (bool)sd_status;
}

bool fs_init(void);
void all_init();
static void kbd_handler_task(void *data);
static void draw_handler_task(void *data);

#define CHAR_WIDTH 0x08
static char directiveCache[256][(LCD_WIDTH/CHAR_WIDTH)-1];
//	Has to be constant, as such we need to know the resolution as well as the character width
//	ahead of time. I have just copied the width entry from lcdspi/fonts/font1.h
static unsigned short d_f = 0;	//	Short name for ease of typing, d_f = "directive_front"

void main(void)
{
	all_init();
	char msg[] = "Your mom\n";
	lcd_print_string(msg);
	
	TaskHandle_t kbd_handler = NULL;
	xTaskCreate(kbd_handler_task, "kbd_handler", 128, (void *) NULL, 16, &kbd_handler);
}

static void draw_handler_task(void *data)
{
	(void)data;
	
	vTaskDelete(NULL);
}
static void kbd_handler_task(void *data)
{
	(void)data;
	
	int key;
	for(;;)
	{
		key = read_i2c_kbd(&key);
	}
	vTaskDelete(NULL);
}



bool fs_init(void)
{
	if(!sd_card_inserted()) { return false; }
	
	// DEBUG_PRINT("fs init SD\n");
	blockdevice_t *sd = blockdevice_sd_create(spi0,
		SD_MOSI_PIN,
		SD_MISO_PIN,
		SD_SCLK_PIN,
		SD_CS_PIN,
		125000000 / 2 / 4, // 15.6MHz
		true);
	
	filesystem_t *fat = filesystem_fat_create();
	int err = fs_mount("/", fat, sd);
	
	if (err == -1)
	{
		// DEBUG_PRINT("format /\n");
		err = fs_format(fat, sd);
		if (err == -1)
		{
			// DEBUG_PRINT("format err: %s\n", strerror(errno));
			return false;
		}
		err = fs_mount("/", fat, sd);
		if (err == -1)
		{
			// DEBUG_PRINT("mount err: %s\n", strerror(errno));
			return false;
		}
	}
	return true;
}

void all_init()
{
	set_sys_clock_khz(133000, true);
	stdio_init_all();

	uart_init(uart0, 115200);

	uart_set_format(uart0, 8, 1, UART_PARITY_NONE);  // 8-N-1
	uart_set_fifo_enabled(uart0, false);

	init_i2c_kbd();
	lcd_init();
	fs_init();
	
	gpio_init(LEDPIN);
	gpio_set_dir(LEDPIN, GPIO_OUT);

	gpio_put(LEDPIN, 1);
	sleep_ms(1600);
	
	// lcd_set_colours(RGB(200, 0, 200), RGB(10, 5, 10));
	lcd_set_colours(RGB(128, 0, 32), BLACK);
	sleep_ms(200);
	lcd_clear();
	sleep_ms(200);
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
