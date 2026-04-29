cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(StrataORM VERSION 0.2.1 LANGUAGES CXX)

#set CXX standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_FLAGS_DEBUG "-std=c++20 -Wall -Wextra -Wpedantic -g")
set (CMAKE_CXX_FLAGS_RELEASE "-std=c++20 -O3")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCES
  src/datatypes.cpp
  src/models.cpp
  src/utils.cpp
)
#Build options
option(DB_ENGINE "Target database engine." "PSQL")

#Database Engine Macro setup
if(DB_ENGINE STREQUAL "PSQL")
  set(DB_MACRO "#define PSQL")
  file(GLOB DB_SRC src/psql/*.cpp)
  list(APPEND SOURCES ${DB_SRC})
  set(DB_LIBS pq pqxx)
  set(DB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/strata/psql")
elseif(DB_ENGINE STREQUAL "MARIADB")
  set(DB_MACRO "#define MARIADB")
  file(GLOB DB_SRC src/mariadb/*.cpp)
  list(APPEND SOURCES ${DB_SRC})
  set(DB_LIBS mdbcxx)
  set(DB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/strata/mariadb")
else()
  message(FATAL_ERROR "Unsupported database engine chosen: ${DB_ENGINE}")
endif()

#Generate db_config for db_engine
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/strata/db_config.hpp" "#pragma once\n${DB_MACRO}\n")

#include headers
# include_directories(strata)

#build both static and shared libraries.
#Shared
add_library(strata_shared SHARED ${SOURCES})
set_target_properties(strata_shared PROPERTIES OUTPUT_NAME "strata")
#Static
add_library(strata_static STATIC ${SOURCES})
set_target_properties(strata_static PROPERTIES OUTPUT_NAME "strata")

target_link_libraries(strata_shared PUBLIC ${DB_LIBS})
target_link_libraries(strata_static PUBLIC ${DB_LIBS})

#generate version.hpp from version.hpp.in in include/strata/
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/strata/version.hpp.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/strata/version.hpp
  @ONLY
)

#Public headers for consumers for shared lib
target_include_directories(strata_shared
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${DB_HEADERS}>
  $<INSTALL_INTERFACE:include>
)

#Public headers for consumers for static lib
target_include_directories(strata_static
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${DB_HEADERS}>
  $<INSTALL_INTERFACE:include>
)

#Install both targets
install(TARGETS strata_shared strata_static
  EXPORT strata-targets
  ARCHIVE DESTINATION lib/strata
  LIBRARY DESTINATION lib/strata
)

#Install top-level headers
install(DIRECTORY include/strata
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp"
  PATTERN "psql" EXCLUDE
  PATTERN "mariadb" EXCLUDE
)

#Install database-specific headers
install(DIRECTORY ${DB_HEADERS}
  DESTINATION include/strata
  FILES_MATCHING PATTERN "*.hpp"
)

#Install config
install(EXPORT strata-targets
  FILE strata-config.cmake
  DESTINATION lib/cmake/strata
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/strata/db_config.hpp"
  DESTINATION include/strata
)
