# Example building a SQLite3 loadable extension that uses SQLiteCpp internally
cmake_minimum_required(VERSION 3.1) # for "CMAKE_CXX_STANDARD" version
project(SQLiteCpp_ExampleExtension VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# KEY OPTION HERE: builds SQLiteCpp for use within the implementation of a loadable extension
set(SQLITECPP_IN_EXTENSION ON CACHE BOOL "" FORCE)

set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
set(SQLITECPP_USE_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
set(SQLITECPP_USE_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
add_subdirectory(../.. SQLiteCpp) # out-of-source build requires explicit subdir name for compilation artifacts

add_library(example SHARED src/example_extension.cpp)
target_link_libraries(example SQLiteCpp)

# Compile driver program that'll load the extension. It links sqlite3 statically, so our extension library
# mustn't itself link sqlite3, either statically or dynamically (that's one thing accomplished by
# SQLITECPP_IN_EXTENSION)
add_executable(example_driver src/main.cpp)
target_link_libraries(example_driver -static sqlite3 dl pthread)
