cmake_minimum_required(VERSION 3.5.0)
project(MyUtils VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

file(GLOB SOURCES "src/*.cpp")

add_library(MyUtils ${SOURCES})
target_include_directories(MyUtils PUBLIC include)

include(CTest)
enable_testing()
