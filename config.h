#ifndef __LUOS_CONFIG__
#define __LUOS_CONFIG__

// Comment out to turn of debug messages sent over stdout
#define USE_DEBUG 1
// #define WAIT_ON_FIRST_SERIAL_INPUT 1
#define WAIT_ON_FIRST_KBD_INPUT 1


// GPIOs for SPI interface (SD card)
#define SD_SPI0			0
#define SD_SCLK_PIN		18
#define SD_MOSI_PIN		19
#define SD_MISO_PIN		16
#define SD_CS_PIN		17
#define SD_DET_PIN 		22

// LCD settings
#define LCD_SPI1		1
#define LCD_SCK_PIN		10
#define LCD_MOSI_PIN	11
#define LCD_MISO_PIN	12
#define LCD_CS_PIN		13
#define LCD_DC_PIN		14
#define LCD_RST_PIN		15

#define BUFFY_USE_LOCAL_MEMORY_BUFFER 1

#define per_pixel_filter(px, x, y) (px)


// GPIOs for audio output
#define AUDIO_LEFT		28
#define AUDIO_RIGHT		27


#endif // __LUOS_CONFIG__
