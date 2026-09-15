cmake_minimum_required(VERSION 3.10)
project(os_lab)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(fifo fifo.cpp processes.hpp processes.cpp )
add_executable(round_robin round_robin.cpp processes.hpp processes.cpp )
add_executable(mlfq mlfq.cpp processes.hpp processes.cpp )