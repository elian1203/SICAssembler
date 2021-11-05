cmake_minimum_required(VERSION 3.20)
project(sicasm C)

set(CMAKE_C_STANDARD 99)

add_executable(sicasm main.c symbols.c instructions.c scoff.c)
add_custom_target(makefile)
