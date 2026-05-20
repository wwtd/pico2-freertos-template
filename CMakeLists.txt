cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2)

include(pico_sdk_import.cmake)

project(pico2-freertos-demo C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel)
set(FREERTOS_CONFIG_FILE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/config)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2350_ARM_NTZ/FreeRTOS_Kernel_import.cmake)

add_executable(pico2_freertos_demo
    main.c
    inspector.c
)

target_link_libraries(pico2_freertos_demo PRIVATE
    pico_stdlib
    FreeRTOS-Kernel-Heap4
)

pico_enable_stdio_usb(pico2_freertos_demo 1)
pico_enable_stdio_uart(pico2_freertos_demo 1)

pico_add_extra_outputs(pico2_freertos_demo)
