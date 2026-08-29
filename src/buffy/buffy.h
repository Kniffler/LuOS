#ifndef __LUOS_BUFFY__
#define __LUOS_BUFFY__

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <hardware/spi.h>
#include <hardware/gpio.h>
// #include <hardware/p>

// Different display commands
#define LCD_CMD_NOP			(0x00)	// no operation
#define LCD_CMD_SWRESET		(0x01)	// software reset
#define LCD_CMD_SLPIN		(0x10)	// sleep in
#define LCD_CMD_SLPOUT		(0x11)	// sleep out
#define LCD_CMD_INVOFF		(0x20)	// display inversion off
#define LCD_CMD_INVON		(0x21)	// display inversion on
#define LCD_CMD_DISPOFF		(0x28)	// display off
#define LCD_CMD_DISPON		(0x29)	// display on
#define LCD_CMD_CASET		(0x2A)	// column address set
#define LCD_CMD_RASET		(0x2B)	// row address set
#define LCD_CMD_RAMWR		(0x2C)	// memory write
#define LCD_CMD_RAMRD		(0x2E)	// memory read
#define LCD_CMD_VSCRDEF		(0x33)	// vertical scroll definition
#define LCD_CMD_MADCTL		(0x36)	// memory access control
#define LCD_CMD_VSCSAD		(0x37)	// vertical scroll start address of RAM
#define LCD_CMD_COLMOD		(0x3A)	// pixel format set
#define LCD_CMD_IFMODE		(0xB0)	// interface mode control
#define LCD_CMD_FRMCTR1		(0xB1)	// frame rate control (in normal mode)
#define LCD_CMD_FRMCTR2		(0xB2)	// frame rate control (in idle mode)
#define LCD_CMD_FRMCTR3		(0xB3)	// frame rate control (in partial mode)
#define LCD_CMD_DIC			(0xB4)	// display inversion control
#define LCD_CMD_DFC			(0xB6)	// display function control
#define LCD_CMD_EMS			(0xB7)	// entry mode set
#define LCD_CMD_MODESEL		(0xB9)	// mode set
#define LCD_CMD_PWR1		(0xC0)	// power control 1
#define LCD_CMD_PWR2		(0xC1)	// power control 2
#define LCD_CMD_PWR3		(0xC2)	// power control 3
#define LCD_CMD_VCMPCTL		(0xC5)	// VCOM control
#define LCD_CMD_PGC			(0xE0)	// positive gamma control
#define LCD_CMD_NGC			(0xE1)	// negative gamma control
#define LCD_CMD_DGC1		(0xE2)	// driver gamma control 1
#define LCD_CMD_DGC2		(0xE3)	// driver gamma control
#define LCD_CMD_DOCA		(0xE8)	// driver output control
#define LCD_CMD_E9			(0xE9)	// Manufacturer command
#define LCD_CMD_F0			(0xF0)	// Manufacturer command
#define LCD_CMD_F7			(0xF7)	// Manufacturer command

// Pin definitions
#define LCD_SCK		10
#define LCD_MOSI_TX	11
#define LCD_MISO_RX	12
#define LCD_CS		13
#define LCD_DC		14
#define LCD_RST		15

#define LCD_SPI_MOD spi1
#define LCD_SPI_FREQ	20000000 // 20Mhz

static void raise_cs(void);
static void lower_cs(void);
static void raise_dc(void);
static void lower_dc(void);

static void spi_write_data(uint8_t data);
static void spi_write_command(uint8_t cmd);
static void spi_write_full_command(uint8_t cmd, int argc, ...);

extern void lcd_display_on(void);
extern void lcd_display_off(void);

static void define_spi_region(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey);

static void lcd_scroll(int pixels);

static void lcd_controller_init(void);

extern int lcd_draw_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t colour);
extern int lcd_draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t colour);


extern int lcd_init(void);

#endif // __LUOS_BUFFY__
