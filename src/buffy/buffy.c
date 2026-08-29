#include "buffy.h"
#include <hardware/gpio.h>
#include <pico/stdlib.h>
#include <hardware/sync.h>
#include <hardware/timer.h>
#include <stdint.h>

static void raise_cs(void) { gpio_put(LCD_CS, 1); }
static void lower_cs(void) { gpio_put(LCD_CS, 0); }
static void raise_dc(void) { gpio_put(LCD_DC, 1); }
static void lower_dc(void) { gpio_put(LCD_DC, 0); }

static void spi_write_data(uint8_t data)
{
	lower_cs();
	raise_dc();
	spi_write_blocking(LCD_SPI_MOD, &data, 1);
	raise_cs();
}
static void spi_write_command(uint8_t cmd)
{
	lower_cs();
	lower_dc();
	spi_write_blocking(LCD_SPI_MOD, &cmd, 1);
	raise_cs();
}
static void spi_write_full_command(uint8_t cmd, int argc, ...)
{
	va_list args;
	va_start(args, argc);
	lower_cs();
	lower_dc();
	spi_write_blocking(LCD_SPI_MOD, &cmd, 1);
	raise_dc();
	for(int i = 0; i < argc; i++) { spi_write_blocking(LCD_SPI_MOD, va_arg(args, int), 1); }
	raise_cs();
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

static void define_spi_region(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey);

static void lcd_scroll(int pixels);

static void lcd_controller_init(void)
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

	spi_init(LCD_SPI_MOD, LCD_SPI_FREQ);
	gpio_set_function(LCD_SCK, GPIO_FUNC_SPI);
	gpio_set_function(LCD_MOSI_TX, GPIO_FUNC_SPI);
	gpio_set_function(LCD_MISO_RX, GPIO_FUNC_SPI);
	gpio_set_function(LCD_SCK, GPIO_FUNC_SPI);

	raise_cs();
	gpio_put(LCD_RST, 1);

	uint32_t irq = save_and_disable_interrupts();

	spi_write_command(LCD_CMD_SWRESET);
	busy_wait_us(10000);

	spi_write_full_command(LCD_CMD_COLMOD, 1, 0b01010101);
	spi_write_full_command(LCD_CMD_MADCTL, 1, 0b01001000);
	spi_write_command(LCD_CMD_INVON);
	spi_write_full_command(LCD_CMD_EMS, 1, 0b11000110);
	spi_write_full_command(LCD_CMD_VSCRDEF, 6, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00);
	spi_write_command(LCD_CMD_SLPOUT);

	restore_interrupts(irq);
	busy_wait_us(10000);

	lcd_display_on();

	// TODO: Clear display
}

extern int lcd_draw_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t colour);
extern int lcd_draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t colour);


extern int lcd_init(void);
