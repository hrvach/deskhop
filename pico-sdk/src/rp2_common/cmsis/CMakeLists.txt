# Deferring this config until we decide how to include other CMSIS libraries... it is likely that we always want to use the stub version of the core
# at least if the vendor/device is RaspberryPi/RP2040...

## PICO_CMAKE_CONFIG: PICO_CMSIS_PATH, directory to locate CMSIS installation, default="included stub CORE only impl", group=build
#if (DEFINED PICO_CMSIS_PATH)
#    set(PICO_CMSIS_PATH "${PICO_CMSIS_PATH}" CACHE PATH "Path to the CMSIS tree to use with Raspberry Pi Pico SDK")
#    message("Using specified PICO_CMSIS_PATH for CMSIS ('${PICO_CMSIS_PATH}')")
#elseif (DEFINED ENV{PICO_CMSIS_PATH})
#    set(PICO_CMSIS_PATH $ENV{PICO_CMSIS_PATH})
#    message("Using PICO_CMSIS_PATH from environment for CMSIS ('${PICO_CMSIS_PATH}')")
#endif()
#
## PICO_CMAKE_CONFIG: PICO_CMSIS_VENDOR, vendor name for CMSIS, default="RaspberryPi", group=build
#if (DEFINED PICO_CMSIS_VENDOR)
#    set(PICO_CMSIS_VENDOR "${PICO_CMSIS_VENDOR}" CACHE STRING "CMSIS vendor name to use")
#    message("Using specified PICO_CMSIS_VENDOR for CMSIS ('${PICO_CMSIS_VENDOR}')")
#elseif (DEFINED ENV{PICO_CMSIS_VENDOR})
#    set(PICO_CMSIS_VENDOR $ENV{PICO_CMSIS_VENDOR})
#    message("Using PICO_CMSIS_VENDOR from environment for CMSIS ('${PICO_CMSIS_VENDOR}')")
#else()
#    set(PICO_CMSIS_VENDOR RaspberryPi)
#endif()
#
## PICO_CMAKE_CONFIG: PICO_CMSIS_DEVICE, device name for CMSIS, default="RP2040", group=build
#if (DEFINED PICO_CMSIS_DEVICE)
#    set(PICO_CMSIS_DEVICE "${PICO_CMSIS_DEVICE}" CACHE STRING "CMSIS device name to use")
#    message("Using specified PICO_CMSIS_DEVICE for CMSIS ('${PICO_CMSIS_DEVICE}')")
#elseif (DEFINED ENV{PICO_CMSIS_DEVICE})
#    set(PICO_CMSIS_DEVICE $ENV{PICO_CMSIS_DEVICE})
#    message("Using PICO_CMSIS_DEVICE from environment for CMSIS ('${PICO_CMSIS_DEVICE}')")
#else()
#    set(PICO_CMSIS_DEVICE RP2040)
#endif()
#
#if (PICO_CMSIS_PATH AND EXISTS ${PICO_CMSIS_PATH}/CMSIS/Core/${PICO_CMSIS_VENDOR}/${PICO_CMSIS_DEVICE})
#    message("CMSIS Core found for path ${PICO_CMSIS_PATH}, vendor ${PICO_CMSIS_VENDOR}, device ${PICO_CMSIS_DEVICE}")
#    set(PICO_CMSIS_CORE_PATH ${PICO_CMSIS_PATH}/CMSIS/Core)
#elseif (PICO_CMSIS_VENDOR STREQUAL "RaspberryPi" AND PICO_CMSIS_DEVICE STREQUAL "RP2040")
#    set(PICO_CMSIS_CORE_PATH ${CMAKE_CURRENT_LIST_DIR}/stub)
#elseif (PICO_CMSIS_PATH)
#    message(WARNING "CMSIS core not found in ${PICO_CMSIS_PATH} for vendor ${PICO_CMSIS_VENDOR}, device ${PICO_CMSIS_DEVICE}")
#    set(PICO_CMSIS_CORE_PATH)
#else()
#    message(WARNING "Non-standard vendor ${PICO_CMSIS_VENDOR} amd device ${PICO_CMSIS_DEVICE} specified, but PICO_CMSIS_PATH was not set")
#endif()

# ... using these 3 lines instead
set(PICO_CMSIS_CORE_PATH ${CMAKE_CURRENT_LIST_DIR}/stub)
set(PICO_CMSIS_VENDOR RaspberryPi)
set(PICO_CMSIS_DEVICE RP2040)

if (PICO_CMSIS_CORE_PATH)
    pico_add_library(cmsis_core)
    target_sources(cmsis_core INTERFACE
            ${PICO_CMSIS_CORE_PATH}/CMSIS/Device/${PICO_CMSIS_VENDOR}/${PICO_CMSIS_DEVICE}/Source/system_${PICO_CMSIS_DEVICE}.c
    )
    target_include_directories(cmsis_core_headers INTERFACE
            ${PICO_CMSIS_CORE_PATH}/CMSIS/Core/Include
            ${PICO_CMSIS_CORE_PATH}/CMSIS/Device/${PICO_CMSIS_VENDOR}/${PICO_CMSIS_DEVICE}/Include
    )
    pico_mirrored_target_link_libraries(cmsis_core INTERFACE hardware_clocks pico_platform)

    list(APPEND PICO_RP2040_CONFIG_HEADER_FILES ${CMAKE_CURRENT_LIST_DIR}/include/cmsis/rename_exceptions.h)
    pico_promote_common_scope_vars()
endif()

