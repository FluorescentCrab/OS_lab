cmake_minimum_required(VERSION 3.10)
project(MyImageProject)

# Forces the compiler to use C++11 or newer (fixing your std::to_string issue!)
set(CMAKE_CXX_STANDARD 20 )
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(my_program image_sharpener.cpp libppm.cpp)
