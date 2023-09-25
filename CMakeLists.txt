cmake_minimum_required(VERSION 3.5)

option(PacketProcessor_BUILD_TEST "" OFF)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(PacketProcessor_BUILD_TEST ON)
endif ()

project(PacketProcessor)

set(CMAKE_CXX_STANDARD 11)
add_compile_options(-Wall)

add_library(${PROJECT_NAME} STATIC
        PacketProcessor.cpp
        crc/crc16.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC .)

if (PacketProcessor_BUILD_TEST)
    link_libraries(${PROJECT_NAME})
    add_executable(${PROJECT_NAME}_test test/main.cpp)
endif ()
