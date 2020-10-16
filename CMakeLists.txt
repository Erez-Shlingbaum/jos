cmake_minimum_required(VERSION 3.15)
project("jos")
set(CMAKE_SYSTEM_PROCESSOR i686)
ENABLE_LANGUAGE(ASM)

add_executable(main kern/init.c inc kern lib)

add_compile_definitions(JOS_USER)
add_executable(userStuff fs/fs.c fs/bc.c fs/ide.c fs/serv.c user/sh.c net net/output.c net/lwip net/lwip/include net/lwip/include/lwip net/lwip/include/lwip/sockets.h net/lwip/include/ipv4/lwip/inet.h user user/httpd.c inc lib)

include_directories(.)
include_directories(net/lwip/include net/lwip/include/ipv4 net/lwip/include/ipv6 net/lwip/include/netif)
include_directories(.)