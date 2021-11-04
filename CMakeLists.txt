cmake_minimum_required(VERSION 3.20)
project(cop3404p2 C)

set(CMAKE_C_STANDARD 99)

add_executable(cop3404p2 main.c symbols.c instructions.c scoff.c)
add_custom_target(makefile)
