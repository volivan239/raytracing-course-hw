project(hw1)
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
add_executable(main src/main.cpp src/color.cpp src/scene.cpp src/sceneio.cpp src/primitives.cpp)
target_include_directories(main PUBLIC ../rapidjson/include src/include)

find_package(OpenMP)
target_link_libraries(main PUBLIC OpenMP::OpenMP_CXX)
