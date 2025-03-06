cmake_minimum_required(VERSION 3.16)
project(orm CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#find_package(libpqxx REQUIRED)

file(GLOB CORE_SOURCES core/src/*.cpp)
include_directories(core/includes)
add_library(core SHARED ${CORE_SOURCES})
target_include_directories(core PUBLIC core/includes)

#file(GLOB USER_SOURCES user/src/models.cpp)
#include_directories(user/includes)
#add_executable(migrate ${USER_SOURCES})
#target_include_directories(migrate PUBLIC user/includes)
#target_link_libraries(migrate PRIVATE core pqxx)
