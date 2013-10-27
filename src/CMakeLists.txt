# CMake file for compiling the wrapper static library itself.
#
# This allow for easy use of SQLiteC++ as a Git submodule;
# simply use add_subdirectory (SQLiteCpp/src) into your main CMakeLists.txt file,
# and link to the "SQLiteCpp" library
#
# Copyright (c) 2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
#
# Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
# or copy at http://opensource.org/licenses/MIT) 

# add sources of the wrapper as a "SQLiteCpp" static library
add_library (SQLiteCpp
 SQLiteC++.h
 Column.cpp
 Column.h
 Database.cpp
 Database.h
 Exception.h
 Statement.cpp
 Statement.h
 Transaction.cpp
 Transaction.h
)
if(LINUX AND (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
    set_target_properties(SQLiteCpp PROPERTIES COMPILE_FLAGS "-fPIC")
endif(LINUX AND (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
