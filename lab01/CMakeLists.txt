cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER "mpicc")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")

project(lab01)

include_directories(
    project/include
)

add_executable(lab01
    project/src/main.c
    project/src/knapsack.c
)
