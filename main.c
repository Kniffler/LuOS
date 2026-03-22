#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "hardware/gpio.h"
#include "lcdspi.h"
#include "splitter.h"
#include <hardware/flash.h>
#include <hardware/watchdog.h>
#include "config.h"

#include <pico/platform/common.h>
#include <pico/stdlib.h>
#include <hardware/sync.h>

#include "blockdevice/sd.h"
#include "filesystem/fat.h"
#include "filesystem/vfs.h"
/*
 * #include "text_directory_ui.h"
 * #include "key_event.h"
 */
// Vector and RAM offset

#if PICO_RP2040
#define VTOR_OFFSET M0PLUS_VTOR_OFFSET
#define MAX_RAM 0x20040000

#elif PICO_RP2350
#define VTOR_OFFSET M33_VTOR_OFFSET
#define MAX_RAM 0x20080000
#endif

bool sd_card_inserted(void)
{
	return !gpio_get(SD_DET_PIN);
}

bool fs_init(void)
{
	blockdevice_t *sd = blockdevice_sd_create(spi0,
											  SD_MOSI_PIN,
										   SD_MISO_PIN,
										   SD_SCLK_PIN,
										   SD_CS_PIN,
										   125000000 / 2 / 4, // 15.6MHz
										   true);

	filesystem_t *fat = filesystem_fat_create();

	int err = fs_mount("/sd", fat, sd);
	if (err != -1) { return true; }

	err = fs_format(fat, sd);
	if (err == -1) { return false; }

	err = fs_mount("/sd", fat, sd);
	if (err == -1) { return false; }
	return true;
}

static bool __not_in_flash_func(is_same_as_existing_program)(FILE *fp)
{
	uint8_t buffer[FLASH_SECTOR_SIZE] = {0};
	size_t program_size = 0;
	size_t len = 0;
	while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0)
	{
		uint8_t *flash = (uint8_t *)(XIP_BASE + SD_BOOT_FLASH_OFFSET + program_size);
		if (memcmp(buffer, flash, len) != 0)
		{ return false; }

		program_size += len;
	}
	return true;
}

// This function must run from RAM since it erases and programs flash memory
static bool __not_in_flash_func(load_program)(const char *filename)
{
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) { return false; }

	if (is_same_as_existing_program(fp))
	{
		return true; // save ourselves this workload
	}

	// Check file size to ensure it doesn't exceed the available flash space
	if (fseek(fp, 0, SEEK_END) == -1) { fclose(fp); return false; }

	long file_size = ftell(fp);

	if (file_size <= 0) { fclose(fp); return false; }
	if (file_size > MAX_APP_SIZE) { fclose(fp); return false; }
	if (fseek(fp, 0, SEEK_SET) == -1) { fclose(fp); return false; }


	size_t program_size = 0;

	uint8_t first_page[FLASH_SECTOR_SIZE] = {0};
	uint8_t buffer[FLASH_SECTOR_SIZE] = {0};

	size_t len = 0;
	size_t flen = 0;

	// Erase and program flash in FLASH_SECTOR_SIZE chunks
	while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0)
	{
		// Ensure we don't write beyond the application area
		if ((program_size + len) > MAX_APP_SIZE)
		{
			fclose(fp);
			return false;
		}

		uint32_t ints = save_and_disable_interrupts();
		flash_range_erase(SD_BOOT_FLASH_OFFSET + program_size, FLASH_SECTOR_SIZE);

		/* Don't program the first page, and save it.
			This way we prevent launching apps that have not yet been fully loaded into flash.
		 */
		if(program_size != 0)
		{
			flash_range_program(SD_BOOT_FLASH_OFFSET + program_size, buffer, len);
		}else{
			*first_page = *buffer;
			flen = len;
		}

		restore_interrupts(ints);

		program_size += len;
	}

	uint32_t ints = save_and_disable_interrupts();
	flash_range_program(SD_BOOT_FLASH_OFFSET, first_page, flen);
	restore_interrupts(ints);

	fclose(fp);
	return true;
}

// This function jumps to the application entry point
// It must update the vector table and stack pointer before jumping
void __not_in_flash_func(launch_application_from)(uint32_t *app_location)
{
	// https://vanhunteradams.com/Pico/Bootloader/Bootloader.html
	uint32_t *new_vector_table = app_location;
	volatile uint32_t *vtor = (uint32_t *)(PPB_BASE + VTOR_OFFSET);
	*vtor = (uint32_t)new_vector_table;
	asm volatile(
		"msr msp, %0\n"
		"bx %1\n"
		:
		: "r"(new_vector_table[0]), "r"(new_vector_table[1])
		:);
}

// Check if a valid application exists in flash by examining the vector table
static bool is_valid_application(uint32_t *app_location)
{
	// Check that the initial stack pointer is within a plausible RAM region (assumed range for Pico: 0x20000000 to 0x20040000)
	uint32_t stack_pointer = app_location[0];
	if (stack_pointer < 0x20000000 || stack_pointer > MAX_RAM)
	{
		return false;
	}

	// Check that the reset vector is within the valid flash application area
	uint32_t reset_vector = app_location[1];
	if (reset_vector < (0x10000000 + SD_BOOT_FLASH_OFFSET) || reset_vector > (0x10000000 + PICO_FLASH_SIZE_BYTES))
	{
		return false;
	}
	return true;
}

int load_and_launch_firmware_by_path(const char *path)
{
	// Attempt to load the application from the SD card
	// bool load_success = load_program(FIRMWARE_PATH);
	bool load_success = load_program(path);

	// Get the pointer to the application flash area
	uint32_t *app_location = (uint32_t *)(XIP_BASE + SD_BOOT_FLASH_OFFSET);

	// Check if the app in flash is valid
	bool has_valid_app = is_valid_application(app_location);

	// Loading was valid and successfull
	if (load_success || has_valid_app)
	{
		// Small delay to allow printf to complete
		sleep_ms(100);
		launch_application_from(app_location);
	}
	else
	{
		sleep_ms(2000);
		// Trigger a watchdog reboot
		watchdog_reboot(0, 0, 0);
	}

	// We should never reach here
	while (1)
	{
		tight_loop_contents();
	}
}

void final_selection_callback(const char *path)
{
	// Trigger firmware loading with the selected path

	char status_message[128];
	const char *extension = ".bin";
	size_t path_len = strlen(path);
	size_t ext_len = strlen(extension);

	if (path_len < ext_len || strcmp(path + path_len - ext_len, extension) != 0)
	{
		snprintf(status_message, sizeof(status_message), "Err: FILE is not a .bin file");
		return;
	}

	snprintf(status_message, sizeof(status_message), "SEL: %s", path);

	sleep_ms(200);

	load_and_launch_firmware_by_path(path);
}

int main()
{
	stdio_init_all();

	// Initialize SD card detection pin
	gpio_init(SD_DET_PIN);
	gpio_set_dir(SD_DET_PIN, GPIO_IN);
	gpio_pull_up(SD_DET_PIN); // Enable pull-up resistor

	lcd_init();
	lcd_clear();
	int id = lcd_region_create(0, 0, LCD_WIDTH, LCD_HEIGHT);
	uint16_t name_length = splitter_init(id);
	uint8_t option_count = name_length&UINT8_MAX;
	name_length >>= 8;

	for(;;) { tight_loop_contents(); }
}
