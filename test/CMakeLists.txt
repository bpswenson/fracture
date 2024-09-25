cmake_minimum_required(VERSION 3.10)
project(test_lib)

# Set CMake module path to include custom scripts
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/src/revy/cmake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")


# Include the custom reverse pass macro
include(add_reverse_test)

set(REVERSE_PASS_LIB "${CMAKE_BINARY_DIR}/lib/libreverse_pass${CMAKE_SHARED_LIBRARY_SUFFIX}")

set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_funcs.cpp
)

# Now use the macro to add the test
add_reverse_test(test_lib ${SOURCE_FILES})
