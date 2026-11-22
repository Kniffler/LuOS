#include "buffy.h"
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/timer.h>
#include <stdlib.h>
#include <pico/stdlib.h>
#include <pico/malloc.h>
#include <hardware/sync.h>
#include <string.h>
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

extern void lcd_ShowDisplayTop(void)
{
	uint32_t irq = save_and_disable_interrupts();
	spi_write_command(LCD_CMD_NORON);
	restore_interrupts(irq);
}
extern void lcd_ShowDisplayBot(void)
{
	uint32_t irq = save_and_disable_interrupts();
	lcd_scroll(160, 0, LCD_RES_H, 0, LCD_MEM_HEIGHT);
	restore_interrupts(irq);
}


extern void lcd_DisplayOn(void)
{
	uint32_t irq = save_and_disable_interrupts();
	spi_write_command(LCD_CMD_DISPON);
	restore_interrupts(irq);
}
extern void lcd_DisplayOff(void)
{
	uint32_t irq = save_and_disable_interrupts();
	spi_write_command(LCD_CMD_DISPOFF);
	restore_interrupts(irq);
}
extern void lcd_Reset(void)
{
	gpio_put(LCD_RST, 0);
	busy_wait_us(20);

	gpio_put(LCD_RST, 1);
	busy_wait_ms(120);
}

static void __not_in_flash_func(normalize_coords)(uint16_t *x1, uint16_t *y1, uint16_t *x2, uint16_t *y2)
{
	*x1 %= LCD_RES_H;
	*y1 %= LCD_MEM_HEIGHT;
	*x2 %= LCD_RES_H;
	*y2 %= LCD_MEM_HEIGHT;
}

static void __not_in_flash_func(define_spi_region)(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	normalize_coords(&x1, &y1, &x2, &y2);

	spi_write_full_command(LCD_CMD_CASET, 4, (x1 >> 8), (x1 & 0xFF), (x2 >> 8), (x2 & 0xFF));

	spi_write_full_command(LCD_CMD_RASET, 4, (y1 >> 8), (y1 & 0xFF), (y2 >> 8), (y2 & 0xFF));

	spi_write_command(LCD_CMD_RAMWR); // Reset writing coords and start writing to ram
	busy_wait_us(1);
}

extern void lcd_scroll(MBNA int lines, uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2)
{
	uint16_t width = x2-x1;
	uint16_t height = y2-y1;

	if(x1>=x2 || y1>=y2) { DEBUG_PRINT_ERR("Scrolling coordinates are seemingly reversed, (%d>=%d or %d>=%d)\n", x1, x2, y1, y2); return; }
	if(x1>LCD_RES_H || x2>LCD_RES_H || y1>LCD_MEM_HEIGHT || y2>LCD_MEM_HEIGHT) { DEBUG_PRINT_ERR("Coordinates out of bounds\n"); return; }
	if(y1+height+(LCD_MEM_HEIGHT-y2)!=LCD_MEM_HEIGHT) { DEBUG_PRINT_ERR("Sum of Y coordinates does not add up to resolution\n"); return; }
	// if(x1+width+(LCD_RES_H-x2)!=LCD_MEM_HEIGHT) { DEBUG_PRINT_ERR("Sum of Y coordinates does not add up to resolution\n"); return; }

	lines %= LCD_MEM_HEIGHT;
	if(lines<0) { lines = LCD_MEM_HEIGHT+(lines*(-1)); }
	lines %= LCD_MEM_HEIGHT;


#ifdef BUFFY_USE_LOCAL_MEMORY_BUFFER
	lcd_buffer += (LCD_RES_H*y1)+x1;

	for(size_t i = 0; i < lines; i++)
	{
		for(size_t k = 0; k < width; k++)
		{
			size_t position = (i*(LCD_RES_H-x1))+k;
			size_t replace = position + (lines*(LCD_RES_H-x1));
			uint16_t tmp = lcd_buffer[position];
			lcd_buffer[position] = lcd_buffer[replace];
			lcd_buffer[replace] = tmp;
		}
	}
#else
	spi_write_full_command(LCD_CMD_VSCRDEF, 6, y1>>8, y1&0xFF, (y2-y1)>>8, (y2-y1)&0xF, (LCD_MEM_HEIGHT-y2)>>8, (LCD_MEM_HEIGHT-y2)&0xF);
	spi_write_full_command(LCD_CMD_VSCSAD, 2, (lines >> 8), (lines & 0xFF));
#endif // BUFFY_USE_LOCAL_MEMORY_BUFFER
}

