# Based on the FindSQLite3.cmake from the CMake project file to add support for
# SQLite3MultipleCiphers (https://github.com/utelle/SQLite3MultipleCiphers)
# to the SQLiteCpp (https://github.com/SRombauts/SQLiteCpp) project.
#
# Created in 2022 Jorrit Wronski (https://github.com/jowr)
#
# Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
# or copy at http://opensource.org/licenses/MIT)

#[=======================================================================[.rst:
FindSQLite3MultipleCiphers
--------------------------

Find the SQLite3MultipleCiphers library that implements SQLite3 with encryption.

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following `IMPORTED` targets:

``SQLite::SQLite3MultipleCiphers``
``SQLite::SQLite3``

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``SQLite3MultipleCiphers_INCLUDE_DIRS``
  where to find sqlite3mc.h, etc.
``SQLite3MultipleCiphers_LIBRARIES``
  the libraries to link against to use SQLite3MultipleCiphers.
``SQLite3MultipleCiphers_VERSION``
  SQLite3 version of the SQLite3MultipleCiphers library found
``SQLite3MultipleCiphers_FOUND``
  TRUE if found

It also sets alias variables for SQLite3 for campatibility reasons:

``SQLite3_INCLUDE_DIRS``
  where to find sqlite3.h, etc.
``SQLite3_LIBRARIES``
  the libraries to link against to use SQLite3.
``SQLite3_VERSION``
  version of the SQLite3 library found
``SQLite3_FOUND``
  TRUE if found
``SQLite::SQLite3``
  the ordinary SQLite3 target for linking

#]=======================================================================]

find_path(SQLite3MultipleCiphers_INCLUDE_DIR NAMES sqlite3mc.h
  HINTS
    ${SQLITE3MULTIPLECIPHERS_DIR}
    ENV SQLITE3MULTIPLECIPHERS_DIR
  PATH_SUFFIXES src
)
mark_as_advanced(SQLite3MultipleCiphers_INCLUDE_DIR)

#if("amd64" IN_LIST CMAKE_CPU_ARCHITECTURES) # I am targeting 64-bit x86.
if(CMAKE_SIZEOF_VOID_P MATCHES "8")
  set(SQLite3MultipleCiphers_LIB_SUFFIX "_x64")
else()
  set(SQLite3MultipleCiphers_LIB_SUFFIX "")
endif()

find_library(SQLite3MultipleCiphers_LIBRARY_RELEASE sqlite3mc${SQLite3MultipleCiphers_LIB_SUFFIX}
  HINTS
    ${SQLITE3MULTIPLECIPHERS_DIR}
    ENV SQLITE3MULTIPLECIPHERS_DIR
  PATH_SUFFIXES
    bin/vc13/lib/release
    bin/vc14/lib/release
	bin/vc15/lib/release
	bin/vc16/lib/release
	bin/vc17/lib/release
	bin/gcc/lib/release
	bin/clang/lib/release
	bin/lib/release
  REQUIRED
)
mark_as_advanced(SQLite3MultipleCiphers_LIBRARY_RELEASE)

find_library(SQLite3MultipleCiphers_LIBRARY_DEBUG sqlite3mc${SQLite3MultipleCiphers_LIB_SUFFIX}
  HINTS
    ${SQLITE3MULTIPLECIPHERS_DIR}
    ENV SQLITE3MULTIPLECIPHERS_DIR
  PATH_SUFFIXES
    bin/vc13/lib/debug
    bin/vc14/lib/debug
	bin/vc15/lib/debug
	bin/vc16/lib/debug
	bin/vc17/lib/debug
	bin/gcc/lib/debug
	bin/clang/lib/debug
	bin/lib/debug
)
mark_as_advanced(SQLite3MultipleCiphers_LIBRARY_DEBUG)

find_library(SQLite3MultipleCiphers_LIBRARY_MINSIZEREL sqlite3mc${SQLite3MultipleCiphers_LIB_SUFFIX}
  HINTS
    ${SQLITE3MULTIPLECIPHERS_DIR}
    ENV SQLITE3MULTIPLECIPHERS_DIR
  PATH_SUFFIXES 
    bin/vc13/lib/minsizerel
    bin/vc14/lib/minsizerel
	bin/vc15/lib/minsizerel
	bin/vc16/lib/minsizerel
	bin/vc17/lib/minsizerel
	bin/gcc/lib/minsizerel
	bin/clang/lib/minsizerel
	bin/lib/minsizerel
)
mark_as_advanced(SQLite3MultipleCiphers_LIBRARY_MINSIZEREL)

