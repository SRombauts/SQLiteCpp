# Main CMake file compiling the library itself, examples and tests.
#
# Copyright (c) 2012-2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
#
# Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
# or copy at http://opensource.org/licenses/MIT) 

cmake_minimum_required (VERSION 2.6)
project (SQLiteCpp)

if (MSVC)
  # build the SQLite3 C library for windows build (for ease of use)
  include_directories ("${PROJECT_SOURCE_DIR}/sqlite3")
  add_library (sqlite3 sqlite3/sqlite3.c sqlite3/sqlite3.h) 
  # disable Visual Studio warnings for fopen() used in the example
  add_definitions (/D_CRT_SECURE_NO_WARNINGS)
elseif (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # GCC flags 
  add_definitions (-rdynamic -fstack-protector-all -Wall -Wextra -pedantic -Weffc++ -Wformat-security -Winit-self -Wswitch-default -Wswitch-enum -Wfloat-equal -Wundef -Wshadow -Wcast-qual -Wconversion -Wlogical-op -Winline -Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn)
endif ()

################################################################################
# add the subdirectory containing the CMakeLists.txt of the library
add_subdirectory (src) 
include_directories ("${PROJECT_SOURCE_DIR}/src")
################################################################################

# add the example1 executable, linked with the wrapper library
add_executable (example1 examples/example1/main.cpp)
target_link_libraries (example1 SQLiteCpp sqlite3)

# add a "test" target:
enable_testing ()

# does the example1 runs successfully
add_test (Example1Run example1)
