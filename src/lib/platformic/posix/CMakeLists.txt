

set(ESWB_SYNC_SRC
        posix_sync.c
    )

add_library(eswb-sync-static STATIC ${ESWB_SYNC_SRC})


target_include_directories(eswb-sync-static  PRIVATE
        ../../include/
        ../../include/public
        )

target_link_libraries(eswb-sync-static pthread)