find_library(SQLite3MultipleCiphers_LIBRARY_RELWITHDEBINFO sqlite3mc${SQLite3MultipleCiphers_LIB_SUFFIX}
  HINTS
    ${SQLITE3MULTIPLECIPHERS_DIR}
    ENV SQLITE3MULTIPLECIPHERS_DIR
  PATH_SUFFIXES
    bin/vc13/lib/relwithdebinfo
    bin/vc14/lib/relwithdebinfo
	bin/vc15/lib/relwithdebinfo
	bin/vc16/lib/relwithdebinfo
	bin/vc17/lib/relwithdebinfo
	bin/gcc/lib/relwithdebinfo
	bin/clang/lib/relwithdebinfo
	bin/lib/relwithdebinfo
)
mark_as_advanced(SQLite3MultipleCiphers_LIBRARY_RELWITHDEBINFO)

# Extract version information from the header file
if(SQLite3MultipleCiphers_INCLUDE_DIR)
    file(STRINGS ${SQLite3MultipleCiphers_INCLUDE_DIR}/sqlite3mc_version.h _ver_line
         REGEX "^#define SQLITE3MC_VERSION_STRING  *\"[0-9]+\\.[0-9]+\\.[0-9]+\""
         LIMIT_COUNT 1)
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+"
           SQLite3MultipleCiphers_VERSION "${_ver_line}")
    unset(_ver_line)
	mark_as_advanced(SQLite3MultipleCiphers_VERSION)
endif()

#include(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SQLite3MultipleCiphers
  REQUIRED_VARS SQLite3MultipleCiphers_INCLUDE_DIR SQLite3MultipleCiphers_LIBRARY_RELEASE SQLite3MultipleCiphers_LIBRARY_DEBUG
  VERSION_VAR SQLite3MultipleCiphers_VERSION)

# Create the imported target
if(SQLite3MultipleCiphers_FOUND)
  set(SQLite3MultipleCiphers_INCLUDE_DIRS ${SQLite3MultipleCiphers_INCLUDE_DIR})
  set(SQLite3MultipleCiphers_LIBRARIES 
    debug ${SQLite3MultipleCiphers_LIBRARY_DEBUG}
	optimized ${SQLite3MultipleCiphers_LIBRARY_RELEASE})
  if(NOT TARGET SQLite::SQLite3MultipleCiphers)
    add_library(SQLite::SQLite3MultipleCiphers UNKNOWN IMPORTED)
    set_target_properties(SQLite::SQLite3MultipleCiphers PROPERTIES
      IMPORTED_LOCATION_DEBUG          "${SQLite3MultipleCiphers_LIBRARY_DEBUG}"
      IMPORTED_LOCATION_RELEASE        "${SQLite3MultipleCiphers_LIBRARY_RELEASE}"
      IMPORTED_LOCATION_MINSIZEREL     "${SQLite3MultipleCiphers_LIBRARY_MINSIZEREL}"
      IMPORTED_LOCATION_RELWITHDEBINFO "${SQLite3MultipleCiphers_LIBRARY_RELWITHDEBINFO}"
      INTERFACE_INCLUDE_DIRECTORIES    "${SQLite3MultipleCiphers_INCLUDE_DIR}")
  endif()
endif()

# define the alias variables
if(NOT SQLite3_INCLUDE_DIRS AND NOT SQLite3_LIBRARIES AND NOT SQLite3_VERSION AND NOT SQLite3_FOUND AND NOT TARGET SQLite::SQLite3)
  set(SQLite3_INCLUDE_DIRS ${SQLite3MultipleCiphers_INCLUDE_DIRS})
  mark_as_advanced(SQLite3_INCLUDE_DIRS)
  set(SQLite3_LIBRARIES ${SQLite3MultipleCiphers_LIBRARIES})
  mark_as_advanced(SQLite3_LIBRARIES)
  #set(SQLite3_VERSION ${SQLite3MultipleCiphers_VERSION})
  #mark_as_advanced(SQLite3_VERSION)
  file(STRINGS ${SQLite3MultipleCiphers_INCLUDE_DIR}/sqlite3.h _ver_line
       REGEX "^#define SQLITE_VERSION  *\"[0-9]+\\.[0-9]+\\.[0-9]+\""
       LIMIT_COUNT 1)
  string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+"
         SQLite3_VERSION "${_ver_line}")
  unset(_ver_line)
  mark_as_advanced(SQLite3_VERSION)
  set(SQLite3_FOUND ${SQLite3MultipleCiphers_FOUND})
  mark_as_advanced(SQLite3_FOUND)
  add_library(SQLite::SQLite3 ALIAS SQLite::SQLite3MultipleCiphers)
else()
  message(FATAL_ERROR "SQLite3 has already been defined, please check you options. The current settings are very likely to produce conflicts.")
endif()
