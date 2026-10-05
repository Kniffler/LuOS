#include "buffy.h"
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/timer.h>
#include <stdlib.h>
#include <pico/stdlib.h>
#include <pico/malloc.h>
#include <hardware/sync.h>
#include <stdint.h>
#include <src/include/debug.h>


static void raise_dc(void) { gpio_put(LCD_DC, 1); }
static void lower_dc(void) { gpio_put(LCD_DC, 0); }


static void spi_write16_colour1px(const uint16_t colour)
{
	spi_set_format(LCD_SPI_MOD, 16, 0, 0, SPI_MSB_FIRST);
	raise_dc(); // Data
	spi_write16_blocking(LCD_SPI_MOD, &colour, 1);
	spi_set_format(LCD_SPI_MOD, 8, 0, 0, SPI_MSB_FIRST);
}
static void spi_write16_colour_buf(const uint16_t *buffer, size_t len)
{
	// DO NOT MOVE THE spi_set_format() OR THE raise_dc() CALLS!
	// They are placed before the raise_dc() to ensure that a minimum
	// chip select high pulse width is achieved (at least 40ns)
	spi_set_format(LCD_SPI_MOD, 16, 0, 0, SPI_MSB_FIRST);

	raise_dc(); // Data
	spi_write16_blocking(LCD_SPI_MOD, buffer, len);

	spi_set_format(LCD_SPI_MOD, 8, 0, 0, SPI_MSB_FIRST);
}

static void spi_write_command(uint8_t cmd)
{
	lower_dc();
	spi_write_blocking(LCD_SPI_MOD, &cmd, 1);
}
static void spi_write_full_command(uint8_t cmd, int argc, ...)
{
	va_list args;
	va_start(args, argc);
	lower_dc();
	spi_write_blocking(LCD_SPI_MOD, &cmd, 1);
	raise_dc();
	for(int i = 0; i < argc; i++)
	{
		spi_write_blocking(LCD_SPI_MOD, &(uint8_t){va_arg(args, int)}, 1);
	}
	va_end(args);
}

extern void lcd_display_on(void)
{
	uint32_t irq = save_and_disable_interrupts();
	spi_write_command(LCD_CMD_DISPON);
	restore_interrupts(irq);
}
extern void lcd_display_off(void)
{
	uint32_t irq = save_and_disable_interrupts();
	spi_write_command(LCD_CMD_DISPOFF);
	restore_interrupts(irq);
}
extern void lcd_reset(void)
{
	gpio_put(LCD_RST, 0);
	busy_wait_us(20);

	gpio_put(LCD_RST, 1);
	busy_wait_ms(120);
}

static void __not_in_flash_func(normalize_coords)(uint16_t *x1, uint16_t *y1, uint16_t *x2, uint16_t *y2)
{
	// uint16_t x_over1 = (*x1)/LCD_RES_H, y_over1 = (*y1)/LCD_MEM_HEIGHT;
	// uint16_t x_over2 = (*x2)/LCD_RES_H, y_over2 = (*y2)/LCD_MEM_HEIGHT;
	*x1 %= LCD_RES_H;
	*y1 %= LCD_MEM_HEIGHT;
	*x2 %= LCD_RES_H;
	*y2 %= LCD_MEM_HEIGHT;
	// if(x_over1) { (*y1) = x_over1; }
	// if(x_over2) { (*y2) = x_over2; }
	// if(y_over1) { (*y1) = y_over1; }
	// if(y_over2) { (*y2) = y_over2; }
}

static void __not_in_flash_func(define_spi_region)(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	normalize_coords(&x1, &y1, &x2, &y2);

	spi_write_full_command(LCD_CMD_CASET, 4, (x1 >> 8), (x1 & 0xFF), (x2 >> 8), (x2 & 0xFF));

	spi_write_full_command(LCD_CMD_RASET, 4, (y1 >> 8), (y1 & 0xFF), (y2 >> 8), (y2 & 0xFF));

	spi_write_command(LCD_CMD_RAMWR); // Reset writing coords and start writing to ram
	busy_wait_us(1);
}

