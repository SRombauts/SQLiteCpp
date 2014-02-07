# Main CMake file for compiling the library itself, examples and tests.
#
# Copyright (c) 2012-2014 Sebastien Rombauts (sebastien.rombauts@gmail.com)
#
# Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
# or copy at http://opensource.org/licenses/MIT) 

cmake_minimum_required(VERSION 2.6)
project(SQLiteCpp)

# Enable the use of SQLite column metadata and Column::getName() method,
# Require that the sqlite3 library is also compiled with this flag (default under Debian/Ubuntu).
option(SQLITE_ENABLE_COLUMN_METADATA "Enable Column::getName(). Require support from sqlite3 library." ON)
if (SQLITE_ENABLE_COLUMN_METADATA) 
    add_definitions(-DSQLITE_ENABLE_COLUMN_METADATA)
endif ()

option(SQLITE_ENABLE_ASSERT_HANDLER "Enable the user defintion of a assertion_failed() handler." ON)
if (SQLITE_ENABLE_ASSERT_HANDLER) 
    # Enable the user defintion of a assertion_failed() handler.
    add_definitions(-DSQLITECPP_ENABLE_ASSERT_HANDLER)
endif ()

# Define useful variables to handle OS/Compiler differences
if (MSVC)
    set(CPPLINT_ARG_OUTPUT      "--output=vs7")
    set(CPPCHECK_ARG_TEMPLATE   "--template=vs")
    set(DEV_NULL                "NUL")
    # build the SQLite3 C library for Windows (for ease of use)
    add_subdirectory(sqlite3) 
    include_directories("${PROJECT_SOURCE_DIR}/sqlite3")
    # disable Visual Studio warnings for fopen() used in the example
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
elseif (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CPPLINT_ARG_OUTPUT      "--output=eclipse")
    set(CPPCHECK_ARG_TEMPLATE   "--template=gcc")
    set(DEV_NULL                "/dev/null")
    # GCC flags
    add_definitions(-rdynamic -fstack-protector-all -Wall -Wextra -pedantic -Weffc++ -Wformat-security -Winit-self -Wswitch-default -Wswitch-enum -Wfloat-equal -Wundef -Wshadow -Wcast-qual -Wconversion -Wlogical-op -Winline -Wsuggest-attribute=pure -Wsuggest-attribute=const)
endif ()
set(CPPLINT_ARG_VERBOSE "--verbose=3")
set(CPPLINT_ARG_LINELENGTH "--linelength=120")


## Core source code ##

# adding a new file require explicittly modifing the CMakeLists.txt
# so that CMake knows that it should rebuild the project (it is best practice)

# list of sources files of the library
set(SQLITECPP_SRC
 src/SQLiteC++.h
 src/Assertion.h
 src/Column.cpp
 src/Column.h
 src/Database.cpp
 src/Database.h
 src/Exception.h
 src/Statement.cpp
 src/Statement.h
 src/Transaction.cpp
 src/Transaction.h
)
# add sources of the wrapper as a "SQLiteCpp" static library
add_library (SQLiteCpp  ${SQLITECPP_SRC})
source_group(src FILES ${SQLITECPP_SRC})

if(UNIX AND (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
    set_target_properties(SQLiteCpp PROPERTIES COMPILE_FLAGS "-fPIC")
endif(UNIX AND (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))


# Optional additional targets:

option(SQLITECPP_RUN_CPPLINT "Run cpplint.py tool for Google C++ StyleGuide." OFF)
if (SQLITECPP_RUN_CPPLINT)
    # add a cpplint target to the "all" target
    add_custom_target(SQLiteCpp_cpplint
     ALL
     COMMAND python cpplint.py ${CPPLINT_ARG_OUTPUT} ${CPPLINT_ARG_VERBOSE} ${CPPLINT_ARG_LINELENGTH} ${PROJECT_SOURCE_DIR}/${SQLITECPP_SRC}
    )
endif()

option(SQLITECPP_RUN_CPPCHECK "Run cppcheck C++ static analysis tool." ON)
if (SQLITECPP_RUN_CPPCHECK)
    # add a cppcheck target to the "all" target
    add_custom_target(SQLiteCpp_cppcheck
     ALL
     COMMAND cppcheck -j 4 cppcheck --enable=style --quiet ${CPPCHECK_ARG_TEMPLATE} ${PROJECT_SOURCE_DIR}/src
    )
endif()

option(SQLITECPP_RUN_DOXYGEN "Run Doxygen C++ documentation tool." ON)
if (SQLITECPP_RUN_DOXYGEN)
    # add a Doxygen target to the "all" target
    add_custom_target(SQLiteCpp_doxygen
     ALL
     COMMAND doxygen Doxyfile > ${DEV_NULL}
     WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
endif()

option(SQLITECPP_RUN_TESTS "Run test tools." ON)
if (SQLITECPP_RUN_TESTS)
    # add the example1 executable, linked with the wrapper library
    add_executable(example1 examples/example1/main.cpp)
    target_link_libraries(example1 SQLiteCpp sqlite3)

    # add a "test" target:
    enable_testing()

    # does the example1 runs successfully?
    add_test(Example1Run example1)
endif()
