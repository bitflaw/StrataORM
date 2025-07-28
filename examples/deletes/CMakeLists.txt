cmake_minimum_required(VERSION 3.16)
project(delete CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(delete
    main.cpp
)

# Add include directories (e.g., your headers)
target_include_directories(delete
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(delete
    PRIVATE
    pq
    pqxx
    strata
)

target_compile_options(delete PRIVATE -Wall -Wextra -Wpedantic)
