cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)

project(manager
	DESCRIPTION "Manager for LUOS"
)

add_library(manager INTERFACE)

target_sources(manager INTERFACE manager.c )

add_subdirectory(iowiz)

target_link_libraries(manager INTERFACE 
	pico_stdlib
	
	hardware_timer
	FreeRTOS-Kernel
	FreeRTOS-Kernel-Heap4
	i2ckbd
	lcdspi
	
	iowiz
)

target_include_directories(manager INTERFACE ${CMAKE_CURRENT_LIST_DIR})