static int lcd_controllerInit(void)
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

	lcd_Reset();

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
	// spi_write_full_command(LCD_CMD_VSCRDEF, 6, 0x00, 0x00, 0x01, 0xE0, 0x00, 0x00);
	// spi_write_full_command(LCD_CMD_VSCRDEF, 6, 0x00, 0x00, 0x01, 0x40, 0x00, 0xA0);

	// Sleep out
	spi_write_command(LCD_CMD_SLPOUT);

	restore_interrupts(irq);
	busy_wait_us(10000);

	lcd_ShowDisplayTop();

	// Turn on display
	lcd_DisplayOn();
	return ret;
}

extern int lcd_Draw1Pixel(MBNA uint16_t x, uint16_t y, uint16_t colour)
{
#ifdef BUFFY_USE_LOCAL_MEMORY_BUFFER
	lcd_buffer[y*LCD_RES_H + x] = colour;
#else
	define_spi_region(x, y, x, y);
	spi_write16_colour1px(colour);
#endif // BUFFY_USE_LOCAL_MEMORY_BUFFER
	return LUOS_SUCCESS;
}

extern int lcd_DrawRect(MBNA uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t colour)
{
	if(width == 0 || height == 0) { return LUOS_ERROR_BOUNDS; }
	if(x>LCD_RES_H || y>LCD_MEM_HEIGHT) { return LUOS_ERROR_BOUNDS; }
	if(x+width>LCD_RES_H || y+height>LCD_MEM_HEIGHT) { return LUOS_ERROR_BOUNDS; }

#ifdef BUFFY_USE_LOCAL_MEMORY_BUFFER
	lcd_buffer += (LCD_RES_H*y)+x;

	for(size_t i = 0; i < height; i++)
	{
		for(size_t k = 0; k < width; k++)
		{
			size_t position = (i*(LCD_RES_H-x))+k;
			lcd_buffer[position] = colour;
		}
	}
#else
	define_spi_region(x, y, x+width-1, y+height-1);
	uint16_t *lineBuffer = (uint16_t*)malloc(sizeof(uint16_t)*width);
	if(!lineBuffer) { return LUOS_ERROR_MEM; }

	for(size_t i = 0; i < width;  i++) { lineBuffer[i] = colour; }

	uint32_t irq = save_and_disable_interrupts();
	for(size_t i = 0; i < height;  i++) { spi_write16_colour_buf(lineBuffer, width); }

	restore_interrupts(irq);
	free(lineBuffer);
#endif // BUFFY_USE_LOCAL_MEMORY_BUFFER
	return LUOS_SUCCESS;
}

extern int lcd_DrawImage(MBNA uint16_t *buffer, size_t buf_length, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
	if(buf_length != width*height || width == 0 || height == 0) { return LUOS_ERROR_BOUNDS; }

	define_spi_region(x, y, x+width-1, y+height-1);
	spi_write16_colour_buf(buffer, buf_length);
	return LUOS_SUCCESS;
}
static int lcd_DrawPartBitmap(MBNA uint8_t *buffer, size_t ignoreFirstXPixels, size_t skipXpixelsAfterLine, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t fc, uint16_t bc)
{
	if(width == 0 || height == 0) { return LUOS_ERROR_BOUNDS; }
#ifdef BUFFY_USE_LOCAL_MEMORY_BUFFER
	lcd_buffer += (LCD_RES_H*y)+x;
	for(size_t i = 0; i < height; i++)
	{
		for(size_t k = 0; k < width; k++)
		{
			size_t passedPixels = ((i*(width+skipXpixelsAfterLine))+k) + ignoreFirstXPixels;

			lcd_buffer[i*(LCD_RES_H)+k] = ( (buffer[passedPixels/8] >> ( (((width+skipXpixelsAfterLine)*height) - passedPixels - 1)%8 )) & 1 ) ? fc : bc;
		}
	}
#else
	define_spi_region(x, y, x+width-1, y+height-1);
	uint16_t *lineBuffer = (uint16_t*)malloc(sizeof(uint16_t)*width);
	if(!lineBuffer) { return LUOS_ERROR_MEM; }

	uint32_t irq = save_and_disable_interrupts();
	for(size_t i = 0; i < height; i++)
	{
		for(size_t k = 0; k < width; k++)
		{
			size_t passedPixels = ((i*(width+skipXpixelsAfterLine))+k) + ignoreFirstXPixels;
			// DEBUG_PRINT("Passed Pixels: %d\n", passedPixels);
			// DEBUG_PRINT("Drawing byte 0b%08b\n", buffer[passedPixels/8]);
			lineBuffer[k] = ( (buffer[passedPixels/8] >> ( (((width+skipXpixelsAfterLine)*height) - passedPixels - 1)%8 )) & 1 ) ? fc : bc;
			// DEBUG_PRINT("Drawing buffer 0x%02X, (i=%d, k=%d)\n", lineBuffer[k], i, k);
		}
		spi_write16_colour_buf(lineBuffer, width);
	}
	restore_interrupts(irq);
	free(lineBuffer);
#endif // BUFFY_USE_LOCAL_MEMORY_BUFFER
	return LUOS_SUCCESS;
}
extern int lcd_DrawBitmap(MBNA uint8_t *buffer, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t fc, uint16_t bc)
{
	return lcd_DrawPartBitmap(MBNAP buffer, 0, 0, x, y, width, height, fc, bc);
}

