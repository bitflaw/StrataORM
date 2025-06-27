cmake_minimum_required(VERSION 3.16)
project(models CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(models
    models.cpp
)

target_link_libraries(models
    PRIVATE
    pq
    pqxx
    strata
)

target_compile_options(models PRIVATE -Wall -Wextra -pedantic)
