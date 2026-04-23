#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "src/lcdspi/lcdspi.h"

int main()
{
	stdio_init_all();
	lcd_init();
	for(;;)
	{
		printf("I AM ALIVE\n");
	}
	int rID = lcd_region_create(0, 0, LCD_WIDTH, LCD_HEIGHT);
	lcd_reset_coords(rID);
	lcd_print_string(rID, "Sup motherfucker\n");
	for(;;) { tight_loop_contents(); }
}