static unsigned char* lcd_Char2Bitmap(unsigned char c, font_t font)
{
	return font + 4 + (int)( ((c - font[2])*font[0]*font[1])/8 );
}

extern int lcd_DrawChar(MBNA char c, uint16_t x, uint16_t y, size_t cutoff_y, font_t font, uint16_t fc, uint16_t bc)
{
	unsigned char* bitmap = lcd_Char2Bitmap(c, font);

	size_t effectiveWidth = MIN(font[0], LCD_RES_H-x);
	size_t effectiveHeight = MIN(font[1], cutoff_y-y);

	int c1 = lcd_DrawPartBitmap(MBNAP bitmap, 0, font[0]-effectiveWidth,
									 x, y, effectiveWidth, effectiveHeight, fc, bc);
	if(c1) { return 1; }

	// Simplest case, entire glyph already drawn.
	if(effectiveHeight==font[1] && effectiveWidth==font[0]) { return 0; }

	// The glyph cuts the right edge of the screen.
	if(effectiveWidth<font[0] && effectiveHeight==font[1])
	{
		int c2 = lcd_DrawPartBitmap(MBNAP bitmap, effectiveWidth, effectiveWidth,
										 0, (y+font[1])%cutoff_y, font[0]-effectiveWidth, effectiveHeight, fc, bc);
		if(c2) { return 1; }

		/* I do NOT wanna talk about this case nor the headaches it gave me.
		 * 	This is the scenario in which we cut the right screen edge, but once we loop
		 * the glyph around, we also cut the bottom and so we have to print back at the top.
		 * I hate this.
		 */
		size_t temp_effective_height = cutoff_y - ((y+font[1])%cutoff_y);
		if(temp_effective_height < font[1])
		{
			int c6 = lcd_DrawPartBitmap(MBNAP bitmap, temp_effective_height*font[0] + effectiveWidth, effectiveWidth,
											 0, 0, font[0]-effectiveWidth, font[1]-temp_effective_height, fc, bc);
			if(c6) { return 1; }

		}
	// The case we cut the bottom of the screen.
	} else if(effectiveHeight<font[1] && effectiveWidth==font[0])
	{
		int c3 = lcd_DrawPartBitmap(MBNAP bitmap, effectiveWidth*effectiveHeight, 0,
										 x, 0, effectiveWidth, font[1]-effectiveHeight, fc, bc);
		if(c3) { return 1; }
	// The case we cut the bottom right corner of the screen - yes that is different from the case I complained about above.
	} else
	{
		int c4 = lcd_DrawPartBitmap(MBNAP bitmap, effectiveWidth, effectiveWidth,
										 0, font[1]-effectiveHeight, font[0]-effectiveWidth, font[1], fc, bc);

		int c5 = lcd_DrawPartBitmap(MBNAP bitmap, font[0]*effectiveHeight, font[0]-effectiveWidth,
										 x, 0, effectiveWidth, font[1]-effectiveHeight, fc, bc);
		if(c4||c5) { return 1; }
	}

	return 0;
}

