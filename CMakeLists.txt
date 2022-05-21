cmake_minimum_required(VERSION 3.6)
project(eswb)
set(CMAKE_CXX_STANDARD 14)

if (NOT FAKE_PTHREAD)
    if (UNIX AND NOT APPLE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -D_GNU_SOURCE")
    endif()
endif()
#set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(Catch2 REQUIRED)

add_subdirectory(src/lib)
add_subdirectory(src/lib/platformic/posix)

add_executable(eswb_catch2_test tests/test-main.cpp tests/tests.cpp tests/tests.h tests/event_chain.c tests/eqrb.cpp tests/tooling.cpp tests/tooling.h)

add_executable(eswb_c_test tests/event_chain.c)
target_link_libraries(eswb_c_test PUBLIC m)
target_link_libraries(eswb_c_test PUBLIC eswb)
target_compile_options(eswb_c_test PUBLIC -DESWB_C_TEST=1)


target_include_directories(eswb_catch2_test PRIVATE src/lib/include)
target_link_libraries(eswb_catch2_test PUBLIC eswb-static eswb-sync-static eswb-eqrb-static Catch2::Catch2)

add_library(eswb-if INTERFACE)
target_link_libraries(eswb-if INTERFACE eswb-static eswb-sync-static eswb-eqrb-static)

add_library(eswb-cpp cpp/eswb.hpp cpp/eswb.cpp)
target_link_libraries(eswb-cpp eswb-if)

add_executable(eswbutil cpp/main.cpp)
target_link_libraries(eswbutil eswb-cpp)


#target_compile_options(eswb_catch2_test PUBLIC -DESWB_UNIT_TESTS=1)
