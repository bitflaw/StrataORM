cmake_minimum_required(VERSION 3.16)
project(query CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(query
    queries.cpp
)

# Add include directories (e.g., your headers)
target_include_directories(query
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(query
    PRIVATE
    pq
    pqxx
    strata
)

target_compile_options(query PRIVATE -Wall -Wextra -pedantic)
