project(hw1)
cmake_minimum_required(VERSION 3.16)

include_directories("src/include")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra")
add_executable(main src/main.cpp src/color.cpp src/scene.cpp src/sceneio.cpp src/primitives.cpp)