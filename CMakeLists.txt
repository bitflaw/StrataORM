cmake_minimum_required(VERSION 3.16)
project(orm CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB CORE_SOURCES core/src/*.cpp)
include_directories(core/includes)
add_library(core SHARED ${CORE_SOURCES})
target_include_directories(core PUBLIC core/includes)
