if (DEFINED ENV{PICO_BTSTACK_PATH} AND (NOT PICO_BTSTACK_PATH))
    set(PICO_BTSTACK_PATH $ENV{PICO_BTSTACK_PATH})
    message("Using PICO_BTSTACK_PATH from environment ('${PICO_BTSTACK_PATH}')")
endif ()

set(BTSTACK_TEST_PATH "src/bluetooth.h")
if (NOT PICO_BTSTACK_PATH)
    set(PICO_BTSTACK_PATH ${PROJECT_SOURCE_DIR}/lib/btstack)
    if (PICO_CYW43_SUPPORTED AND NOT EXISTS ${PICO_BTSTACK_PATH}/${BTSTACK_TEST_PATH})
        message(WARNING "btstack submodule has not been initialized; Pico W BLE support will be unavailable.
            hint: try 'git submodule update --init' from your SDK directory (${PICO_SDK_PATH}).")
    endif()
elseif (NOT EXISTS ${PICO_BTSTACK_PATH}/${BTSTACK_TEST_PATH})
    message(WARNING "PICO_BTSTACK_PATH specified but content not present.")
endif()

if (EXISTS ${PICO_BTSTACK_PATH}/${BTSTACK_TEST_PATH})
    message("BTstack available at ${PICO_BTSTACK_PATH}")

    pico_register_common_scope_var(PICO_BTSTACK_PATH)

    pico_add_library(pico_btstack_base NOFLAG)
    target_include_directories(pico_btstack_base_headers INTERFACE
        ${PICO_BTSTACK_PATH}/src
        ${PICO_BTSTACK_PATH}/platform/embedded
        )

    target_sources(pico_btstack_base INTERFACE
        ${PICO_BTSTACK_PATH}/3rd-party/micro-ecc/uECC.c
        ${PICO_BTSTACK_PATH}/3rd-party/rijndael/rijndael.c
        ${PICO_BTSTACK_PATH}/3rd-party/segger-rtt/SEGGER_RTT.c
        ${PICO_BTSTACK_PATH}/3rd-party/segger-rtt/SEGGER_RTT_printf.c
        ${PICO_BTSTACK_PATH}/platform/embedded/btstack_tlv_flash_bank.c
        ${PICO_BTSTACK_PATH}/platform/embedded/hci_dump_embedded_stdout.c
        ${PICO_BTSTACK_PATH}/platform/embedded/hci_dump_segger_rtt_stdout.c
        ${PICO_BTSTACK_PATH}/src/ad_parser.c
        ${PICO_BTSTACK_PATH}/src/btstack_audio.c
        ${PICO_BTSTACK_PATH}/src/btstack_base64_decoder.c
        ${PICO_BTSTACK_PATH}/src/btstack_crypto.c
        ${PICO_BTSTACK_PATH}/src/btstack_hid_parser.c
        ${PICO_BTSTACK_PATH}/src/btstack_linked_list.c
        ${PICO_BTSTACK_PATH}/src/btstack_memory.c
        ${PICO_BTSTACK_PATH}/src/btstack_memory_pool.c
        ${PICO_BTSTACK_PATH}/src/btstack_resample.c
        ${PICO_BTSTACK_PATH}/src/btstack_ring_buffer.c
        ${PICO_BTSTACK_PATH}/src/btstack_run_loop.c
        ${PICO_BTSTACK_PATH}/src/btstack_run_loop_base.c
        ${PICO_BTSTACK_PATH}/src/btstack_slip.c
        ${PICO_BTSTACK_PATH}/src/btstack_tlv.c
        ${PICO_BTSTACK_PATH}/src/btstack_tlv_none.c
        ${PICO_BTSTACK_PATH}/src/btstack_util.c
        ${PICO_BTSTACK_PATH}/src/hci.c
        ${PICO_BTSTACK_PATH}/src/hci_cmd.c
        ${PICO_BTSTACK_PATH}/src/hci_dump.c
        ${PICO_BTSTACK_PATH}/src/hci_event.c
        ${PICO_BTSTACK_PATH}/src/l2cap.c
        ${PICO_BTSTACK_PATH}/src/l2cap_signaling.c
        ${PICO_BTSTACK_PATH}/src/mesh/gatt-service/mesh_provisioning_service_server.c
        ${PICO_BTSTACK_PATH}/src/mesh/gatt-service/mesh_proxy_service_server.c
        ${PICO_BTSTACK_PATH}/3rd-party/md5/md5.c
        ${PICO_BTSTACK_PATH}/3rd-party/yxml/yxml.c
        ${CMAKE_CURRENT_LIST_DIR}/btstack_stdin_pico.c
    )
    target_include_directories(pico_btstack_base_headers INTERFACE
        ${PICO_BTSTACK_PATH}/
        ${PICO_BTSTACK_PATH}/3rd-party/md5
        ${PICO_BTSTACK_PATH}/3rd-party/yxml
        ${PICO_BTSTACK_PATH}/3rd-party/rijndael
        ${PICO_BTSTACK_PATH}/3rd-party/micro-ecc
        ${PICO_BTSTACK_PATH}/3rd-party/segger-rtt
    )

    pico_add_library(pico_btstack_ble)
    target_sources(pico_btstack_ble INTERFACE
        ${PICO_BTSTACK_PATH}/src/ble/att_db.c
        ${PICO_BTSTACK_PATH}/src/ble/att_db_util.c
        ${PICO_BTSTACK_PATH}/src/ble/att_dispatch.c
        ${PICO_BTSTACK_PATH}/src/ble/att_server.c
        ${PICO_BTSTACK_PATH}/src/ble/gatt-service/battery_service_server.c
        ${PICO_BTSTACK_PATH}/src/ble/gatt-service/battery_service_client.c
        ${PICO_BTSTACK_PATH}/src/ble/gatt-service/cycling_power_service_server.c
        ${PICO_BTSTACK_PATH}/src/ble/gatt-service/cycling_speed_and_cadence_service_server.c
        ${PICO_BTSTACK_PATH}/src/ble/gatt-service/device_information_service_server.c
        ${PICO_BTSTACK_PATH}/src/ble/gatt-service/device_information_service_client.c
        ${PICO_BTSTACK_PATH}/src/ble/gatt-service/heart_rate_service_server.c
        ${PICO_BTSTACK_PATH}/src/ble/gatt-service/hids_client.c
        ${PICO_BTSTACK_PATH}/src/ble/gatt-service/hids_device.c
        ${PICO_BTSTACK_PATH}/src/ble/gatt-service/nordic_spp_service_server.c
        ${PICO_BTSTACK_PATH}/src/ble/gatt-service/ublox_spp_service_server.c
        ${PICO_BTSTACK_PATH}/src/ble/gatt-service/ancs_client.c
        ${PICO_BTSTACK_PATH}/src/ble/gatt_client.c
        ${PICO_BTSTACK_PATH}/src/ble/le_device_db_memory.c
        ${PICO_BTSTACK_PATH}/src/ble/le_device_db_tlv.c
        ${PICO_BTSTACK_PATH}/src/ble/sm.c
    )
    pico_mirrored_target_link_libraries(pico_btstack_ble INTERFACE
        pico_btstack_base
    )
    target_compile_definitions(pico_btstack_ble_headers INTERFACE
            ENABLE_BLE=1
            )

    pico_add_library(pico_btstack_classic)
    target_sources(pico_btstack_classic INTERFACE
        ${PICO_BTSTACK_PATH}/src/classic/a2dp.c
        ${PICO_BTSTACK_PATH}/src/classic/a2dp_sink.c
        ${PICO_BTSTACK_PATH}/src/classic/a2dp_source.c
        ${PICO_BTSTACK_PATH}/src/classic/avdtp.c
        ${PICO_BTSTACK_PATH}/src/classic/avdtp_acceptor.c
        ${PICO_BTSTACK_PATH}/src/classic/avdtp_initiator.c
        ${PICO_BTSTACK_PATH}/src/classic/avdtp_sink.c
        ${PICO_BTSTACK_PATH}/src/classic/avdtp_source.c
        ${PICO_BTSTACK_PATH}/src/classic/avdtp_util.c
        ${PICO_BTSTACK_PATH}/src/classic/avrcp.c
        ${PICO_BTSTACK_PATH}/src/classic/avrcp_browsing.c
        ${PICO_BTSTACK_PATH}/src/classic/avrcp_browsing_controller.c
        ${PICO_BTSTACK_PATH}/src/classic/avrcp_browsing_target.c
        ${PICO_BTSTACK_PATH}/src/classic/avrcp_controller.c
        ${PICO_BTSTACK_PATH}/src/classic/avrcp_cover_art_client.c
        ${PICO_BTSTACK_PATH}/src/classic/avrcp_media_item_iterator.c
        ${PICO_BTSTACK_PATH}/src/classic/avrcp_target.c
        ${PICO_BTSTACK_PATH}/src/classic/btstack_cvsd_plc.c
        ${PICO_BTSTACK_PATH}/src/classic/btstack_link_key_db_tlv.c
        ${PICO_BTSTACK_PATH}/src/classic/btstack_sbc_plc.c
        ${PICO_BTSTACK_PATH}/src/classic/device_id_server.c
        ${PICO_BTSTACK_PATH}/src/classic/gatt_sdp.c
        ${PICO_BTSTACK_PATH}/src/classic/goep_client.c
        ${PICO_BTSTACK_PATH}/src/classic/goep_server.c
        ${PICO_BTSTACK_PATH}/src/classic/hfp.c
        ${PICO_BTSTACK_PATH}/src/classic/hfp_ag.c
        ${PICO_BTSTACK_PATH}/src/classic/hfp_gsm_model.c
        ${PICO_BTSTACK_PATH}/src/classic/hfp_hf.c
        ${PICO_BTSTACK_PATH}/src/classic/hfp_msbc.c
        ${PICO_BTSTACK_PATH}/src/classic/hid_device.c
        ${PICO_BTSTACK_PATH}/src/classic/hid_host.c
        ${PICO_BTSTACK_PATH}/src/classic/hsp_ag.c
        ${PICO_BTSTACK_PATH}/src/classic/hsp_hs.c
        ${PICO_BTSTACK_PATH}/src/classic/obex_iterator.c
        ${PICO_BTSTACK_PATH}/src/classic/obex_message_builder.c
        ${PICO_BTSTACK_PATH}/src/classic/obex_parser.c
        ${PICO_BTSTACK_PATH}/src/classic/pan.c
        ${PICO_BTSTACK_PATH}/src/classic/pbap_client.c
        ${PICO_BTSTACK_PATH}/src/classic/rfcomm.c
        ${PICO_BTSTACK_PATH}/src/classic/sdp_client.c
        ${PICO_BTSTACK_PATH}/src/classic/sdp_client_rfcomm.c
        ${PICO_BTSTACK_PATH}/src/classic/sdp_server.c
        ${PICO_BTSTACK_PATH}/src/classic/sdp_util.c
        ${PICO_BTSTACK_PATH}/src/classic/spp_server.c
    )
    pico_mirrored_target_link_libraries(pico_btstack_classic INTERFACE
        pico_btstack_base
    )
    target_compile_definitions(pico_btstack_classic_headers INTERFACE
            ENABLE_CLASSIC=1
            )

    pico_add_library(pico_btstack_flash_bank)
    target_sources(pico_btstack_flash_bank INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/btstack_flash_bank.c
    )
    target_include_directories(pico_btstack_flash_bank_headers INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
    pico_mirrored_target_link_libraries(pico_btstack_flash_bank INTERFACE pico_btstack_base pico_flash)

    pico_add_library(pico_btstack_run_loop_async_context NOFLAG)
    target_sources(pico_btstack_run_loop_async_context INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/btstack_run_loop_async_context.c
    )
    target_include_directories(pico_btstack_run_loop_async_context_headers INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
    pico_mirrored_target_link_libraries(pico_btstack_run_loop_async_context INTERFACE pico_btstack_base pico_async_context_base)

    pico_add_library(pico_btstack_sbc_encoder NOFLAG)
    target_sources(pico_btstack_sbc_encoder INTERFACE
            # SBC codec for A2DP and HFP demos
            ${PICO_BTSTACK_PATH}/src/classic/btstack_sbc_encoder_bluedroid.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/encoder/srce/sbc_analysis.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/encoder/srce/sbc_dct.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/encoder/srce/sbc_dct_coeffs.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/encoder/srce/sbc_enc_bit_alloc_mono.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/encoder/srce/sbc_enc_bit_alloc_ste.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/encoder/srce/sbc_enc_coeffs.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/encoder/srce/sbc_encoder.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/encoder/srce/sbc_packing.c
            )
    target_include_directories(pico_btstack_sbc_encoder_headers INTERFACE
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/encoder/include
            )

    pico_add_library(pico_btstack_sbc_decoder NOFLAG)
    target_sources(pico_btstack_sbc_decoder INTERFACE
            ${PICO_BTSTACK_PATH}/src/classic/btstack_sbc_decoder_bluedroid.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/srce/alloc.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/srce/bitalloc.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/srce/bitalloc-sbc.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/srce/bitstream-decode.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/srce/decoder-oina.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/srce/decoder-private.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/srce/decoder-sbc.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/srce/dequant.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/srce/framing.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/srce/framing-sbc.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/srce/oi_codec_version.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/srce/synthesis-sbc.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/srce/synthesis-dct8.c
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/srce/synthesis-8-generated.c
            )
    target_include_directories(pico_btstack_sbc_decoder_headers INTERFACE
            ${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/include
            )

    pico_add_library(pico_btstack_bnep_lwip NOFLAG)
    target_sources(pico_btstack_bnep_lwip INTERFACE
            ${PICO_BTSTACK_PATH}/src/classic/bnep.c
            ${PICO_BTSTACK_PATH}/platform/lwip/bnep_lwip.c
            )
    target_include_directories(pico_btstack_bnep_lwip_headers INTERFACE
            ${PICO_BTSTACK_PATH}/platform/lwip
            )

    pico_add_library(pico_btstack_bnep_lwip_sys_freertos NOFLAG)
    target_include_directories(pico_btstack_bnep_lwip_sys_freertos INTERFACE
            ${PICO_BTSTACK_PATH}/platform/freertos
            )
    pico_mirrored_target_link_libraries(pico_btstack_bnep_lwip_sys_freertos INTERFACE
            pico_btstack_bnep_lwip
    )
    target_compile_definitions(pico_btstack_bnep_lwip_sys_freertos INTERFACE
            LWIP_PROVIDE_ERRNO=1
            PICO_LWIP_CUSTOM_LOCK_TCPIP_CORE=1
            )

    pico_promote_common_scope_vars()

    # Make a GATT header file from a BTstack GATT file
    # Pass the target library name library type and path to the GATT input file
    function(pico_btstack_make_gatt_header TARGET_LIB TARGET_TYPE GATT_FILE)
            find_package (Python3 REQUIRED COMPONENTS Interpreter)
            get_filename_component(GATT_NAME "${GATT_FILE}" NAME_WE)
            get_filename_component(GATT_PATH "${GATT_FILE}" PATH)
            set(GATT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
            set(GATT_HEADER "${GATT_BINARY_DIR}/${GATT_NAME}.h")
            set(TARGET_GATT "${TARGET_LIB}_gatt_header")

            add_custom_target(${TARGET_GATT} DEPENDS ${GATT_HEADER})
            add_custom_command(
                    OUTPUT ${GATT_HEADER}
                    DEPENDS ${GATT_FILE}
                    WORKING_DIRECTORY ${GATT_PATH}
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${GATT_BINARY_DIR} &&
                            ${Python3_EXECUTABLE} ${PICO_SDK_PATH}/lib/btstack/tool/compile_gatt.py ${GATT_FILE} ${GATT_HEADER}
                    VERBATIM)
            add_dependencies(${TARGET_LIB}
                    ${TARGET_GATT}
                    )
            target_include_directories(${TARGET_LIB} ${TARGET_TYPE}
                    ${GATT_BINARY_DIR}
                    )
    endfunction()

    function(suppress_btstack_warnings)
        set_source_files_properties(
            ${PICO_BTSTACK_PATH}/src/ble/att_server.c
            ${PICO_BTSTACK_PATH}/src/ble/gatt-service/device_information_service_server.c
            ${PICO_BTSTACK_PATH}/src/ble/gatt-service/hids_client.c
            ${PICO_BTSTACK_PATH}/src/btstack_util.c
            ${PICO_BTSTACK_PATH}/src/btstack_crypto.c
            ${PICO_BTSTACK_PATH}/src/classic/a2dp.c
            ${PICO_BTSTACK_PATH}/src/classic/a2dp_sink.c
            ${PICO_BTSTACK_PATH}/src/classic/a2dp_source.c
            ${PICO_BTSTACK_PATH}/src/classic/avdtp.c
            ${PICO_BTSTACK_PATH}/src/classic/avdtp_source.c
            ${PICO_BTSTACK_PATH}/src/classic/avrcp.c
            ${PICO_BTSTACK_PATH}/src/classic/avrcp_controller.c
            ${PICO_BTSTACK_PATH}/src/classic/btstack_sbc_decoder_bluedroid.c
            ${PICO_BTSTACK_PATH}/src/classic/avrcp_target.c
            ${PICO_BTSTACK_PATH}/src/classic/hid_device.c
            ${PICO_BTSTACK_PATH}/src/classic/hsp_ag.c
            ${PICO_BTSTACK_PATH}/src/classic/hsp_hs.c
            ${PICO_BTSTACK_PATH}/src/classic/pan.c
            ${PICO_BTSTACK_PATH}/src/classic/pbap_client.c
            ${PICO_BTSTACK_PATH}/src/classic/rfcomm.c
            ${PICO_BTSTACK_PATH}/src/classic/sdp_client_rfcomm.c
            ${PICO_BTSTACK_PATH}/src/classic/sdp_server.c
            ${PICO_BTSTACK_PATH}/src/classic/spp_server.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-cast-qual"
            )
        set_source_files_properties(
            ${PICO_BTSTACK_PATH}/src/ble/sm.c
            ${PICO_BTSTACK_PATH}/src/l2cap.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-cast-qual;-Wno-unused-parameter"
            )
        set_source_files_properties(
            ${PICO_BTSTACK_PATH}/src/btstack_hid_parser.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-maybe-uninitialized"
            )
        set_source_files_properties(
            ${PICO_BTSTACK_PATH}/src/btstack_tlv_none.c
            ${PICO_BTSTACK_PATH}/src/classic/avdtp_util.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-unused-parameter"
            )
        set_source_files_properties(
            ${PICO_BTSTACK_PATH}/platform/embedded/hci_dump_embedded_stdout.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-suggest-attribute=format"
            )
        set_source_files_properties(
            ${PICO_BTSTACK_PATH}/src/hci.c
            ${PICO_BTSTACK_PATH}/src/classic/rfcomm.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-cast-qual;-Wno-format"
            )
        set_source_files_properties(
            ${PICO_BTSTACK_PATH}/platform/embedded/hal_flash_bank_memory.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-sign-compare;-Wno-format"
            )
        set_source_files_properties(
            ${PICO_BTSTACK_PATH}/platform/embedded/btstack_tlv_flash_bank.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-unused-parameter;-Wno-sign-compare"
            )
        set_source_files_properties(
            ${PICO_BTSTACK_PATH}/src/ble/gatt-service/hids_client.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-cast-qual;-Wno-null-dereference"
            )
        set_source_files_properties(
            ${PICO_BTSTACK_PATH}/src/classic/hfp.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-cast-qual;-Wno-null-dereference;-Wno-unused-parameter"
            )
        set_source_files_properties(
            ${PICO_BTSTACK_PATH}/src/classic/goep_server.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-unused-parameter;-Wno-null-dereference"
            )
        set_source_files_properties(
            ${PICO_BTSTACK_PATH}/src/ble/gatt-service/battery_service_client.c
            ${PICO_BTSTACK_PATH}/src/ble/gatt-service/device_information_service_client.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-null-dereference"
            )
        set_source_files_properties(
            ${PICO_BTSTACK_PATH}/src/classic/hfp_hf.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-type-limits;-Wno-stringop-overflow"
            )
        set_source_files_properties(
            ${PICO_BTSTACK_PATH}/src/btstack_crypto.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-cast-qual;-Wno-sign-compare"
            )
    endfunction()
endif()
