if (PICO_CYW43_SUPPORTED) # set by BOARD=pico-w
    if (TARGET cyw43_driver_picow)
        pico_add_library(pico_cyw43_arch)
        target_sources(pico_cyw43_arch INTERFACE
                ${CMAKE_CURRENT_LIST_DIR}/cyw43_arch.c
                ${CMAKE_CURRENT_LIST_DIR}/cyw43_arch_poll.c
                ${CMAKE_CURRENT_LIST_DIR}/cyw43_arch_threadsafe_background.c
                ${CMAKE_CURRENT_LIST_DIR}/cyw43_arch_freertos.c
                )

        target_include_directories(pico_cyw43_arch_headers INTERFACE
                ${CMAKE_CURRENT_LIST_DIR}/include)

        pico_mirrored_target_link_libraries(pico_cyw43_arch INTERFACE
                pico_unique_id
                cyw43_driver_picow # driver for pico w
                pico_cyw43_driver # integration with async_context
                )

        if (NOT TARGET pico_lwip)
            message(WARNING "lwIP is not available; Full Pico W wireless support will be unavailable")
        else()
            message("Pico W Wi-Fi build support available.")
            pico_add_library(pico_cyw43_arch_poll NOFLAG)
            target_compile_definitions(pico_cyw43_arch_poll_headers INTERFACE
                    PICO_CYW43_ARCH_POLL=1
                    )
            pico_mirrored_target_link_libraries(pico_cyw43_arch_poll INTERFACE
                    pico_cyw43_arch
                    pico_async_context_poll)

            pico_add_library(pico_cyw43_arch_lwip_poll NOFLAG)
            pico_mirrored_target_link_libraries(pico_cyw43_arch_lwip_poll INTERFACE
                    pico_lwip_nosys
                    pico_cyw43_arch_poll)
            target_compile_definitions(pico_cyw43_arch_lwip_poll_headers INTERFACE
                    CYW43_LWIP=1
                    )

            pico_add_library(pico_cyw43_arch_threadsafe_background NOFLAG)
            pico_mirrored_target_link_libraries(pico_cyw43_arch_threadsafe_background INTERFACE
                    pico_cyw43_arch
                    pico_async_context_threadsafe_background)
            target_compile_definitions(pico_cyw43_arch_threadsafe_background_headers INTERFACE
                    PICO_CYW43_ARCH_THREADSAFE_BACKGROUND=1
                    )

            pico_add_library(pico_cyw43_arch_lwip_threadsafe_background NOFLAG)
            pico_mirrored_target_link_libraries(pico_cyw43_arch_lwip_threadsafe_background INTERFACE
                    pico_lwip_nosys
                    pico_cyw43_arch_threadsafe_background)
            target_compile_definitions(pico_cyw43_arch_lwip_threadsafe_background_headers INTERFACE
                    CYW43_LWIP=1
                    )

            pico_add_library(pico_cyw43_arch_sys_freertos NOFLAG)
            pico_mirrored_target_link_libraries(pico_cyw43_arch_sys_freertos INTERFACE
                    pico_cyw43_arch
                    pico_async_context_freertos)
            target_compile_definitions(pico_cyw43_arch_sys_freertos_headers INTERFACE
                    PICO_CYW43_ARCH_FREERTOS=1
                    )

            pico_add_library(pico_cyw43_arch_lwip_sys_freertos NOFLAG)
            pico_mirrored_target_link_libraries(pico_cyw43_arch_lwip_sys_freertos INTERFACE
                    pico_lwip_freertos
                    pico_cyw43_arch_sys_freertos)
            target_compile_definitions(pico_cyw43_arch_lwip_sys_freertos_headers INTERFACE
                    CYW43_LWIP=1
                    LWIP_PROVIDE_ERRNO=1
                    # now the default
                    #PICO_LWIP_CUSTOM_LOCK_TCPIP_CORE=1 # we want to override the lwip locking mechanism to use our mutex
                    )
        endif()

        pico_add_library(pico_cyw43_arch_none NOFLAG)
        pico_mirrored_target_link_libraries(pico_cyw43_arch_none INTERFACE
                pico_cyw43_arch
                pico_async_context_threadsafe_background)
        target_compile_definitions(pico_cyw43_arch_none_headers INTERFACE
                CYW43_LWIP=0
                PICO_CYW43_ARCH_THREADSAFE_BACKGROUND=1 # none still uses threadsafe_background to make gpio use easy
                )
    endif()
endif()

if (PICO_CYW43_DRIVER_PATH AND EXISTS "${PICO_CYW43_DRIVER_PATH}")
    pico_add_doxygen(${PICO_CYW43_DRIVER_PATH}/src)
endif()