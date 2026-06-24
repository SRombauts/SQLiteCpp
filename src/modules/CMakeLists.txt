file(GLOB_RECURSE SQLITECPP_MODULES *.cppm)

add_library(sqlitecpp_modules)

cmake_minimum_required(VERSION 3.28)

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

configure_cpp_module_target(sqlitecpp_modules)

target_link_libraries(sqlitecpp_modules
  PUBLIC
    sqlitecpp
)

target_include_directories(sqlitecpp_modules
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(sqlitecpp_modules PUBLIC cxx_std_20)