cmake_minimum_required(VERSION 3.5...4.0)
project(SQLiteCpp_Consumer LANGUAGES CXX)

set(SQLITECPP_CXX_STANDARD 11 CACHE STRING "C++ standard used to build SQLiteCpp")
set(CONSUMER_CXX_STANDARD 11 CACHE STRING "C++ standard used to build the consumer")

# Build SQLiteCpp with the requested standard.
set(CMAKE_CXX_STANDARD ${SQLITECPP_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SQLITECPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SQLITECPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
add_subdirectory(../.. SQLiteCpp)

# Build the consumer with its own standard.
add_executable(SQLiteCpp_Consumer main.cpp)
set_target_properties(SQLiteCpp_Consumer PROPERTIES
    CXX_STANDARD ${CONSUMER_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
)
target_compile_definitions(SQLiteCpp_Consumer PRIVATE
    SQLITECPP_CONSUMER_CXX_STANDARD=${CONSUMER_CXX_STANDARD}
)
target_link_libraries(SQLiteCpp_Consumer PRIVATE SQLiteCpp)

enable_testing()
add_test(NAME SQLiteCpp_Consumer COMMAND SQLiteCpp_Consumer)
