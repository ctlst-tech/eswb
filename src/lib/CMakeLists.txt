cmake_minimum_required(VERSION 3.6)

if (NOT FAKE_PTHREAD)
    if (UNIX AND NOT APPLE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
    endif()
endif()

set(ESWB_LIB_SRC
        api.c
        topic_proclaiming_tree.c
        event_queue.c
        bridge.c
        registry.c
        domain_switching.c
        local_buses.c
        topic_io.c
        topic_mem.c
        misc/errors.c
        misc/misc.c
        include/public/eswb/api.h
        include/public/eswb/types.h
        include/public/eswb/event_queue.h
        include/public/eswb/bridge.h
        include/registry.h
        include/topic_mem.h
        include/local_buses.h
        include/domain_switching.h
        include/topic_io.h
        include/sync.h
        include/ids_map.h)

add_library(eswb-static STATIC ${ESWB_LIB_SRC})

target_include_directories(eswb-static PUBLIC
        include/public
        )
target_include_directories(eswb-static PRIVATE
        include
        )



set(ESWB_UTIL_EQRB_SRC
        services/eqrb/eqrb_client.c
        services/eqrb/eqrb_server.c
        services/eqrb/eqrb_misc.c
        services/eqrb/eqrb_priv.h
        services/eqrb/drivers/sdtl.c
        services/eqrb/drivers/file.c
        include/public/eswb/services/eqrb.h
        )

set(ESWB_SERVICE_SDTL_SRC
        services/sdtl/sdtl.c
        services/sdtl/sdtl_opaque.h
        include/public/eswb/services/sdtl.h
        services/sdtl/media_serial.c
        services/sdtl/crc16-ccitt.c
        services/sdtl/crc16-ccitt.h
        services/sdtl/bbee_framing.c
        services/sdtl/bbee_framing.h
        )

add_library(eswb-eqrb-static STATIC ${ESWB_UTIL_EQRB_SRC})

target_include_directories(eswb-eqrb-static PUBLIC include/public)
target_include_directories(eswb-eqrb-static PRIVATE include)

target_link_libraries(eswb-eqrb-static PRIVATE eswb-static)

add_library(eswb-sdtl-static STATIC ${ESWB_SERVICE_SDTL_SRC})

target_include_directories(eswb-sdtl-static PUBLIC include/public)
target_include_directories(eswb-sdtl-static PRIVATE include)

target_link_libraries(eswb-sdtl-static PRIVATE eswb-static)

if (ESWB_EQRB_NO_SOCKET)
    target_compile_definitions(eswb-eqrb-static PUBLIC ESWB_NO_SOCKET=1)
endif()

if (ESWB_EQRB_NO_SERIAL)
    target_compile_definitions(eswb-eqrb-static PUBLIC ESWB_NO_SERIAL=1)
endif()

add_library(eswb SHARED ${ESWB_LIB_SRC} ${ESWB_UTIL_EQRB_SRC} ${ESWB_SERVICE_SDTL_SRC} platformic/posix/posix_sync.c)
target_link_libraries(eswb PUBLIC ${PL_SPECIFIC_LIBS})

target_include_directories(eswb PUBLIC
        include/public
        )
target_include_directories(eswb PRIVATE
        include
        )

if (ESWB_DEBUG)
    message("ESWB_DEBUG activated")
    target_compile_definitions(eswb-eqrb-static PUBLIC EQRB_DEBUG)
    target_compile_definitions(eswb PUBLIC EQRB_DEBUG)
endif()

include(GNUInstallDirs)

install(TARGETS eswb LIBRARY)
install(DIRECTORY include/public/eswb
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

#set_target_properties(eswb PROPERTIES PUBLIC_HEADER "include/public/eswb")


#
#add_library(eswb-static-cov STATIC ${ESWB_LIB_SRC})
#
#target_include_directories(eswb-static-cov PUBLIC
#        include/public
#        )
#
#target_include_directories(eswb-static-cov PRIVATE
#        include
#        )
#
#
#target_compile_options(eswb-static-cov PUBLIC --coverage)
