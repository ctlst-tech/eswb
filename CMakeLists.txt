cmake_minimum_required(VERSION 3.6)
project(eswb)
set(CMAKE_CXX_STANDARD 14)
# set(CMAKE_VERBOSE_MAKEFILE ON)

if (NOT FAKE_PTHREAD)
    if (UNIX AND NOT APPLE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -D_GNU_SOURCE")
    endif()
endif()

add_executable(eswb_test_dummy tests/eswb_test_dummy.c tests/event_chain.c)
target_link_libraries(eswb_test_dummy PUBLIC m)
target_link_libraries(eswb_test_dummy PUBLIC eswb)

add_library(eswb-if INTERFACE)
target_link_libraries(eswb-if INTERFACE eswb-static eswb-sync-static eswb-eqrb-static eswb-sdtl-static )

add_subdirectory(src/lib)
add_subdirectory(src/lib/platformic/posix)

if (ESWB_TEST STREQUAL "YES")
    find_package(Catch2 REQUIRED)
    set(TEST_SRC_COMMON
    tests/tooling.cpp
    tests/tooling.h
    tests/sdtl_tooling.cpp
    tests/sdtl_tooling.h
    )

    add_executable(eswb_test_basic tests/tests.cpp tests/tests.h tests/event_chain.c ${TEST_SRC_COMMON})
    add_executable(eswb_test_eqrb tests/eqrb.cpp  ${TEST_SRC_COMMON})
    add_executable(eswb_test_sdtl tests/sdtl.cpp ${TEST_SRC_COMMON})
    add_executable(eswb_test_bbee_framing tests/bbee_framing.cpp ${TEST_SRC_COMMON})
    add_executable(eswb_test_eqrb_file tests/eqrb_file.cpp ${TEST_SRC_COMMON})


add_executable(eswb_test_all
    tests/tests.cpp
    tests/tests.h
    tests/event_chain.c
    tests/sdtl.cpp
    tests/eqrb.cpp
    tests/bbee_framing.cpp
    tests/eqrb_file.cpp
    ${TEST_SRC_COMMON})

    target_include_directories(eswb_test_basic PRIVATE src/lib/include)
    target_link_libraries(eswb_test_basic PUBLIC eswb-static eswb-sync-static Catch2::Catch2WithMain)

    target_include_directories(eswb_test_eqrb PRIVATE src/lib/include)
    target_link_libraries(eswb_test_eqrb PUBLIC eswb-static eswb-eqrb-static eswb-sdtl-static eswb-sync-static Catch2::Catch2WithMain)

    target_include_directories(eswb_test_sdtl PRIVATE src/lib/include)
    target_link_libraries(eswb_test_sdtl PUBLIC eswb-static eswb-sdtl-static eswb-sync-static Catch2::Catch2WithMain)

    target_include_directories(eswb_test_all PRIVATE src/lib/include)
    target_link_libraries(eswb_test_all PUBLIC eswb-static eswb-eqrb-static eswb-sdtl-static eswb-sync-static Catch2::Catch2WithMain)

    target_include_directories(eswb_test_bbee_framing PRIVATE src/lib/include)
    target_link_libraries(eswb_test_bbee_framing PUBLIC eswb-static eswb-eqrb-static eswb-sdtl-static eswb-sync-static Catch2::Catch2WithMain)

    target_include_directories(eswb_test_eqrb_file PRIVATE src/lib/include)
    target_link_libraries(eswb_test_eqrb_file PUBLIC eswb-static eswb-eqrb-static eswb-sdtl-static eswb-sync-static Catch2::Catch2WithMain)

    add_library(eswb-cpp STATIC 
    cpp/eswb.hpp 
    cpp/eswb.cpp 
    cpp/conv/conv.cpp
    cpp/conv/conv.hpp
    cpp/sdtl/sdtl.cpp
    cpp/sdtl/sdtl.hpp)

    target_link_libraries(eswb-cpp PUBLIC eswb-static eswb-sync-static eswb-eqrb-static eswb-sdtl-static)
    target_include_directories(eswb-cpp PUBLIC cpp cpp/conv cpp/sdtl)

    add_executable(eswbutil cpp/eswbutil.cpp)
    target_link_libraries(eswbutil eswb-cpp)

    install(TARGETS eswbutil RUNTIME)
    install(TARGETS eswb_test_dummy RUNTIME)
    install(TARGETS eswb-cpp LIBRARY)
endif()
