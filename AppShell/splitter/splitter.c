#include "splitter.h"
// #include "fonts/font1.h"
#include "fonts/LuOS_System_Font.h"

void splitter_init()
{
	int rID = lcd_region_create(0, 0, LCD_WIDTH, LCD_HEIGHT);
	lcd_region_set_asthetics(rID, ORANGE, BLACK, 1, SHIFT_DOWNWARDS, (unsigned char *)LuOS_System_Font_data);
	lcd_print_string(rID, "sphinx of black quartz, judge my vow.\n");
	lcd_print_string(rID, "SPHINX OF BLACK QUARTZ; JUDGE MY VOW:\n");
	lcd_print_string(rID, " 0 1 2 3 4 5 6 7 8 9\n\\ / < = > ( ) [ ] { }\n # @ % & ? ! - + * \"");

	for(;;) {  }
}
