cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)

project(iowiz
	DESCRIPTION "I/O wizard for LuOS"
)

add_library(iowiz INTERFACE)

target_sources(iowiz INTERFACE iowiz.c )

target_link_libraries(iowiz INTERFACE 
	pico_stdlib
	
	hardware_gpio
	hardware_uart
	hardware_timer
	FreeRTOS-Kernel
	FreeRTOS-Kernel-Heap4
)

target_include_directories(iowiz INTERFACE ${CMAKE_CURRENT_LIST_DIR})
