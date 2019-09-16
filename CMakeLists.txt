cmake_minimum_required(VERSION 3.1)

option(packetprocessor_BUILD_EXAMPLE "build example" ON)

project(packetprocessor)

set(CMAKE_CXX_STANDARD 11)
add_compile_options(-Wall)

add_library(${PROJECT_NAME} STATIC
        PacketProcessor.cpp
        crc/crc16.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC .)

# log
include_directories(modules/log)

# example
if(packetprocessor_BUILD_EXAMPLE)
    include_directories(.)
    link_libraries(${PROJECT_NAME})
    add_executable(packetprocessor_exampple example/packet_example.cpp)
endif()