extern void lcd_scroll(int lines)
{
	lines %= LCD_RES_V;
	if(lines<0) { lines = LCD_RES_V+(lines*(-1)); }
	lines %= LCD_RES_V;

	// spi_write_full_command(LCD_CMD_VSCRDEF, 6, 0x00, 0x00, 0x01, 0x40, 0x00, 0xA0);
	spi_write_full_command(LCD_CMD_VSCSAD, 2, (lines >> 8), (lines & 0xFF));
	// spi_write_command(LCD_CMD_RAMWR);
	// spi_write_command(LCD_CMD_NOP);
}

static int lcd_controller_init(void)
{
	gpio_init(LCD_CS);
	gpio_init(LCD_DC);
	gpio_init(LCD_MOSI_TX);
	gpio_init(LCD_MISO_RX);
	gpio_init(LCD_RST);
	gpio_init(LCD_SCK);

	gpio_set_dir(LCD_CS, GPIO_OUT);
	gpio_set_dir(LCD_DC, GPIO_OUT);
	gpio_set_dir(LCD_MOSI_TX, GPIO_OUT);
	gpio_set_dir(LCD_MISO_RX, GPIO_OUT);
	gpio_set_dir(LCD_RST, GPIO_OUT);
	gpio_set_dir(LCD_SCK, GPIO_OUT);

	int ret = spi_init(LCD_SPI_MOD, LCD_SPI_FREQ);
	ret = spi_set_baudrate(LCD_SPI_MOD, LCD_SPI_FREQ);
	gpio_set_function(LCD_SCK, GPIO_FUNC_SPI);
	gpio_set_function(LCD_MOSI_TX, GPIO_FUNC_SPI);
	gpio_set_function(LCD_MISO_RX, GPIO_FUNC_SPI);

	gpio_put(LCD_RST, 1);

	uint32_t irq = save_and_disable_interrupts();

	lcd_reset();

	// Set RGB mode to RGB565
	spi_write_full_command(LCD_CMD_COLMOD, 1, 0b01010101);

	// Memory Control
	spi_write_full_command(LCD_CMD_MADCTL, 1, 0b01001000);
	// MY=0, MX=1, MV=0, ML=0, RGB=1, MH=0

	// Colour inversion on
	spi_write_command(LCD_CMD_INVON);

	// Entry Mode Set
	spi_write_full_command(LCD_CMD_EMS, 3, 0xC6, 0xE9, 0x00);

	// Vertical scroll definition
	spi_write_full_command(LCD_CMD_VSCRDEF, 6, 0x00, 0x00, 0x01, 0xE0, 0x00, 0x00);
	// spi_write_full_command(LCD_CMD_VSCRDEF, 6, 0x00, 0x00, 0x01, 0x40, 0x00, 0xA0);

	// Sleep out
	spi_write_command(LCD_CMD_SLPOUT);

	restore_interrupts(irq);
	busy_wait_us(10000);

	lcd_clear(0x0000);

	// Turn on display
	lcd_display_on();
	return ret;
}

extern int lcd_draw_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t colour)
{
	define_spi_region(x, y, x+width-1, y+height-1);
	uint16_t *line_buffer = (uint16_t*)malloc(sizeof(uint16_t)*width);
	if(!line_buffer) { return 1; }

	for(size_t i = 0; i < width;  i++) { line_buffer[i] = colour; }

	uint32_t irq = save_and_disable_interrupts();
	for(size_t i = 0; i < height;  i++) { spi_write16_colour_buf(line_buffer, width); }

	restore_interrupts(irq);
	free(line_buffer);
	return 0;
}

