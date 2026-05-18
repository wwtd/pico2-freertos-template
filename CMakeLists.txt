cmake_minimum_required(VERSION 3.13)

# !!! 关键：指定板子是 Pico 2 !!!
set(PICO_BOARD pico2)

include(pico_sdk_import.cmake)

project(blink C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(blink
    blink.c
)

# Pico 2 使用 SDK 的标准库
target_link_libraries(blink
    pico_stdlib
)

# 启用 USB / UART（你现在 USB 有问题，但先开着）
pico_enable_stdio_usb(blink 1)
pico_enable_stdio_uart(blink 0)

# 生成 UF2 / ELF / BIN
pico_add_extra_outputs(blink)
