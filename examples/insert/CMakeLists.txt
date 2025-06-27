cmake_minimum_required(VERSION 3.16)
project(inserts CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(inserts
    insert.cpp
)

# Add include directories (e.g., your headers)
target_include_directories(inserts
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(inserts
    PRIVATE
    pq
    pqxx
    strata
)

target_compile_options(inserts PRIVATE -Wall -Wextra -pedantic)
