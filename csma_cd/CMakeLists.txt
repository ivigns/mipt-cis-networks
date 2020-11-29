cmake_minimum_required(VERSION 3.12)
project(csma-cd)

set(CMAKE_CXX_STANDARD 17)

add_executable(csma-cd main.cpp protocol.cpp utils.cpp logger.cpp)
