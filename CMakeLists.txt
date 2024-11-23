# This is a template for a CMake project.
cmake_minimum_required(VERSION 3.15)

project(cmake_template)

include(CMake_cfg.cmake)
include(CMake_pkg.cmake)

add_subdirectory(core)
add_subdirectory(gui) 
add_subdirectory(test) 