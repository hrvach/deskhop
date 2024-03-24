# cyw43 shared bus read and write
pico_add_library(cybt_shared_bus NOFLAG)

target_sources(cybt_shared_bus INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/cybt_shared_bus.c
    ${CMAKE_CURRENT_LIST_DIR}/cybt_shared_bus_driver.c
)
target_include_directories(cybt_shared_bus_headers INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

# The BT firmware is supplied as a source file containing a static array with ascii hex data
# Set this to true to use this for testing
set(CYW43_USE_HEX_BTFW 0)
if (CYW43_USE_HEX_BTFW)
    message("Warning: CYW43_USE_HEX_BTFW is true")
    target_sources(cybt_shared_bus INTERFACE
        ${PICO_CYW43_DRIVER_PATH}/firmware/cybt_firmware_43439.c
    )
    target_compile_definitions(cybt_shared_bus INTERFACE
        CYW43_USE_HEX_BTFW=1
    )
endif()