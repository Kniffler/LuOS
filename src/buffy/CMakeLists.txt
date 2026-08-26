cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)

project(buffy
	DESCRIPTION "LCD driver for picocalc."
)

add_library(buffy INTERFACE)

target_sources(buffy INTERFACE
	buffy.c
)

target_link_libraries(buffy INTERFACE  pico_stdlib hardware_spi)

target_include_directories(buffy INTERFACE ${CMAKE_CURRENT_LIST_DIR})
