cmake_minimum_required(VERSION 3.15)
project(lab5_part1 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-DMODE=1")
add_executable(server server.c)
add_executable(client client.c)