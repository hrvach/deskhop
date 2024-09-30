pico_add_library(pico_async_context_base NOFLAG)
target_include_directories(pico_async_context_base_headers INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_sources(pico_async_context_base INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/async_context_base.c
        )
pico_mirrored_target_link_libraries(pico_async_context_base INTERFACE pico_platform)

pico_add_library(pico_async_context_poll)
target_sources(pico_async_context_poll INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/async_context_poll.c
)
pico_mirrored_target_link_libraries(pico_async_context_poll INTERFACE pico_async_context_base)

pico_add_library(pico_async_context_threadsafe_background)
target_sources(pico_async_context_threadsafe_background INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/async_context_threadsafe_background.c
        )
pico_mirrored_target_link_libraries(pico_async_context_threadsafe_background INTERFACE pico_async_context_base)

pico_add_library(pico_async_context_freertos)
target_sources(pico_async_context_freertos INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/async_context_freertos.c
        )
pico_mirrored_target_link_libraries(pico_async_context_freertos INTERFACE pico_async_context_base)
