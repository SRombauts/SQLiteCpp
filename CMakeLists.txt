cmake_minimum_required (VERSION 2.6)
project (SQLiteCpp)
add_definitions(-rdynamic -fstack-protector-all -Wall -Wextra -pedantic -Weffc++ -Wformat-security -Winit-self -Wswitch-default -Wswitch-enum -Wfloat-equal -Wundef -Wshadow -Wcast-qual -Wconversion -Wlogical-op -Winline -Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn)

# add the wrapper as a library
add_library(SQLiteCpp
 src/Column.cpp
 src/Database.cpp
 src/Statement.cpp
 src/Transaction.cpp
)

# add the exmple1 executable, linked with the wrapper library
add_executable(example1 examples/example1/main.cpp)
target_link_libraries (example1 SQLiteCpp sqlite3)

# add a "test" target:
enable_testing()

# does the example1 runs successfully
add_test (Example1Run example1)
 

