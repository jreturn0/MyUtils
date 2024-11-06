cmake_minimum_required(VERSION 3.5.0)
project(MyUtils VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

file(GLOB SOURCES "src/*.cpp")

add_library(MyUtils ${SOURCES})
target_include_directories(MyUtils PUBLIC include)

include(CTest)
enable_testing()

#cmake_minimum_required(VERSION 3.12)
#project(MyUtilsTest)
#
## Add the path to your built library (update the path if necessary)
#add_subdirectory(path/to/MyUtils)
#
## Create the test executable
#add_executable(MyUtilsTest main.cpp)  # Replace with your actual test file
#target_link_libraries(MyUtilsTest PRIVATE MyUtils)