cmake_minimum_required(VERSION 3.16)
project(KRON-1-assembler C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(../../KRON-1-isa isa)
add_subdirectory(../KRON-1-asmcore asmcore)

add_executable(KRON-1-assembler main.c
        main.h)

target_link_libraries(KRON-1-assembler PUBLIC KRON-1-asmcore)