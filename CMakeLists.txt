cmake_minimum_required(VERSION 3.25)

project(MapReduceSocket LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Dependencies
find_package(asio CONFIG REQUIRED)

if(WIN32)
    set(EXTRA_LIBS ws2_32 mswsock)
elseif(APPLE)
    set(EXTRA_LIBS "")
else()
    set(EXTRA_LIBS pthread)
endif()

# Build
add_executable(mapreduce-server src/server_main.cpp)
target_link_libraries(mapreduce-server PRIVATE asio::asio ${EXTRA_LIBS})
target_include_directories(mapreduce-server PRIVATE lib src)

add_executable(mapreduce-client src/client_main.cpp)
target_link_libraries(mapreduce-client PRIVATE asio::asio ${EXTRA_LIBS})
target_include_directories(mapreduce-client PRIVATE lib src)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
