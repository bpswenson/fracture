cmake_minimum_required(VERSION 3.10)
project(fracture)

# Find LLVM 19
find_package(LLVM REQUIRED CONFIG)

# Set up include and link directories for LLVM
include_directories(${LLVM_INCLUDE_DIRS} .)
link_directories(${LLVM_LIBRARY_DIRS})

# Add the fracture simulator source files
add_library(fracture STATIC simulator.cpp)

# Ensure reverse pass is built before the simulator
add_dependencies(fracture reverse_pass)
