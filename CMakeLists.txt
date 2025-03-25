cmake_minimum_required(VERSION 3.16)
project(orm++ CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


file(READ ${CMAKE_SOURCE_DIR}/config.json CONFIG_JSON)
string(JSON DB_ENGINE GET ${CONFIG_JSON} "db_engine")

if(${DB_ENGINE} STREQUAL "psql")
  set(DB_DEFN "#define PSQL")
else()
  message(FATAL_ERROR "Unsupported database engine chosen")
endif()

file(WRITE ${CMAKE_SOURCE_DIR}/core/includes/db_config.hpp "${DB_DEFN}\n")

file(GLOB CORE_SOURCES core/src/*.cpp)
include_directories(core/includes)
add_library(orm++ SHARED ${CORE_SOURCES})
target_include_directories(orm++ PUBLIC core/includes)
