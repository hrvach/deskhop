pico_add_library(pico_bootsel_via_double_reset)

target_sources(pico_bootsel_via_double_reset INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/pico_bootsel_via_double_reset.c
        )

pico_mirrored_target_link_libraries(pico_bootsel_via_double_reset INTERFACE
        pico_bootrom
        pico_time
        pico_binary_info
        )
