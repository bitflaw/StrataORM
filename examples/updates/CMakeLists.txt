cmake_minimum_required(VERSION 3.16)
project(update CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(update
    main.cpp
)

# Add include directories (e.g., your headers)
target_include_directories(update
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(update
    PRIVATE
    pq
    pqxx
    strata
)

target_compile_options(update PRIVATE -Wall -Wextra -Wpedantic)
