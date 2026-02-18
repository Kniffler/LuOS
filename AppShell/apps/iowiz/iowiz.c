#include "iowiz.h"

static int rID = 0;

static TaskHandle_t uart_draw_task_handle;
static TaskHandle_t uart_tx_task_handle;	// Sending chars
static TaskHandle_t uart_rx_task_handle;	// Receiving chars

static QueueHandle_t uart_tx_queue_handle;	// Sending chars to this queue will send them
static QueueHandle_t uart_rx_queue_handle;	// Sending chars to this queue will print them


static void uart_rx_task(void *data)
{
	uart_init(UART_CON, 115200);
	uart_set_format(UART_CON, 8, 1, UART_PARITY_NONE);  // 8-N-1

	uart_set_hw_flow(UART_CON, false, false);
	
	gpio_set_function(UART_TX, GPIO_FUNC_UART);
	gpio_set_function(UART_RX, GPIO_FUNC_UART);
	
	uart_set_fifo_enabled(UART_CON, true);
	
	static uint16_t buf_index = 0;
	sleep_ms(100);

	uart_putc_raw(UART_CON, '\r');
	uart_putc_raw(UART_CON, '\n');
	
	uint32_t last_read_time = time_us_32();

	unsigned char c = 0;
	const unsigned char n = '\n';
	const unsigned char sp = ' ';
	const unsigned char b = '\b';
	for(;;)
	{
		while(uart_is_readable(UART_CON))
		{
			c = uart_getc(UART_CON);
			switch(c)
			{
				case '\r': break;
				// case '\n':
				// 	xQueueSend(uart_rx_queue_handle, &n, 0);
				// 	break;
				default:
					xQueueSend(uart_rx_queue_handle, &c, 0);
					break;
			}
		}
		vTaskDelay(1);
	}
	vTaskDelete(NULL);
}

static void uart_draw_task(void *data)
{
	unsigned char c = 0;
	for(;;)
	{
		xQueueReceive(uart_rx_queue_handle, (void *)&c, portMAX_DELAY);
		switch(c)
		{
			case '\b':
				lcd_print_string(rID, "\b \b");
				break;
			default:
				lcd_put_char(rID, c, 0);
				break;
		}
		vTaskDelay(1);
	}
	vTaskDelete(NULL);
}
static void uart_tx_task(void *data)
{
	unsigned char c = 0;
	for(;;)
	{
		xQueueReceive(uart_tx_queue_handle, (void *)&c, portMAX_DELAY);
		// xQueueSend(uart_rx_queue_handle, &c, 0);	// The echo handles this
		uart_putc_raw(UART_CON, c);
		vTaskDelay(1);
	}
	vTaskDelete(NULL);
}


int iowiz_open(int draw_rID)
{
	rID = draw_rID;
	lcd_print_string(rID, "IOWIZ has been started in UART mode\n...\n");

	uart_tx_queue_handle = xQueueCreate(CHAR_AREA, sizeof(unsigned char));
	uart_rx_queue_handle = xQueueCreate(CHAR_AREA, sizeof(unsigned char));
	sleep_ms(100);
	
	xTaskCreate(uart_draw_task, "UART drawer", 2048, (void *) NULL, 11, &uart_draw_task_handle);
	xTaskCreate(uart_rx_task, "UART receiver", 2048, (void *) NULL, 11, &uart_rx_task_handle);
	xTaskCreate(uart_tx_task, "UART transmitter", 2048, (void *) NULL, 11, &uart_tx_task_handle);

	lcd_print_string(rID, "UART mode initialized...\n");
	sleep_ms(250);
	
	lcd_region_clear(rID);
	lcd_reset_coords(rID);
}
void iowiz_close()
{
	vTaskDelete(uart_draw_task_handle);
	vTaskDelete(uart_tx_task_handle);
	vTaskDelete(uart_tx_task_handle);
	
	vQueueDelete(uart_tx_queue_handle);
	vQueueDelete(uart_rx_queue_handle);
}
void iowiz_directive_sent(char *dir)
{
	
}
void iowiz_key_press(int key)
{
	key = (unsigned char)key;
	const unsigned char cr = '\r';
	const unsigned char lf = '\n';
	const unsigned char b = '\b';
	switch(key)
	{
		
		case KEY_MOD_CTRL:
		case KEY_MOD_SHL:
		case KEY_MOD_SHR:
		case KEY_MOD_ALT:
		case KEY_CAPS_LOCK:
		
			break;
		case KEY_BACKSPACE:
			xQueueSend(uart_tx_queue_handle, &b, 0);
			break;
		case KEY_ENTER:
			// xQueueSend(uart_tx_queue_handle, &cr, 0);
			xQueueSend(uart_tx_queue_handle, &lf, 0);
			break;
		default:
			// if(!isprint(key)) { break; }
			
			// xQueueSend(uart_rx_queue_handle, &key, 0);
			xQueueSend(uart_tx_queue_handle, &key, 0);
			break;
	}
	return;
}

