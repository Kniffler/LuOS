cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

project(LUOS C CXX ASM)

pico_sdk_init()

#	FreeRTOS

execute_process(
	#	Copy the config provided by:
	# https://github.com/Wilkua/pi-pico-freertos-starter)
	
	# We do this because my knowledge of submodules is limited and I would
	#	like to avoid going through git hell for this one config to persist.
	#	As such, we copy this config over each time, just to be sure.

	# Alternatively, if you're on windows, comment out this procedure and 
	# copy the file manually
	COMMAND cp include/FreeRTOSConfig.h FreeRTOS/include/
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE include)
target_compile_definitions(freertos_config INTERFACE projCOVERAGE_TEST=0)
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS)

# add_subdirectory(FreeRTOS)  only gives an error, instead we have it handled by:
include(FreeRTOS_Kernel_import.cmake)

#	

include_directories(
	${CMAKE_CURRENT_LIST_DIR}
)

add_subdirectory(pico-vfs)
add_subdirectory(i2ckbd)
add_subdirectory(lcdspi)
# add_subdirectory(manager)

add_executable(LUOS main.c)


target_link_libraries(LUOS
	pico_stdlib
	hardware_flash
	
	hardware_timer
	pico_unique_id
	FreeRTOS-Kernel
	FreeRTOS-Kernel-Heap4
	
	hardware_irq
	hardware_adc
	hardware_pwm
	hardware_i2c
	hardware_spi
	hardware_dma
	hardware_exception
	hardware_pio
	pico_multicore

	blockdevice_sd
	filesystem_fat	
	filesystem_vfs
	
	i2ckbd
	lcdspi
	# manager
)

pico_enable_stdio_usb(LUOS 0)
pico_enable_stdio_uart(LUOS 1)

pico_add_extra_outputs(LUOS)

# pico_enable_filesystem(${CMAKE_PROJECT_NAME})
