cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_FETCH_FROM_GIT off)
set(PICO_BOARD=pico)

include(pico_sdk_import.cmake)
set(CMAKE_CXX_FLAGS "-Ofast -Wall -mcpu=cortex-m0plus -mtune=cortex-m0plus")

set(PICO_COPY_TO_RAM 1)

project(deskhop_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

set(PICO_PIO_USB_DIR ${CMAKE_CURRENT_LIST_DIR}/Pico-PIO-USB)

add_library(Pico-PIO-USB STATIC
    ${PICO_PIO_USB_DIR}/src/pio_usb.c
    ${PICO_PIO_USB_DIR}/src/pio_usb_host.c
    ${PICO_PIO_USB_DIR}/src/usb_crc.c
)
pico_generate_pio_header(Pico-PIO-USB ${PICO_PIO_USB_DIR}/src/usb_tx.pio)
pico_generate_pio_header(Pico-PIO-USB ${PICO_PIO_USB_DIR}/src/usb_rx.pio)

target_link_libraries(Pico-PIO-USB PRIVATE
  pico_stdlib
  pico_multicore
  hardware_pio
  hardware_dma
)
target_include_directories(Pico-PIO-USB PRIVATE ${PICO_PIO_USB_DIR})

set(COMMON_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/usb_descriptors.c
        ${CMAKE_CURRENT_LIST_DIR}/src/hid_parser.c
        ${CMAKE_CURRENT_LIST_DIR}/src/utils.c
        ${CMAKE_CURRENT_LIST_DIR}/src/handlers.c
        ${CMAKE_CURRENT_LIST_DIR}/src/setup.c
        ${CMAKE_CURRENT_LIST_DIR}/src/keyboard.c
        ${CMAKE_CURRENT_LIST_DIR}/src/mouse.c
        ${CMAKE_CURRENT_LIST_DIR}/src/led.c
        ${CMAKE_CURRENT_LIST_DIR}/src/uart.c
        ${CMAKE_CURRENT_LIST_DIR}/src/usb.c
        ${CMAKE_CURRENT_LIST_DIR}/src/main.c
        ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
        ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)

set(COMMON_INCLUDES
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${PICO_PIO_USB_DIR}/src
)

set(COMMON_LINK_LIBRARIES
  pico_stdlib
  hardware_uart
  hardware_gpio
  hardware_pio

  tinyusb_device 
  tinyusb_host
  pico_multicore
  Pico-PIO-USB
)

# Pico A - Keyboard
add_executable(board_A)

target_sources(board_A PUBLIC ${COMMON_SOURCES})
target_compile_definitions(board_A PRIVATE BOARD_ROLE=0 PIO_USB_USE_TINYUSB=1 PIO_USB_DP_PIN_DEFAULT=14)
target_include_directories(board_A PUBLIC ${COMMON_INCLUDES})
target_link_libraries(board_A PUBLIC ${COMMON_LINK_LIBRARIES})

pico_enable_stdio_usb(board_A 0)
pico_add_extra_outputs(board_A)


# Pico B - Mouse
add_executable(board_B)

target_compile_definitions(board_B PRIVATE BOARD_ROLE=1 PIO_USB_USE_TINYUSB=1 PIO_USB_DP_PIN_DEFAULT=14)
target_sources(board_B PUBLIC ${COMMON_SOURCES})
target_include_directories(board_B PUBLIC ${COMMON_INCLUDES})
target_link_libraries(board_B PUBLIC ${COMMON_LINK_LIBRARIES})

pico_enable_stdio_usb(board_B 0)
pico_add_extra_outputs(board_B)

target_link_options(board_A PRIVATE
  -Xlinker
  --print-memory-usage
)

target_link_options(board_B PRIVATE
  -Xlinker
  --print-memory-usage
)
