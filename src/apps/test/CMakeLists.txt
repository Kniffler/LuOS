cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(TESTA testA)

project(${TESTA}
	DESCRIPTION "A testing app for the LuOS bootloader"
)

add_executable(${TESTA} test.c)

target_link_libraries(${TESTA}
	pico_stdlib
	i2ckbd
	lcdspi
)

pico_enable_stdio_usb(${TESTA} 1)
pico_enable_stdio_uart(${TESTA} 0)

pico_add_extra_outputs(${TESTA})

pico_set_linker_script(${TESTA} ${CMAKE_SOURCE_DIR}/memmap_default_rp2040.ld)
