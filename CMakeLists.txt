cmake_minimum_required(VERSION 3.10)
project(ViewVolcano)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED) 
find_package(GLEW REQUIRED)

add_executable(view_volcano main.cpp)

target_link_libraries(view_volcano OpenGL::GL glfw GLEW::GLEW)