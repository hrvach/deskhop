pico_add_library(pico_unique_id)

target_sources(pico_unique_id INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/unique_id.c
)

target_include_directories(pico_unique_id_headers INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

pico_mirrored_target_link_libraries(pico_unique_id INTERFACE hardware_flash)
