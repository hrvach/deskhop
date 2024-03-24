if (NOT TARGET pico_i2c_slave)
    pico_add_library(pico_i2c_slave)

    target_sources(pico_i2c_slave INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/i2c_slave.c)

    target_include_directories(pico_i2c_slave_headers INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    pico_mirrored_target_link_libraries(pico_i2c_slave INTERFACE hardware_i2c hardware_irq)
endif()
