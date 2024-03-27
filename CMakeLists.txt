cmake_minimum_required(VERSION 3.5)

set(PICO_SDK_FETCH_FROM_GIT off)
set(PICO_BOARD=pico)
set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/pico-sdk)

include(pico_sdk_import.cmake)
set(CMAKE_C_FLAGS "-Ofast -Wall -mcpu=cortex-m0plus -mtune=cortex-m0plus -funroll-loops")

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
        ${CMAKE_CURRENT_LIST_DIR}/src/defaults.c
        ${CMAKE_CURRENT_LIST_DIR}/src/hid_parser.c
        ${CMAKE_CURRENT_LIST_DIR}/src/utils.c
        ${CMAKE_CURRENT_LIST_DIR}/src/handlers.c
        ${CMAKE_CURRENT_LIST_DIR}/src/setup.c
        ${CMAKE_CURRENT_LIST_DIR}/src/keyboard.c
        ${CMAKE_CURRENT_LIST_DIR}/src/mouse.c
        ${CMAKE_CURRENT_LIST_DIR}/src/led.c
        ${CMAKE_CURRENT_LIST_DIR}/src/uart.c
        ${CMAKE_CURRENT_LIST_DIR}/src/usb.c
        ${CMAKE_CURRENT_LIST_DIR}/src/hid.c
        ${CMAKE_CURRENT_LIST_DIR}/src/dfu.c
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
  hardware_flash
  hardware_uart
  hardware_gpio
  hardware_pio

  tinyusb_device
  tinyusb_host
  pico_multicore
  Pico-PIO-USB
  pico_unique_id
)

# Pico A - Keyboard (board_role = 0)
#      B - Mouse    (board_role = 1)

set(binaries board_A board_B)

foreach(board_role RANGE 0 1)
  list (GET binaries ${board_role} binary)

  add_executable(${binary})

  target_sources(${binary} PUBLIC ${COMMON_SOURCES})
  target_compile_definitions(${binary} PRIVATE BOARD_ROLE=${board_role} PIO_USB_USE_TINYUSB=1 PIO_USB_DP_PIN_DEFAULT=14)

  target_include_directories(${binary} PUBLIC ${COMMON_INCLUDES})
  target_link_libraries(${binary} PUBLIC ${COMMON_LINK_LIBRARIES})

  pico_enable_stdio_usb(${binary} 0)
  pico_add_extra_outputs(${binary})

  target_link_options(${binary} PRIVATE
    -Xlinker
    --print-memory-usage
  )

  pico_set_linker_script(${binary} ${CMAKE_SOURCE_DIR}/memory_map.ld)

endforeach()
