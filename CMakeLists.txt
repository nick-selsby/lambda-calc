cmake_minimum_required(VERSION 3.10)
project(lambda-calc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_executable(
    lambda
    src/main.cpp
    src/expr.cpp
    src/interp.cpp
)

target_include_directories(
    lambda
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)