cmake_minimum_required(VERSION 3.16)
project(orm++ CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED DB_ENGINE)
  set(DB_ENGINE "PSQL")
endif()

if(DB_ENGINE STREQUAL "PSQL")
  set(DB_MACRO "#define PSQL")
else()
  message(FATAL_ERROR "Unsupported database engine chosen: ${DB_ENGINE}")
endif()

file(WRITE "${CMAKE_SOURCE_DIR}/orm++/db_config.hpp" "#pragma once\n${DB_MACRO}\n")

#Builds only shared rn.
file(GLOB CORE_SOURCES src/*.cpp)
include_directories(orm++)
add_library(orm++ SHARED ${CORE_SOURCES})
target_include_directories(orm++ PUBLIC orm++/)
