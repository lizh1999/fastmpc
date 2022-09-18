cmake_minimum_required(VERSION 3.16)

project(fastmpc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(.)

add_library(fastmpc
  fastmpc/net.cc
)
