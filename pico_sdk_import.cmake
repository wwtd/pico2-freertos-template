# Uses the official Pico SDK import mechanism.
# Set PICO_SDK_PATH as cmake -D or environment variable.

if (NOT PICO_SDK_PATH)
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()

if (NOT PICO_SDK_PATH)
    foreach(POSSIBLE_SUFFIX pico-sdk pico-sdk-master)
        if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/${POSSIBLE_SUFFIX}/external/pico_sdk_import.cmake")
            set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/${POSSIBLE_SUFFIX}")
            break()
        endif()
    endforeach()
endif()

if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR "Set PICO_SDK_PATH or place pico-sdk as a sibling directory")
endif()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
