# Main CMake file compiling the library itself, examples and tests.
#
# Copyright (c) 2012-2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
#
# Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
# or copy at http://opensource.org/licenses/MIT) 

cmake_minimum_required (VERSION 2.6)
project (SQLiteCpp)

# Enable the use of SQLite column metadata and Column::getName() method,
# Require that the sqlite3 library is also compiled with this flag.
option (SQLITE_ENABLE_COLUMN_METADATA 
        "Enable Column::getName(). Require support from sqlite3 library." OFF)
if (SQLITE_ENABLE_COLUMN_METADATA) 
  add_definitions(-DSQLITE_ENABLE_COLUMN_METADATA)
endif()

# Enable the user defintion of a assertion_failed() handler.
add_definitions(-DSQLITECPP_ENABLE_ASSERT_HANDLER)

if (MSVC)
  # build the SQLite3 C library for Windows (for ease of use)
  add_subdirectory (sqlite3) 
  include_directories ("${PROJECT_SOURCE_DIR}/sqlite3")
  # disable Visual Studio warnings for fopen() used in the example
  add_definitions (/D_CRT_SECURE_NO_WARNINGS)
elseif (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # GCC flags 
  add_definitions (-rdynamic -fstack-protector-all -Wall -Wextra -pedantic -Weffc++ -Wformat-security -Winit-self -Wswitch-default -Wswitch-enum -Wfloat-equal -Wundef -Wshadow -Wcast-qual -Wconversion -Wlogical-op -Winline -Wsuggest-attribute=pure -Wsuggest-attribute=const)
endif ()

# add a cppcheck target to the "all" target
add_custom_target(cppcheck
 ALL
 COMMAND cppcheck -j 4 cppcheck --enable=style,portability,performance,information --quiet --template='{file}:{line}: warning: cppcheck: {message} [{severity}/{id}]' src
 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}  
)

################################################################################
# add the subdirectory containing the CMakeLists.txt of the wrapper library
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
