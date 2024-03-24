if (DEFINED ENV{PICO_LWIP_PATH} AND (NOT PICO_LWIP_PATH))
    set(PICO_LWIP_PATH $ENV{PICO_LWIP_PATH})
    message("Using PICO_LWIP_PATH from environment ('${PICO_LWIP_PATH}')")
endif ()

set(LWIP_TEST_PATH "src/Filelists.cmake")
if (NOT PICO_LWIP_PATH)
    set(PICO_LWIP_PATH ${PROJECT_SOURCE_DIR}/lib/lwip)
    if (PICO_CYW43_SUPPORTED AND NOT EXISTS ${PICO_LWIP_PATH}/${LWIP_TEST_PATH})
        message(WARNING "LWIP submodule has not been initialized; Pico W wireless support will be unavailable
#hint: try 'git submodule update --init' from your SDK directory (${PICO_SDK_PATH}).")
    endif()
elseif (NOT EXISTS ${PICO_LWIP_PATH}/${LWIP_TEST_PATH})
    message(WARNING "PICO_LWIP_PATH specified but content not present.")
endif()

if (EXISTS ${PICO_LWIP_PATH}/${LWIP_TEST_PATH})
    message("lwIP available at ${PICO_LWIP_PATH}")

    # argh... wanted to use this, but they dump stuff into the source tree, which breaks parallel builds
    #set(LWIP_DIR ${PICO_LWIP_PATH})
    #include(${PICO_LWIP_PATH}/src/Filelists.cmake)

    pico_register_common_scope_var(PICO_LWIP_PATH)

    # The minimum set of files needed for lwIP.
    pico_add_library(pico_lwip_core NOFLAG)
    target_sources(pico_lwip_core INTERFACE
            ${PICO_LWIP_PATH}/src/core/init.c
            ${PICO_LWIP_PATH}/src/core/def.c
            ${PICO_LWIP_PATH}/src/core/dns.c
            ${PICO_LWIP_PATH}/src/core/inet_chksum.c
            ${PICO_LWIP_PATH}/src/core/ip.c
            ${PICO_LWIP_PATH}/src/core/mem.c
            ${PICO_LWIP_PATH}/src/core/memp.c
            ${PICO_LWIP_PATH}/src/core/netif.c
            ${PICO_LWIP_PATH}/src/core/pbuf.c
            ${PICO_LWIP_PATH}/src/core/raw.c
            ${PICO_LWIP_PATH}/src/core/stats.c
            ${PICO_LWIP_PATH}/src/core/sys.c
            ${PICO_LWIP_PATH}/src/core/altcp.c
            ${PICO_LWIP_PATH}/src/core/altcp_alloc.c
            ${PICO_LWIP_PATH}/src/core/altcp_tcp.c
            ${PICO_LWIP_PATH}/src/core/tcp.c
            ${PICO_LWIP_PATH}/src/core/tcp_in.c
            ${PICO_LWIP_PATH}/src/core/tcp_out.c
            ${PICO_LWIP_PATH}/src/core/timeouts.c
            ${PICO_LWIP_PATH}/src/core/udp.c
            )
    target_include_directories(pico_lwip_core_headers INTERFACE
            ${PICO_LWIP_PATH}/src/include)

    pico_add_library(pico_lwip_core4 NOFLAG)
    target_sources(pico_lwip_core4 INTERFACE
            ${PICO_LWIP_PATH}/src/core/ipv4/autoip.c
            ${PICO_LWIP_PATH}/src/core/ipv4/dhcp.c
            ${PICO_LWIP_PATH}/src/core/ipv4/etharp.c
            ${PICO_LWIP_PATH}/src/core/ipv4/icmp.c
            ${PICO_LWIP_PATH}/src/core/ipv4/igmp.c
            ${PICO_LWIP_PATH}/src/core/ipv4/ip4_frag.c
            ${PICO_LWIP_PATH}/src/core/ipv4/ip4.c
            ${PICO_LWIP_PATH}/src/core/ipv4/ip4_addr.c
            )

    # Doesn't exists in version earlier than 2.1.3
    if (EXISTS ${PICO_LWIP_PATH}/src/core/ipv4/acd.c)
        target_sources(pico_lwip_core4 INTERFACE
            ${PICO_LWIP_PATH}/src/core/ipv4/acd.c
            )
    endif()

    pico_add_library(pico_lwip_core6 NOFLAG)
    target_sources(pico_lwip_core6 INTERFACE
            ${PICO_LWIP_PATH}/src/core/ipv6/dhcp6.c
            ${PICO_LWIP_PATH}/src/core/ipv6/ethip6.c
            ${PICO_LWIP_PATH}/src/core/ipv6/icmp6.c
            ${PICO_LWIP_PATH}/src/core/ipv6/inet6.c
            ${PICO_LWIP_PATH}/src/core/ipv6/ip6.c
            ${PICO_LWIP_PATH}/src/core/ipv6/ip6_addr.c
            ${PICO_LWIP_PATH}/src/core/ipv6/ip6_frag.c
            ${PICO_LWIP_PATH}/src/core/ipv6/mld6.c
            ${PICO_LWIP_PATH}/src/core/ipv6/nd6.c
            )

    # APIFILES: The files which implement the sequential and socket APIs.
    pico_add_library(pico_lwip_api NOFLAG)
    target_sources(pico_lwip_api INTERFACE
            ${PICO_LWIP_PATH}/src/api/api_lib.c
            ${PICO_LWIP_PATH}/src/api/api_msg.c
            ${PICO_LWIP_PATH}/src/api/err.c
            ${PICO_LWIP_PATH}/src/api/if_api.c
            ${PICO_LWIP_PATH}/src/api/netbuf.c
            ${PICO_LWIP_PATH}/src/api/netdb.c
            ${PICO_LWIP_PATH}/src/api/netifapi.c
            ${PICO_LWIP_PATH}/src/api/sockets.c
            ${PICO_LWIP_PATH}/src/api/tcpip.c
            )

    # Files implementing various generic network interface functions
    pico_add_library(pico_lwip_netif NOFLAG)
    target_sources(pico_lwip_netif INTERFACE
            ${PICO_LWIP_PATH}/src/netif/ethernet.c
            ${PICO_LWIP_PATH}/src/netif/bridgeif.c
            ${PICO_LWIP_PATH}/src/netif/bridgeif_fdb.c
            ${PICO_LWIP_PATH}/src/netif/slipif.c
            )

    # 6LoWPAN
    pico_add_library(pico_lwip_sixlowpan NOFLAG)
    target_sources(pico_lwip_sixlowpan INTERFACE
            ${PICO_LWIP_PATH}/src/netif/lowpan6_common.c
            ${PICO_LWIP_PATH}/src/netif/lowpan6.c
            ${PICO_LWIP_PATH}/src/netif/lowpan6_ble.c
            ${PICO_LWIP_PATH}/src/netif/zepif.c
            )

    # PPP
    pico_add_library(pico_lwip_ppp NOFLAG)
    target_sources(pico_lwip_ppp INTERFACE
            ${PICO_LWIP_PATH}/src/netif/ppp/auth.c
            ${PICO_LWIP_PATH}/src/netif/ppp/ccp.c
            ${PICO_LWIP_PATH}/src/netif/ppp/chap-md5.c
            ${PICO_LWIP_PATH}/src/netif/ppp/chap_ms.c
            ${PICO_LWIP_PATH}/src/netif/ppp/chap-new.c
            ${PICO_LWIP_PATH}/src/netif/ppp/demand.c
            ${PICO_LWIP_PATH}/src/netif/ppp/eap.c
            ${PICO_LWIP_PATH}/src/netif/ppp/ecp.c
            ${PICO_LWIP_PATH}/src/netif/ppp/eui64.c
            ${PICO_LWIP_PATH}/src/netif/ppp/fsm.c
            ${PICO_LWIP_PATH}/src/netif/ppp/ipcp.c
            ${PICO_LWIP_PATH}/src/netif/ppp/ipv6cp.c
            ${PICO_LWIP_PATH}/src/netif/ppp/lcp.c
            ${PICO_LWIP_PATH}/src/netif/ppp/magic.c
            ${PICO_LWIP_PATH}/src/netif/ppp/mppe.c
            ${PICO_LWIP_PATH}/src/netif/ppp/multilink.c
            ${PICO_LWIP_PATH}/src/netif/ppp/ppp.c
            ${PICO_LWIP_PATH}/src/netif/ppp/pppapi.c
            ${PICO_LWIP_PATH}/src/netif/ppp/pppcrypt.c
            ${PICO_LWIP_PATH}/src/netif/ppp/pppoe.c
            ${PICO_LWIP_PATH}/src/netif/ppp/pppol2tp.c
            ${PICO_LWIP_PATH}/src/netif/ppp/pppos.c
            ${PICO_LWIP_PATH}/src/netif/ppp/upap.c
            ${PICO_LWIP_PATH}/src/netif/ppp/utils.c
            ${PICO_LWIP_PATH}/src/netif/ppp/vj.c
            ${PICO_LWIP_PATH}/src/netif/ppp/polarssl/arc4.c
            ${PICO_LWIP_PATH}/src/netif/ppp/polarssl/des.c
            ${PICO_LWIP_PATH}/src/netif/ppp/polarssl/md4.c
            ${PICO_LWIP_PATH}/src/netif/ppp/polarssl/md5.c
            ${PICO_LWIP_PATH}/src/netif/ppp/polarssl/sha1.c
            )

    # SNMPv3 agent
    pico_add_library(pico_lwip_snmp NOFLAG)
    target_sources(pico_lwip_snmp INTERFACE
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_asn1.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_core.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_mib2.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_mib2_icmp.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_mib2_interfaces.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_mib2_ip.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_mib2_snmp.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_mib2_system.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_mib2_tcp.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_mib2_udp.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_snmpv2_framework.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_snmpv2_usm.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_msg.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmpv3.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_netconn.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_pbuf_stream.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_raw.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_scalar.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_table.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_threadsync.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmp_traps.c
            )

    # HTTP server + client
    pico_add_library(pico_lwip_http NOFLAG)
    target_sources(pico_lwip_http INTERFACE
            ${PICO_LWIP_PATH}/src/apps/http/altcp_proxyconnect.c
            ${PICO_LWIP_PATH}/src/apps/http/fs.c
            ${PICO_LWIP_PATH}/src/apps/http/http_client.c
            ${PICO_LWIP_PATH}/src/apps/http/httpd.c
            )

    # MAKEFSDATA HTTP server host utility
    pico_add_library(pico_lwip_makefsdata NOFLAG)
    target_sources(pico_lwip_makefsdata INTERFACE
            ${PICO_LWIP_PATH}/src/apps/http/makefsdata/makefsdata.c
            )

    # iperf
    pico_add_library(pico_lwip_iperf NOFLAG)
    target_sources(pico_lwip_iperf INTERFACE
            ${PICO_LWIP_PATH}/src/apps/lwiperf/lwiperf.c
            )

    # SMTP client
    pico_add_library(pico_lwip_smtp NOFLAG)
    target_sources(pico_lwip_smtp INTERFACE
            ${PICO_LWIP_PATH}/src/apps/smtp/smtp.c
            )

    # SNTP client
    pico_add_library(pico_lwip_sntp NOFLAG)
    target_sources(pico_lwip_sntp INTERFACE
            ${PICO_LWIP_PATH}/src/apps/sntp/sntp.c
            )

    # MDNS responder
    pico_add_library(pico_lwip_mdns NOFLAG)
    target_sources(pico_lwip_mdns INTERFACE
            ${PICO_LWIP_PATH}/src/apps/mdns/mdns.c
            )

    # Old versions of lwip had everything in mdns.c
    if (EXISTS ${PICO_LWIP_PATH}/src/apps/mdns/mdns_out.c)
        target_sources(pico_lwip_mdns INTERFACE
                ${PICO_LWIP_PATH}/src/apps/mdns/mdns_out.c
                ${PICO_LWIP_PATH}/src/apps/mdns/mdns_domain.c
                )
    endif()

    # NetBIOS name server
    pico_add_library(pico_lwip_netbios NOFLAG)
    target_sources(pico_lwip_netbios INTERFACE
            ${PICO_LWIP_PATH}/src/apps/netbiosns/netbiosns.c
            )

    # TFTP server files
    pico_add_library(pico_lwip_tftp NOFLAG)
    target_sources(pico_lwip_tftp INTERFACE
            ${PICO_LWIP_PATH}/src/apps/tftp/tftp.c
            )

    # Mbed TLS files
    pico_add_library(pico_lwip_mbedtls NOFLAG)
    target_sources(pico_lwip_mbedtls INTERFACE
            ${PICO_LWIP_PATH}/src/apps/altcp_tls/altcp_tls_mbedtls.c
            ${PICO_LWIP_PATH}/src/apps/altcp_tls/altcp_tls_mbedtls_mem.c
            ${PICO_LWIP_PATH}/src/apps/snmp/snmpv3_mbedtls.c
            )

    # MQTT client files
    pico_add_library(pico_lwip_mqtt NOFLAG)
    target_sources(pico_lwip_mqtt INTERFACE
            ${PICO_LWIP_PATH}/src/apps/mqtt/mqtt.c
            )

    # All LWIP files without apps
    pico_add_library(pico_lwip NOFLAG)
    pico_mirrored_target_link_libraries(pico_lwip INTERFACE
        pico_lwip_core
        pico_lwip_core4
        pico_lwip_core6
        pico_lwip_api
        pico_lwip_netif
        pico_lwip_sixlowpan
        pico_lwip_ppp
    )

    # our arch/cc.h
    pico_add_library(pico_lwip_arch NOFLAG)
    target_include_directories(pico_lwip_arch_headers INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/include)
    pico_mirrored_target_link_libraries(pico_lwip_arch INTERFACE pico_rand)

    # our nosys impl
    pico_add_library(pico_lwip_nosys NOFLAG)
    target_sources(pico_lwip_nosys INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/lwip_nosys.c
            )
    pico_mirrored_target_link_libraries(pico_lwip_nosys INTERFACE
            pico_async_context_base
            pico_lwip_arch
            pico_lwip)

    if (NOT PICO_LWIP_CONTRIB_PATH)
        set(PICO_LWIP_CONTRIB_PATH ${PICO_LWIP_PATH}/contrib)
    endif()
    pico_register_common_scope_var(PICO_LWIP_CONTRIB_PATH)

    # Make lwip_contrib_freertos library, with the FreeRTOS/lwIP code from lwip-contrib
    pico_add_library(pico_lwip_contrib_freertos NOFLAG)
    target_sources(pico_lwip_contrib_freertos INTERFACE
            ${PICO_LWIP_CONTRIB_PATH}/ports/freertos/sys_arch.c
            )
    target_include_directories(pico_lwip_contrib_freertos_headers INTERFACE
            ${PICO_LWIP_CONTRIB_PATH}/ports/freertos/include
            )
    pico_mirrored_target_link_libraries(pico_lwip_contrib_freertos INTERFACE
            pico_lwip_arch)

    pico_add_library(pico_lwip_freertos NOFLAG)
    target_sources(pico_lwip_freertos INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/lwip_freertos.c
            )
    pico_mirrored_target_link_libraries(pico_lwip_freertos INTERFACE
            pico_async_context_base
            pico_lwip
            pico_lwip_contrib_freertos
            pico_rand)

    pico_promote_common_scope_vars()
endif()
