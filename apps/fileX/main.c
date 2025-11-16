
#include "blockdevice/sd.h"
#include "filesystem/fat.h"
#include "filesystem/vfs.h"

//	Interface pins for the SD card
#define SD_SPI0         0
#define SD_SCLK_PIN     18
#define SD_MOSI_PIN     19
#define SD_MISO_PIN     16
#define SD_CS_PIN       17
#define SD_DET_PIN			22

uint8_t sd_status;	// 0 no sdcard, 1 has sd card
bool sd_card_inserted(void)
{
	sd_status = !gpio_get(SD_DET_PIN);
	return (bool)sd_status;
}

bool fs_init(void)
{
	if(!sd_card_inserted()) { return false; }
	
	// DEBUG_PRINT("fs init SD\n");
	blockdevice_t *sd = blockdevice_sd_create(spi0,
		SD_MOSI_PIN,
		SD_MISO_PIN,
		SD_SCLK_PIN,
		SD_CS_PIN,
		125000000 / 2 / 4,	// 15.6MHz
		true);
	
	filesystem_t *fat = filesystem_fat_create();
	int err = fs_mount("/", fat, sd);
	
	if (err == -1)
	{
		// DEBUG_PRINT("format /\n");
		err = fs_format(fat, sd);
		if (err == -1)
		{
			// DEBUG_PRINT("format err: %s\n", strerror(errno));
			return false;
		}
		err = fs_mount("/", fat, sd);
		if (err == -1)
		{
			// DEBUG_PRINT("mount err: %s\n", strerror(errno));
			return false;
		}
	}
	return true;
}