extern int lcd_draw_image(uint16_t *buffer, size_t buf_length, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
	if(buf_length != width*height || width == 0 || height == 0) { return 1; }

	define_spi_region(x, y, x+width-1, y+height-1);
	spi_write16_colour_buf(buffer, buf_length);
	return 0;
}
static int lcd_draw_bitmap_partial(uint8_t *buffer, size_t ignoreFirstXPixels, size_t skipXpixelsAfterLine, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t fc, uint16_t bc)
{
	define_spi_region(x, y, x+width-1, y+height-1);
	uint16_t *line_buffer = (uint16_t*)malloc(sizeof(uint16_t)*width);
	if(!line_buffer || width == 0 || height == 0) { return 1; }

	uint32_t irq = save_and_disable_interrupts();
	for(size_t i = 0; i < height; i++)
	{
		for(size_t k = 0; k < width; k++)
		{
			size_t passedPixels = ((i*(width+skipXbitsAfterLine))+k) + ignoreFirstXPixels;
			// DEBUG_PRINT("Passed Pixels: %d\n", passedPixels);
			// DEBUG_PRINT("Drawing byte 0b%08b\n", buffer[passedPixels/8]);
			line_buffer[k] = ( (buffer[passedPixels/8] >> ( (((width+skipXbitsAfterLine)*height) - passedPixels - 1)%8 )) & 1 ) ? fc : bc;
			// DEBUG_PRINT("Drawing buffer 0x%02X, (i=%d, k=%d)\n", line_buffer[k], i, k);
		}
		spi_write16_colour_buf(line_buffer, width);
	}
	restore_interrupts(irq);
	free(line_buffer);
	return 0;
}
extern int lcd_draw_bitmap(uint8_t *buffer, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t fc, uint16_t bc)
{
	return lcd_draw_bitmap_partial(buffer, 0, 0, x, y, width, height, fc, bc);
}

static unsigned char* lcd_get_pointer_for_char_bitmap(unsigned char c, font_t font)
{
	return font + 4 + (int)( ((c - font[2])*font[0]*font[1])/8 );
}

extern void lcd_draw_string_on_line(unsigned char *str, size_t length, uint16_t x, uint16_t y, font_t font, uint16_t fc, uint16_t bc)
{
	if(x+(length*font[0])>LCD_RES_H || y+font[1]>LCD_RES_V) { return; }

	define_spi_region(x, y, x+(length*font[0])-1, y+font[1]-1);

	uint8_t font_width = font[0], font_height = font[1];

	size_t buf_length = length*font_width*font_height;
	uint16_t *line_buffer = malloc(sizeof(uint16_t)*buf_length);
	if(!line_buffer) { return; }

	for(size_t row = 0; row < font_height; row++)
	{
		for(size_t strIndex = 0; strIndex < length; strIndex++)
		{
			unsigned char *c = lcd_get_pointer_for_char_bitmap(str[strIndex], font);
			c += ( row * font_width )/8;

			for(size_t col = 0; col < font_width; col++)
			{
				uint8_t shiftVal = 7 - ((row*font_width + col)%8);
				uint8_t bit = ( (*c) >> shiftVal) & 1;
				line_buffer[row*length*font_width + strIndex*font_width + col] = (bit) ? fc : bc;
			}
		}
	}
	spi_write16_colour_buf(line_buffer, buf_length);
	free(line_buffer);
}

extern int lcd_draw_char(char c, uint16_t x, uint16_t y, font_t font, uint16_t fc, uint16_t bc)
{
	unsigned char* bitmap = lcd_get_pointer_for_char_bitmap(c, font);

	size_t effective_width = MIN(font[0], LCD_RES_H-x);
	size_t effective_height = MIN(font[1], LCD_RES_V-y);

	int c1 = lcd_draw_bitmap_partial(bitmap, 0, font[0]-effective_width,
									 x, y, effective_width, effective_height, fc, bc);
	if(c1) { return 1; }

	// Simplest case, entire glyph already drawn.
	if(effective_height==font[1] && effective_width==font[0]) { return 0; }

	// The glyph cuts the right edge of the screen.
	if(effective_width<font[0] && effective_height==font[1])
	{
		int c2 = lcd_draw_bitmap_partial(bitmap, effective_width, effective_width,
										 0, (y+font[1])%LCD_RES_V, font[0]-effective_width, effective_height, fc, bc);
		if(c2) { return 1; }

		/* I do NOT wanna talk about this case nor the headaches it gave me.
		 * 	This is the scenario in which we cute the right screen edge, but once we loop
		 * the glyph around, we also cut the bottom and so we have to print back at the top.
		 * I hate this.
		 */
		size_t temp_effective_height = LCD_RES_V - ((y+font[1])%LCD_RES_V);
		if(temp_effective_height < font[1])
		{
			int c6 = lcd_draw_bitmap_partial(bitmap, temp_effective_height*font[0] + effective_width, effective_width,
											 0, 0, font[0]-effective_width, font[1]-temp_effective_height, fc, bc);
			if(c6) { return 1; }

		}
	// The case we cut the bottom of the screen.
	} else if(effective_height<font[1] && effective_width==font[0])
	{
		int c3 = lcd_draw_bitmap_partial(bitmap, effective_width*effective_height, 0,
										 x, 0, effective_width, font[1]-effective_height, fc, bc);
		if(c3) { return 1; }
	// The case we cut the bottom right corner of the screen - yes that is different from the case I complained about above.
	} else
	{
		int c4 = lcd_draw_bitmap_partial(bitmap, effective_width, effective_width,
										 0, font[1]-effective_height, font[0]-effective_width, font[1], fc, bc);

		int c5 = lcd_draw_bitmap_partial(bitmap, font[0]*effective_height, font[0]-effective_width,
										 x, 0, effective_width, font[1]-effective_height, fc, bc);
		if(c4||c5) { return 1; }
	}

	return 0;
}

