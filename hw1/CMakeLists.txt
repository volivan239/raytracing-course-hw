project(hw1)
cmake_minimum_required(VERSION 3.16)

include_directories("src/include")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -O2 -fopenmp")
add_executable(main src/main.cpp src/point.cpp src/color.cpp src/scene.cpp src/sceneio.cpp src/quaternion.cpp src/primitives.cpp)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sample.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})