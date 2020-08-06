cmake_minimum_required(VERSION 3.15)
project(jos)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i386)

ENABLE_LANGUAGE(ASM)


add_executable(jos
        boot/main.c
        kern/init.c
        )
include_directories(.)