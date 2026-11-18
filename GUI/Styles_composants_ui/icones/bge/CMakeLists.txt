cmake_minimum_required(VERSION 3.16)
project(BGE VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(bge INTERFACE)                       # BGE est « header-only » dans ce prototype
target_include_directories(bge INTERFACE include)
add_executable(desktop_demo examples/desktop_demo.cpp)
target_link_libraries(desktop_demo bge)
# Régénérer les icônes depuis ../baura-icons.js :  python3 tools/gen_icons.py
