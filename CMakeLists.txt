cmake_minimum_required (VERSION 2.6)
project (SQLiteCpp)

if(MSVC)
  include_directories ("${PROJECT_SOURCE_DIR}/sqlite3")
  add_library (sqlite3 sqlite3/sqlite3.c sqlite3/sqlite3.h) 
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  add_definitions(-rdynamic -fstack-protector-all -Wall -Wextra -pedantic -Weffc++ -Wformat-security -Winit-self -Wswitch-default -Wswitch-enum -Wfloat-equal -Wundef -Wshadow -Wcast-qual -Wconversion -Wlogical-op -Winline -Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn)
endif()

# add the wrapper as a library
add_library(SQLiteCpp
 src/SQLiteC++.h
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

# add the exemple1 executable, linked with the wrapper library
add_executable(example1 examples/example1/main.cpp)
target_link_libraries (example1 SQLiteCpp sqlite3)

# add a "test" target:
enable_testing()

# does the example1 runs successfully
add_test (Example1Run example1)
 