static void lcd_handle_xy_char(uint16_t *x, uint16_t *y, uint16_t x_limit, uint16_t y_limit, font_t font)
{
	uint16_t nx = (*x)+font[0];
	uint16_t ny = (*y)+font[1];
	*x = nx%x_limit;
	if( nx/x_limit > 0) { *y = ny%y_limit; }
}
static void lcd_handle_xy_char_reverse(uint16_t *x, uint16_t *y, uint16_t x_limit, uint16_t y_limit, font_t font)
{
	int16_t ox = (*x)-font[0];
	int16_t oy = (*y)-font[1];

#define IF_NEGATIVE_ADD(a, x) ((a<0) ? (a)+(x) : (a))
#define INA(a, x) IF_NEGATIVE_ADD(a, x)

	*x = INA(ox, x_limit)%x_limit;
	if(ox < 0) { *y = INA(oy, y_limit)%y_limit; }
}
extern void lcd_draw_string(unsigned char *str, size_t length, uint16_t x, uint16_t y, font_t font, uint8_t tabsize, uint16_t fc, uint16_t bc)
{
	uint16_t cur_x = x, cur_y = y;
	uint16_t cur_fc = fc, cur_bc = bc;
	for(size_t i = 0; i < length; i++)
	{
		unsigned char c = str[i];
		if(c >= 32 && c <= 126 )
		{
			 // On error, preserve the letter and try printing in the next available space.
			if(lcd_draw_char(c, cur_x, cur_y, font, cur_fc, cur_bc))
			{ i--; }
			lcd_handle_xy_char(&cur_x, &cur_y, LCD_RES_H, LCD_RES_V, font);
			continue;
		}
		switch(c)
		{
			case '\0': return; break;
			case '\n':
				cur_x = LCD_RES_H;
				lcd_handle_xy_char(&cur_x, &cur_y, LCD_RES_H, LCD_RES_V, font);
				break;
			case '\r':
				cur_x = 0;
				break;
			case '\t':
				if(x+(tabsize*font[0])>=LCD_RES_H ) { break; }
				while(x%tabsize)
				{
					lcd_draw_char(' ', cur_x, cur_y, font, cur_fc, cur_bc);
					lcd_handle_xy_char(&cur_x, &cur_y, LCD_RES_H, LCD_RES_V, font);
				}
				break;
			case '\b':
				lcd_handle_xy_char_reverse(&cur_x, &cur_y, LCD_RES_H, LCD_RES_V, font);
				break;
			case '\017':
				cur_fc = bc;
				cur_bc = fc;
				break;
			case '\016':
				cur_fc = fc;
				cur_bc = bc;
				break;
			default: continue; break;
		}
	}
}

extern void lcd_clear(uint16_t colour)
{
	lcd_draw_rect(0, 0, 320, 320, colour);
}
extern int lcd_init(void)
{
	int ret = lcd_controller_init();
	// lcd_clear(0x0000);
	return ret;
}
