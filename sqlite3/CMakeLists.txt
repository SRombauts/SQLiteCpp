# CMake file for compiling the sqlite3 static library under Windows (for ease of use)
#
# Copyright (c) 2012-2016 Sebastien Rombauts (sebastien.rombauts@gmail.com)
#
# Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
# or copy at http://opensource.org/licenses/MIT) 

# add sources of the "sqlite3" static library
add_library(sqlite3
 sqlite3.c
 sqlite3.h
)

if (SQLITE_ENABLE_COLUMN_METADATA)
    # Enable the use of SQLite column metadata method
    # Require that the sqlite3 library is also compiled with this flag:
    target_compile_definitions(sqlite3 PUBLIC SQLITE_ENABLE_COLUMN_METADATA)
endif (SQLITE_ENABLE_COLUMN_METADATA)

if (UNIX AND (CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang"))
    set_target_properties(sqlite3 PROPERTIES COMPILE_FLAGS "-fPIC")
endif (UNIX AND (CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang"))

if (UNIX AND CMAKE_COMPILER_IS_GNUCXX)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
        target_compile_options(sqlite3 PRIVATE "-Wimplicit-fallthrough=0")
    endif()
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8.0)
        target_compile_options(sqlite3 PRIVATE "-Wno-cast-function-type")
    endif()
endif()
