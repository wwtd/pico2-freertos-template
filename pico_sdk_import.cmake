# This file is copied from the Pico SDK, DO NOT EDIT.

if (DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
else()
    set(PICO_SDK_PATH "/home/pi/pico-dir/pico-sdk")
endif()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
