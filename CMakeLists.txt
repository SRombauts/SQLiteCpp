cmake_minimum_required (VERSION 2.6)
project (SQLiteCpp)
add_definitions(-rdynamic -fstack-protector-all -Wall -Wextra -pedantic -Weffc++ -Wformat-security -Winit-self -Wswitch-default -Wswitch-enum -Wfloat-equal -Wundef -Wshadow -Wcast-qual -Wconversion -Wlogical-op -Winline -Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn)
add_library(SQLiteCpp
 src/Column.cpp
 src/Database.cpp
 src/Statement.cpp
 src/Transaction.cpp
)
add_executable(example1 examples/example1/main.cpp)
target_link_libraries (example1 SQLiteCpp sqlite3)

