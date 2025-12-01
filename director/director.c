#include "director.h"

uint16_t app_focus = 0;

static void uart_test_task(void *data)
{
	uart_init(uart1, 115200);
	uart_set_format(uart1, 8, 1, UART_PARITY_NONE);  // 8-E-1

	uart_set_hw_flow(uart1, false, false);
	
	gpio_set_function(4, GPIO_FUNC_UART);
	gpio_set_function(5, GPIO_FUNC_UART);
	
	uart_set_fifo_enabled(uart1, true);

	static uint16_t buf_index = 0;
	static uint8_t fifo_buffer[CHAR_AREA];
	
	sleep_ms(100);

	uint32_t last_read_time = time_us_32();
	
	for(;;)
	{
		while(uart_is_readable(uart1))
		{
			char c = uart_getc(uart1);
			last_read_time = time_us_32();

			if(buf_index < CHAR_AREA-1)
			{
				fifo_buffer[buf_index++] = c;
			}else
			{
				for(int i = 1; i < buf_index; i++)
				{
					fifo_buffer[i-1] = fifo_buffer[i];
				}
				fifo_buffer[CHAR_AREA-1] = c;
			}
		}
		if(buf_index>0 && (time_us_32() - last_read_time) > 64000)
		{
			for(int i = 0; i < buf_index; i++) {
				lcd_put_char(fifo_buffer[i], 0);
			}
			buf_index = 0;
		}
	}
	vTaskDelete(NULL);
}

void dir_init()
{
	for(int i = 0; i < DIR_LEN; i++)
	{
		directive_spacer[i] = '-';
	}
	directive_spacer[DIR_LEN-1] = '\0';
}

void dir_clear()
{
	lcd_clear();
	lcd_set_coords(0, 0);

	print_directive(0);
	lcd_print_string("\n");
	// Necessary as the print_directive function returns to the previous point 
	// of printing without making changes
	
	lcd_print_string(directive_spacer);
	lcd_print_string("\n");
	lcd_set_reset_current();
}


static uint8_t directive_execute()
{
	/*	The ordering of conditions in this function
		is highly delicate, do not touch
	*/
	char *directive_subject = (history_pointer==preview_pointer) ? 
		directive : directive_changed_history[preview_pointer];
		
	if(strlen(directive_subject)<=0) { return 1; }

	if(directive_subject[0]==',')
	{

	}else if(strcmp(directive_subject, ".clear")==0)
	{
		dir_clear();
	}else if(strcmp(directive_subject, ".test")==0)
	{
		xTaskCreate(uart_test_task, "uart shittery", 128, (void *) NULL, 10, NULL);
	}


	bool is_not_repeating = history_pointer<1||strcmp(directive_subject,directive_history[history_pointer-1])!=0;
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

void directive_key_press(int key)
{
	char *directive_subject = (history_pointer==preview_pointer) ? 
		directive : directive_changed_history[preview_pointer];

	
	bool execute_at_end = false;
	
	if(isprint(key) && cursor_pos+1<(LCD_WIDTH/FONT_WIDTH))
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
	char *print_subject = (history_pointer==preview_pointer) ? 
		directive : directive_changed_history[preview_pointer];
	
	int temp_x = lcd_get_current_x();
	int temp_y = lcd_get_current_y();
	lcd_set_coords(0, 0);	// Temporarily draw first row
	draw_rect_spi(0, 0, LCD_WIDTH-1, FONT_HEIGHT, BLACK);	// Clear first row
	lcd_print_string(">");

	directive_char_insert(cursor_pos, '\017');
	directive_char_insert(cursor_pos+2, '\016');
	
	lcd_print_string(print_subject);

	if(strlen(print_subject)<=DIR_LEN-1) { lcd_print_string(" "); }
	
	lcd_print_string("\016\n");
	
	directive_char_delete(cursor_pos);
	directive_char_delete(cursor_pos+1);
	
	lcd_set_coords(temp_x, temp_y);	// Return to wherever we have drawn before
	fflush(stdout);
}

