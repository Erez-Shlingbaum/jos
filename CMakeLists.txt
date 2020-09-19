cmake_minimum_required(VERSION 3.15)
project("jos")
set(CMAKE_SYSTEM_PROCESSOR i686)
ENABLE_LANGUAGE(ASM)

add_executable(main kern/init.c kern lib)
include_directories(.)