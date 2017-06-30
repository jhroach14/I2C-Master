cmake_minimum_required(VERSION 3.6)
project(i2c_Master)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp)
add_executable(i2c_Master ${SOURCE_FILES})