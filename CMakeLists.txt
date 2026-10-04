cmake_minimum_required(VERSION 4.3)
project(assembler C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(../../isa isa)
add_subdirectory(../asmcore asmcore)

add_executable(assembler main.c
        main.h)

target_link_libraries(assembler PUBLIC asmcore)