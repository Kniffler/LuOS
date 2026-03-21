# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)

project(splitter
	DESCRIPTION "Split navigation interface for various applications."
)

add_library(splitter INTERFACE)

target_sources(splitter INTERFACE
	splitter.c
)

target_link_libraries(splitter INTERFACE 
	pico_stdlib
	i2ckbd
	lcdspi
	pico_multicore
)

target_include_directories(splitter INTERFACE ${CMAKE_CURRENT_LIST_DIR})
