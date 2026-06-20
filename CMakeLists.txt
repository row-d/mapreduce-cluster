cmake_minimum_required(VERSION 3.10)

project(MapReduceSocket)

find_package(asio CONFIG REQUIRED)

file(GLOB_RECURSE FUENTES
    "src/main.cpp"
    "lib/*.cpp"
)

add_executable(MapReduceSocket ${FUENTES})

target_link_libraries(MapReduceSocket PRIVATE asio::asio ws2_32)
target_include_directories(MapReduceSocket PRIVATE lib)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
