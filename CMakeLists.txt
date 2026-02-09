cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(usb_character C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# --- character target ---
add_executable(usb_character
    main.c
    usb_descriptors.c
    msc_disk.c
)

target_include_directories(usb_character PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(usb_character
    pico_stdlib
    tinyusb_device
    tinyusb_board
    hardware_i2c
)

pico_add_extra_outputs(usb_character)

# --- blink target ---
add_executable(blink blink.c)
target_link_libraries(blink pico_stdlib)
pico_enable_stdio_usb(blink 1)
pico_enable_stdio_uart(blink 0)
pico_add_extra_outputs(blink)