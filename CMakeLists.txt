cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

project(LUOS C CXX ASM)

pico_sdk_init()

include_directories(
	${CMAKE_CURRENT_LIST_DIR}
)

# I see no point in having these libraries twice in my file structure
add_subdirectory(AppShell/pico-vfs)
add_subdirectory(AppShell/i2ckbd)
add_subdirectory(AppShell/lcdspi)
add_subdirectory(AppShell/splitter)

add_executable(LUOS main.c)

add_compile_options(-Wall
	-Wno-format     
		# int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
	-Wno-unused-function
		# we have some from the docs that aren't called
	-Wno-maybe-uninitialized
	-Os)

target_link_libraries(LUOS
	pico_stdlib
	pico_bootsel_via_double_reset
	hardware_flash
	hardware_dma
	hardware_uart
	hardware_watchdog
	hardware_pwm

	i2ckbd
	lcdspi
	splitter
	
	blockdevice_sd
	filesystem_fat	
	filesystem_vfs
	
)

pico_enable_stdio_usb(LUOS 0)
pico_enable_stdio_uart(LUOS 1)	

pico_add_extra_outputs(LUOS)

# pico_enable_filesystem(${CMAKE_PROJECT_NAME})

# This function sets the custom linker script for SD card applications
# The memmap_sdcard_app.ld script configures the application to start at
# FLASH origin = 0x10000000 + 256k, which aligns with the SD_BOOT_FLASH_OFFSET
# defined in main.c (256 * 1024). This ensures the bootloader (first 256KB)
# is preserved when flashing the application.
function(enable_sdcard_app target)
  #pico_set_linker_script(${target} ${CMAKE_SOURCE_DIR}/memmap_sdcard_app.ld)
  if(${PICO_PLATFORM} STREQUAL "rp2040")
    pico_set_linker_script(${CMAKE_PROJECT_NAME} ${CMAKE_SOURCE_DIR}/memmap_default_rp2040.ld)
  elseif(${PICO_PLATFORM} MATCHES "rp2350")
    pico_set_linker_script(${CMAKE_PROJECT_NAME} ${CMAKE_SOURCE_DIR}/memmap_default_rp2350.ld)
  endif()
endfunction()
