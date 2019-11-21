cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER "mpicc")
set(CMAKE_CXX_COMPILER "mpic++")

project(lab01)

include_directories(
    project/include
)

add_executable(lab01
    project/src/main.c
    project/src/knapsack.c
)
