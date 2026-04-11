cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)

project(GSM
	DESCRIPTION "General Settings Manager (GSM) for various applications"
)

add_library(GSM INTERFACE)

target_sources(GSM INTERFACE
	GSM.c
)

target_link_libraries(GSM INTERFACE
	pico_stdlib
	pico_malloc

	# blockdevice_sd
	# filesystem_fat
	# filesystem_vfs
)

target_include_directories(GSM INTERFACE ${CMAKE_CURRENT_LIST_DIR})
