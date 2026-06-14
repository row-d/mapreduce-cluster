cmake_minimum_required(VERSION 3.10)

project(MapReduceSocket)

find_package(asio CONFIG REQUIRED)

add_executable(MapReduceSocket src/main.cpp)

target_link_libraries(MapReduceSocket PRIVATE asio::asio ws2_32)