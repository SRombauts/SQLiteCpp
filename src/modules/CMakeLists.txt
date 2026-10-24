cmake_minimum_required(VERSION 3.28)

add_library(sqlitecpp_module)

target_sources(sqlitecpp_module
    PUBLIC
        FILE_SET CXX_MODULES FILES
            SQLiteCpp.cppm
)

if(NOT COMMAND configure_cpp_module_target)
  function(configure_cpp_module_target target)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
      target_sources(${target} PUBLIC FILE_SET CXX_MODULES FILES ${SQLITECPP_MODULES})
    else()
      message(WARNING "C++ modules require CMake 3.28+. Using standard compilation.")
      target_sources(${target} PRIVATE ${SQLITECPP_MODULES})
    endif()
  endfunction()
endif()

configure_cpp_module_target(sqlitecpp_module)

target_link_libraries(sqlitecpp_module
  PUBLIC
    sqlitecpp
)

target_include_directories(sqlitecpp_module
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(sqlitecpp_module PUBLIC cxx_std_20)


add_library(sqlitecpp::module ALIAS sqlitecpp_module)