# Standalone build for the userfaultfd on-demand core-dump experiment.
# Linux-only in substance; configures (as a no-op binary) elsewhere.
#
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
#   cmake --build build
#   ./build/uffd_coredump_demo /tmp/ondemand.core 256 4 2

cmake_minimum_required(VERSION 3.16)
project(uffd_coredump_experiment CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_library(uffd_coredump STATIC uffd_coredump.cpp)
target_include_directories(uffd_coredump PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(uffd_coredump_demo demo_main.cpp)
target_link_libraries(uffd_coredump_demo PRIVATE uffd_coredump)

find_package(Threads REQUIRED)
target_link_libraries(uffd_coredump PUBLIC Threads::Threads)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(WARNING
        "uffd_coredump is substantive only on Linux; on ${CMAKE_SYSTEM_NAME} "
        "it builds as a no-op that reports Status::Unsupported.")
endif()