static void lcd_handle_xy_char(uint16_t *x, uint16_t *y, uint16_t limitX, uint16_t limitY, font_t font)
{
	uint16_t nx = (*x)+font[0];
	uint16_t ny = (*y)+font[1];
	*x = nx%limitX;
	if( nx/limitX > 0) { *y = ny%limitY; }
}
static void lcd_handle_xy_char_reverse(uint16_t *x, uint16_t *y, uint16_t limitX, uint16_t limitY, font_t font)
{
	int16_t ox = (*x)-font[0];
	int16_t oy = (*y)-font[1];

#define IF_NEGATIVE_ADD(a, x) ((a<0) ? (a)+(x) : (a))
#define INA(a, x) IF_NEGATIVE_ADD(a, x)

	*x = INA(ox, limitX)%limitX;
	if(ox < 0) { *y = INA(oy, limitY)%limitY; }
}
extern size_t lcd_DrawString(MBNA unsigned char *str, size_t length, uint16_t x, uint16_t y, size_t yCutoff, font_t font, uint8_t tabsize, uint16_t fc, uint16_t bc, bool *invert)
{
	size_t printedChars = 0;
	uint16_t xCurrent = x, yCurrent = y;
	uint16_t currentFC = (*invert) ? bc : fc;
	uint16_t currentBC = (*invert) ? fc : bc;
	for(size_t i = 0; i < length; i++)
	{
		unsigned char c = str[i];
		if(c >= 32 && c <= 126 )
		{
			 // On error, preserve the letter and try printing in the next available space.
			if(lcd_DrawChar(MBNAP c, xCurrent, yCurrent, yCutoff, font, currentFC, currentBC))
			{ i--; }
			printedChars++;
			lcd_handle_xy_char(&xCurrent, &yCurrent, LCD_RES_H, yCutoff, font);
			continue;
		}
		switch(c)
		{
			case '\0': return printedChars; break;
			case '\n':
				xCurrent = LCD_RES_H;
				lcd_handle_xy_char(&xCurrent, &yCurrent, LCD_RES_H, yCutoff, font);
				break;
			case '\r':
				xCurrent = 0;
				break;
			case '\t':
				if(x+(tabsize*font[0])>=LCD_RES_H ) { break; }
				while(x%tabsize)
				{
					lcd_DrawChar(MBNAP ' ', xCurrent, yCurrent, yCutoff, font, currentFC, currentBC);
					printedChars++;
					lcd_handle_xy_char(&xCurrent, &yCurrent, LCD_RES_H, yCutoff, font);
				}
				break;
			case '\b':
				lcd_handle_xy_char_reverse(&xCurrent, &yCurrent, LCD_RES_H, yCutoff, font);
				printedChars--;
				break;
			case '\017':
				currentFC = bc;
				currentBC = fc;
				*invert = true;
				break;
			case '\016':
				currentFC = fc;
				currentBC = bc;
				*invert = false;
				break;
			default: continue; break;
		}
	}
	return printedChars;
}

extern void lcd_Clear(MBNA uint16_t colour)
{
	int err = lcd_DrawRect(MBNAP 0, 0, LCD_RES_H, LCD_MEM_HEIGHT, colour);
	DEBUG_PRINT_ERR("Error for clear: %d\n", err);
}
extern int lcd_Init(void)
{
	int ret = lcd_controllerInit();
	// lcd_clear(0x0000);
	return ret;
}

extern uint16_t* lcd_BufferCreate(void)
{
#ifdef BUFFY_USE_LOCAL_MEMORY_BUFFER
	uint16_t* buf = (uint16_t*)calloc(LCD_RES_H*LCD_MEM_HEIGHT, sizeof(uint16_t));
	DEBUG_ASSERT(buf);
	lcd_Refresh(buf);
	return buf;
#else
	#warning Do not create a buffer when the local memory remains unused by the buffy library.
	return NULL;
#endif // BUFFY_USE_LOCAL_MEMORY_BUFFER
}


extern void lcd_Refresh(uint16_t *lcd_buffer)
{
#ifdef BUFFY_USE_LOCAL_MEMORY_BUFFER
	define_spi_region(0, 0, LCD_RES_H-1, LCD_MEM_HEIGHT-1);
	spi_write16_colour_buf(lcd_buffer, LCD_RES_H*LCD_MEM_HEIGHT);
#else
#warning Do not call lcd_refresh when not using local memory.
#endif // BUFFY_USE_LOCAL_MEMORY_BUFFER
}
