cmake_minimum_required(VERSION 3.15)
project("jos")
set(CMAKE_SYSTEM_PROCESSOR i686)
ENABLE_LANGUAGE(ASM)

add_executable(main kern/init.c inc kern lib)

add_compile_definitions(JOS_USER)
add_executable(fs fs/fs.c fs/bc.c fs/ide.c fs/serv.c user/sh.c  inc lib)

include_directories(.)