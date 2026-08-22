#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "src/lcdspi/lcdspi.h"
#include "src/i2ckbd/i2ckbd.h"
#include "src/lcdspi/fonts/LuOS_System_Font.h"

unsigned char* mainFont = (unsigned char*)LuOS_System_Font_data;

int main()
{
	stdio_init_all();
	lcd_init();
	init_i2c_kbd();

	int rID = lcd_region_create(0, 0, LCD_WIDTH, LCD_HEIGHT);
	lcd_region_set_asthetics(0, MAGENTA, BLACK, true, SHIFT_UPWARDS, mainFont);
	lcd_reset_coords(rID);
	lcd_region_clear(rID);
	lcd_print_string(rID, "Sup motherfucker, press any button to check on your battery\n");

	int c = -1;
	for(;;) {
		c = read_i2c_kbd();
		if(c==-1) { tight_loop_contents(); continue; }

		int bat_pcnt = read_battery();
		bat_pcnt >>= 8;
		bool bat_charging = (bat_pcnt>>7)&1;
		bat_pcnt &= 0b01111111; // 0x7F;
		char msg[128];
		snprintf(msg, sizeof(msg), "Battery %s at %d%%\n", (bat_charging) ? "charging" : "discharging", bat_pcnt);
		lcd_print_string(rID, msg);
	}
}
