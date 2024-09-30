pico_add_library(pico_flash)

target_sources(pico_flash INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/flash.c
)

target_include_directories(pico_flash_headers INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

# just include multicore headers, as we don't want to pull in the lib if it isn't pulled in already
target_link_libraries(pico_flash INTERFACE pico_multicore_headers)

pico_mirrored_target_link_libraries(pico_flash INTERFACE pico_time hardware_sync)
