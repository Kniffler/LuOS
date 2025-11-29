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

#define DIR_LEN ((LCD_WIDTH/FONT_WIDTH)-1+2)

const unsigned int LEDPIN = 25;

uint16_t app_focus = 0;

void all_init();

static void kbd_handler_task(void *data);
bool is_printable_key(int key) { return key>31&&key<127; };

static char directive_history[64][DIR_LEN] = {'\0'};
static char directive_changed_history[64][DIR_LEN] = {'\0'};
static short history_pointer = 0;
static short preview_pointer = 0;

static void copy_directive_history();

static char directive[DIR_LEN] = {'\0'};
static short cursor_pos = 0;

static void directive_key_press(int key);
static uint8_t directive_execute();
static char directive_spacer[DIR_LEN];

static void print_directive(uint8_t deb_num);

static void directive_char_insert(int i, char c);
static void directive_char_delete(int i);


void main(void)
{
	all_init();
	
	TaskHandle_t kbd_handler = NULL;
	BaseType_t xReturned;
	xReturned = xTaskCreate(kbd_handler_task, "kbd_handler", 128, (void *) NULL, KBD_TASK_PRIORITY, &kbd_handler);


	for(int i = 0; i < DIR_LEN; i++)
	{
		directive_spacer[i] = '-';
	}
	directive_spacer[DIR_LEN-1] = '\0';
	
	lcd_clear();
	lcd_reset_coords();
	
	print_directive(0);
	
	lcd_print_string(directive_spacer);
	lcd_print_string("\n");

	vTaskStartScheduler();
}


static uint8_t directive_execute()
{
	/*	The ordering of conditions in this function
		is highly delicate, 
	*/
	char *directive_subject = (history_pointer==preview_pointer) ? 
		directive : directive_changed_history[preview_pointer];
		
	if(strlen(directive_subject)<=0) { return 1; }

	
	if(strcmp(directive_subject, ".clear")==0)
	{
		lcd_clear();
		lcd_reset_coords();

		lcd_print_string("\n");
		lcd_print_string(directive_spacer);
		lcd_print_string("\n");
	}else if(strcmp(directive_subject, ".test")==0)
	{
		lcd_print_string(directive_spacer);
	}


	bool is_not_repeating = history_pointer<1||strcmp(directive_subject, directive_history[history_pointer-1])!=0;
	
	//	Only save non-repeating directives, for ease of navigation
	if(is_not_repeating)
	{
		strcpy(directive_history[history_pointer], directive_subject);
		strcpy(directive_changed_history[history_pointer], directive_subject);
		
	}
	
	if(history_pointer==preview_pointer)
	{
		cursor_pos = 0;
		strcpy(directive, '\0');
	}else
	{
		cursor_pos = strlen(directive);
		copy_directive_history();
	}
	
	if(is_not_repeating) { history_pointer++; }
	
	preview_pointer = history_pointer;
	
	return 0;
}

static void directive_key_press(int key)
{
	char *directive_subject = (history_pointer==preview_pointer) ? 
		directive : directive_changed_history[preview_pointer];

	
	bool execute_at_end = false;
	
	if(is_printable_key(key) && cursor_pos+1<(LCD_WIDTH/FONT_WIDTH))
	{
		directive_char_insert(cursor_pos++, (char)key);
	}
	
	switch(key)
	{
		case KEY_HOME: app_focus = 0;	break;
		case KEY_BACKSPACE:
			if(cursor_pos==0) { directive_char_delete(cursor_pos); break; }
			else if(cursor_pos-1<0) { break; }
			
			if(directive_subject[cursor_pos-1]>0) { directive_char_delete(--cursor_pos); }
			else if(directive_subject[cursor_pos]>0) { directive_char_delete(cursor_pos); }
			break;
		case KEY_ENTER: execute_at_end = true; break;
		case KEY_LEFT:
			if(cursor_pos-1<0) { break; }
			cursor_pos--;
			break;
		case KEY_RIGHT:
			if(cursor_pos>=strlen(directive_subject)) { break; }
			cursor_pos++;
			break;
		
		
		case KEY_UP:
			if(preview_pointer-1<0) { break; }
			preview_pointer--;
			cursor_pos = strlen(directive_changed_history[preview_pointer]);
			break;
		case KEY_DOWN:
			if(preview_pointer+1>history_pointer) { break; }
			preview_pointer++;
			cursor_pos = strlen(directive_changed_history[preview_pointer]);
			break;
		
		default: break;
	}
	if(cursor_pos>DIR_LEN-3) { cursor_pos = DIR_LEN-3;}

	if(execute_at_end) { directive_execute(); }

	print_directive(0);
}

static void copy_directive_history()
{
	for(int i = 0; i < 64; i++)
	{
		strcpy(directive_changed_history[i], directive_history[i]);
	}
}

static void directive_char_insert(int i, char c)
{
	char *directive_subject = (history_pointer==preview_pointer) ? 
		directive : directive_changed_history[preview_pointer];
	
	for(int k = DIR_LEN-2; k >= i; k--)
	{
		// s[strlen(s)+1] looks extremely cursed to me I gotta be honest,
		// but https://pythonexamples.org/c/how-to-insert-character-at-specific-index-in-string
		// says otherwise. I've capped this in the initilization of k, just to be sure.

		// Ignore my outdated comment above
		directive_subject[k+1] = directive_subject[k];
	}
	directive_subject[i] = c;
	
}
static void directive_char_delete(int i)
{
	char *directive_subject = (history_pointer==preview_pointer) ? 
		directive : directive_changed_history[preview_pointer];
	
	for(; i < strlen(directive_subject); i++)
	{
		if(i+1>=strlen(directive_subject))
			{ directive_subject[i] = '\0'; return; }
		directive_subject[i] = directive_subject[i+1];
	}
}

static void print_directive(uint8_t deb_num)
{
	/*
	if(history_pointer!=preview_pointer)
		{ lcd_print_string("Found ourselves printing in the past\n");}
	else
		{ lcd_print_string("Found ourselves printing in the present\n");}
	*/
	char *print_subject = (history_pointer==preview_pointer) ? 
		directive : directive_changed_history[preview_pointer];
	
	int temp_x = lcd_get_current_x();
	int temp_y = lcd_get_current_y();
	lcd_reset_coords();	// Temporarily draw first row
	draw_rect_spi(0, 0, LCD_WIDTH-1, FONT_HEIGHT, BLACK);	// Clear first row
	lcd_print_string(">");

	directive_char_insert(cursor_pos, '\017');
	directive_char_insert(cursor_pos+2, '\016');
	
	lcd_print_string(print_subject);

	if(strlen(print_subject)<=DIR_LEN-1) { lcd_print_string(" "); }
	
	lcd_print_string("\016\n");
	
	directive_char_delete(cursor_pos);
	directive_char_delete(cursor_pos+1);
	/*
	lcd_print_string(directive_spacer);
	lcd_print_string("\n");
	*/
	lcd_set_coords(temp_x, temp_y);	// Return to wherever we have drawn before
	fflush(stdout);
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
