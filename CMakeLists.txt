cmake_minimum_required(VERSION 3.10)
project(membrane)

set(CMAKE_CXX_STANDARD 17)

set(PROJECT_SOURCES src/main.cpp src/mbnwrite.cpp)

add_executable(membrane ${PROJECT_SOURCES})