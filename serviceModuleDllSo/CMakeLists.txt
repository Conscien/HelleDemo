cmake_minimum_required(VERSION 2.8)
project(myDllSo)

set(CMAKE_CXX_STANDARD 11)

add_library(myDllSo SHARED library.cpp)
