cmake_minimum_required(VERSION 3.0)
project(REQUEST)

set(CMAKE_CXX_STANDARD 11)

add_executable(requester requester.cpp)

target_link_libraries(requester
    PRIVATE
    -lrt
    -lpthread
    Logger
)
