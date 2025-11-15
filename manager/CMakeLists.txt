cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)

project(manager
	VERSION 0.0.1
	DESCRIPTION "Manager for LUOS"
)

add_library(manager INTERFACE)

target_sources(manager INTERFACE manager.c )

target_link_libraries(manager INTERFACE 
	pico_stdlib
	hardware_flash
	
	hardware_timer
	pico_unique_id
	FreeRTOS-Kernel
	FreeRTOS-Kernel-Heap4
	
	pico_multicore
	i2ckbd
	lcdspi
)

target_include_directories(manager INTERFACE ${CMAKE_CURRENT_LIST_DIR})
