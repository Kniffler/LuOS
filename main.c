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

#define KBD_TASK_PRIORITY	12
#define FONT_WIDTH 0x08
#define FONT_HEIGHT 0x0C

const unsigned int LEDPIN = 25;

uint16_t app_focus = 0;

void all_init();

static void kbd_handler_task(void *data);
bool is_printable_key(int key) { return key>31&&key<127; };

static char directive[(LCD_WIDTH/FONT_WIDTH)-3] = {'\0'};
static short cursor_pos = 0;
static short d_w_x = 0;	// Directive Write x
static short d_w_y = 0;	// Directive Write y

static void directive_key_press(int key);
static uint8_t directive_execute();
static char directive_spacer[(LCD_WIDTH/FONT_WIDTH)+1];

static void print_directive();

static void insert_char(char *s, int i, char c);
static void delete_char(char *s, int i);


void main(void)
{
	all_init();
	
	TaskHandle_t kbd_handler = NULL;
	BaseType_t xReturned;
	xReturned = xTaskCreate(kbd_handler_task, "kbd_handler", 128, (void *) NULL, KBD_TASK_PRIORITY, &kbd_handler);

	for(int i = 0; i < LCD_WIDTH/FONT_WIDTH; i++)
	{
		directive_spacer[i] = '-';
	}
	directive_spacer[(LCD_WIDTH/FONT_WIDTH)] = '\0';
	
	lcd_clear();
	lcd_reset_coords();
	
	print_directive();
	d_w_x = lcd_get_current_x();
	d_w_y = lcd_get_current_y();
	
	lcd_print_string("\n");
	lcd_print_string(directive_spacer);
	lcd_print_string("\n");

	vTaskStartScheduler();
}


static uint8_t directive_execute()
{
	if(strcmp(directive, ".clear")==0)
	{
		lcd_clear();
		lcd_reset_coords();
		
		// strcpy(directive, "\0");
		// print_directive(); 
		// No need to call these since the end of the function handles it.
		lcd_print_string("\n");
		lcd_print_string(directive_spacer);
		lcd_print_string("\n");
	}

	strcpy(directive, "\0");
	cursor_pos = 0;
	return 0;
}

static void directive_key_press(int key)
{
	bool execute_at_end = false;
	if(is_printable_key(key) && cursor_pos+1<(LCD_WIDTH/FONT_WIDTH))
	{
		insert_char(directive, cursor_pos++, (char)key);
	}
	
	switch(key)
	{
		case KEY_HOME: app_focus = 0;	break;
		case KEY_BACKSPACE:
			if(cursor_pos==0) { delete_char(directive, cursor_pos); break; }
			else if(cursor_pos-1<0) { break; }
			
			if(directive[cursor_pos-1]>0) { delete_char(directive, --cursor_pos); }
			else if(directive[cursor_pos]>0) { delete_char(directive, cursor_pos); }
			break;
		case KEY_ENTER: execute_at_end = true; break;
		case KEY_LEFT:
			if(cursor_pos-1<0) { break; }
			cursor_pos--;
			break;
		case KEY_RIGHT:
			if(directive[cursor_pos]=='\0') { break; }
			cursor_pos++;
			break;
		
		
		case KEY_UP:		break;
		case KEY_DOWN:	break;
	}
	if(cursor_pos>(LCD_WIDTH/FONT_WIDTH)-3) { cursor_pos = (LCD_WIDTH/FONT_WIDTH)-2;}

	if(execute_at_end) { directive_execute(); }
	print_directive();
}

static void insert_char(char *s, int i, char c)
{
	for(int k = (LCD_WIDTH/FONT_WIDTH)-3; k >= i; k--)
	{
		// s[strlen(s)+1] looks extremely cursed to me I gotta be honest,
		// but https://pythonexamples.org/c/how-to-insert-character-at-specific-index-in-string
		// says otherwise. I've capped this in the initilization of k, just to be sure.
		s[k+1] = s[k];
	}
	s[i] = c;
}
static void delete_char(char *s, int i)
{
	for(; i < strlen(s); i++)
	{
		if(i+1>=strlen(s)) { s[i] = '\0'; return; }
		s[i] = s[i+1];
	}
}

static void print_directive()
{
	draw_rect_spi(0, 0, LCD_WIDTH-1, FONT_HEIGHT, BLACK);	// Clear first row
	
	short temp_x = lcd_get_current_x();
	short temp_y = lcd_get_current_y();
	lcd_reset_coords();	// Temporarily draw first row
	lcd_print_string(">");
	
	int i = 0;
	for(; i < cursor_pos; i++)
	{
		lcd_putc(0, directive[i]);
	}
	lcd_putc(0, '\017');
	
	if(i>=strlen(directive))
	{
		lcd_putc(0, ' ');
		lcd_putc(0, '\016');
		return;
	}
	
	lcd_putc(0, directive[i++]);
	lcd_putc(0, '\016');
	for(; i < strlen(directive); i++)
	{
		lcd_putc(0, directive[i]);
	}
	lcd_set_coords(temp_x, temp_y);	// Return to wherever we have drawn before
}


static void kbd_handler_task(void *data)
{
	int key;
	for(;;)
	{
		key = read_i2c_kbd();
		if(key == -1) { continue; }
		
		switch(app_focus)
		{
			case 0: directive_key_press(key); break; 
		}
	}
	vTaskDelete(NULL);
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
	
	gpio_init(LEDPIN);
	gpio_set_dir(LEDPIN, GPIO_OUT);

	gpio_put(LEDPIN, 1);
	sleep_ms(1600);
	
	lcd_set_colours(WHITE, BLACK);
	// lcd_set_colours(RGB(128, 0, 32), BLACK);
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
