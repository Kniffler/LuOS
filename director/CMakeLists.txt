cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)

project(director
	DESCRIPTION "Manager for directives in LuOS"
)

add_library(director INTERFACE)

target_sources(director INTERFACE
	director.c
)

target_link_libraries(director INTERFACE
	pico_stdlib
	lcdspi
)

target_include_directories(director INTERFACE ${CMAKE_CURRENT_LIST_DIR})
