cmake_minimum_required(VERSION 3.16)
project(StrataORM VERSION 0.1.0 LANGUAGES CXX)

#set CXX standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Build options
option(DB_ENGINE "Target database engine(only 'PSQL' supported now)" "PSQL")
option(BUILD_SHARED_LIBS "Build a shared library" ON)

#Database Engine Macro setup
if(DB_ENGINE STREQUAL "PSQL")
  set(DB_MACRO "#define PSQL")
else()
  message(FATAL_ERROR "Unsupported database engine chosen: ${DB_ENGINE}")
endif()

#Generate db_config for db_engine
file(WRITE "${CMAKE_SOURCE_DIR}/strata/db_config.hpp" "#pragma once\n${DB_MACRO}\n")

#Collect src files
file(GLOB SRC CONFIGURE_DEPENDS src/*.cpp)
#include headers
include_directories(strata)

# BUILD SHARED OR STATIC
if(BUILD_SHARED_LIBS)
  add_library(strata SHARED ${SRC})
else()
  add_library(strata STATIC ${SRC})
endif()

#generate version.hpp from version.hpp.in in strata/
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/strata/version.hpp.in
  ${CMAKE_CURRENT_SOURCE_DIR}/strata/version.hpp
  @ONLY
)

#Public headers for consumers
target_include_directories(strata
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/strata>
  $<INSTALL_INTERFACE:include>
)

#Install libs
install(TARGETS strata
  EXPORT strata-targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

#Install headers
install(DIRECTORY strata
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp"
)

#Install config
install(EXPORT strata-targets
  FILE strata-config.cmake
  DESTINATION lib/cmake/strata
)
