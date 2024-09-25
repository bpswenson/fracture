cmake_minimum_required(VERSION 3.13)
project(ReversePass)

# Find the LLVM package
find_package(LLVM REQUIRED CONFIG)

# Include the LLVM directories and definitions
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Add your pass plugin as a shared library
add_library(reverse_pass SHARED revy.cpp)

# Dynamically link your pass plugin against the required LLVM components
target_link_libraries(reverse_pass PRIVATE LLVM)

# Ensure LLVM is linked dynamically
target_link_options(reverse_pass PRIVATE "-Wl,--no-undefined")

install(TARGETS reverse_pass LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

# Print some useful information about the LLVM setup
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

