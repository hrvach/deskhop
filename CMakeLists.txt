cmake_minimum_required(VERSION 3.6)

set(VERSION_MAJOR 00)
set(VERSION_MINOR 147)

set(PICO_SDK_FETCH_FROM_GIT off)
set(PICO_BOARD=pico)
set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/pico-sdk)

include(pico_sdk_import.cmake)
set(CMAKE_C_FLAGS "-Ofast -Wall -mcpu=cortex-m0plus -mtune=cortex-m0plus -fstack-usage")

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
        ${CMAKE_CURRENT_LIST_DIR}/src/constants.c
        ${CMAKE_CURRENT_LIST_DIR}/src/protocol.c
        ${CMAKE_CURRENT_LIST_DIR}/src/hid_parser.c
        ${CMAKE_CURRENT_LIST_DIR}/src/hid_report.c
        ${CMAKE_CURRENT_LIST_DIR}/src/utils.c
        ${CMAKE_CURRENT_LIST_DIR}/src/handlers.c
        ${CMAKE_CURRENT_LIST_DIR}/src/setup.c
        ${CMAKE_CURRENT_LIST_DIR}/src/keyboard.c
        ${CMAKE_CURRENT_LIST_DIR}/src/mouse.c
        ${CMAKE_CURRENT_LIST_DIR}/src/tasks.c
        ${CMAKE_CURRENT_LIST_DIR}/src/led.c
        ${CMAKE_CURRENT_LIST_DIR}/src/uart.c
        ${CMAKE_CURRENT_LIST_DIR}/src/usb.c
        ${CMAKE_CURRENT_LIST_DIR}/src/main.c
        ${CMAKE_CURRENT_LIST_DIR}/src/ramdisk.c
        ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
        ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)

set(COMMON_INCLUDES
        ${CMAKE_CURRENT_LIST_DIR}/src/include
        ${PICO_PIO_USB_DIR}/src
)

set(COMMON_LINK_LIBRARIES
  pico_stdlib
  hardware_flash
  hardware_uart
  hardware_gpio
  hardware_pio
  hardware_dma

  tinyusb_device 
  tinyusb_host
  pico_multicore
  Pico-PIO-USB
)

set(binary deskhop)

set(DISK_ASM "${CMAKE_CURRENT_LIST_DIR}/disk/disk.S")
set(DISK_BIN "${CMAKE_CURRENT_LIST_DIR}/disk/disk.img")

set_property(SOURCE ${DISK_ASM} APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")

add_executable(${binary} ${DISK_ASM})

target_sources(${binary} PUBLIC ${COMMON_SOURCES})
target_compile_definitions(${binary} 
    PRIVATE 
    PIO_USB_USE_TINYUSB=1 
    PIO_USB_DP_PIN_DEFAULT=14
    # Uncomment to enable debug uart:
    # DH_DEBUG=1
    __disk_file_path__="${DISK_BIN}"
)
  
target_include_directories(${binary} PUBLIC ${COMMON_INCLUDES})
target_link_libraries(${binary} PUBLIC ${COMMON_LINK_LIBRARIES})

pico_enable_stdio_usb(${binary} 0)
pico_enable_stdio_uart(${binary} 0)

pico_add_extra_outputs(${binary})

add_custom_command(
    TARGET ${binary} POST_BUILD
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/crc32.py ${binary}.bin ${binary}.crc ${VERSION_MAJOR}${VERSION_MINOR}
    COMMAND ${CMAKE_OBJCOPY} --update-section .section_metadata=${binary}.crc ${binary}.elf
    COMMAND ${CMAKE_OBJCOPY} -O binary ${binary}.elf ${binary}.bin
    COMMAND ${CMAKE_BINARY_DIR}/elf2uf2/elf2uf2 ${binary}.elf ${binary}.uf2
    COMMENT "Update CRC32 section to match the actual binary"  
)

target_link_options(${binary} PRIVATE
  -Xlinker
  --print-memory-usage
)

pico_set_linker_script(${binary} ${CMAKE_SOURCE_DIR}/memory_map.ld)
