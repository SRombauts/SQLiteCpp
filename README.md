SQLiteC++
---------

[![release](https://img.shields.io/github/release/SRombauts/SQLiteCpp.svg)](https://github.com/SRombauts/SQLiteCpp/releases)
[![license](https://img.shields.io/github/license/SRombauts/SQLiteCpp.svg)](https://github.com/SRombauts/SQLiteCpp/blob/master/LICENSE.txt)
[![Travis CI Linux Build Status](https://travis-ci.org/SRombauts/SQLiteCpp.svg)](https://travis-ci.org/SRombauts/SQLiteCpp "Travis CI Linux Build Status")
[![AppVeyor Windows Build status](https://ci.appveyor.com/api/projects/status/github/SRombauts/SQLiteCpp?svg=true)](https://ci.appveyor.com/project/SbastienRombauts/SQLiteCpp "AppVeyor Windows Build status")
[![Join the chat at https://gitter.im/SRombauts/SQLiteCpp](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/SRombauts/SQLiteCpp?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

SQLiteC++ (SQLiteCpp) is a smart and easy to use C++ SQLite3 wrapper.

See SQLiteC++ website http://srombauts.github.com/SQLiteCpp on GitHub.

Keywords: sqlite, sqlite3, C, library, wrapper C++

## About SQLiteC++:

SQLiteC++ offers an encapsulation arround the native C APIs of SQLite,
with a few intuitive and well documented C++ class.

### License:

Copyright (c) 2012-2016 Sébastien Rombauts (sebastien.rombauts@gmail.com)

Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
or copy at http://opensource.org/licenses/MIT)

#### Note on redistribution of SQLite source files

As stated by the MIT License, you are welcome to reuse, modify, and redistribute the SQLiteCpp source code
the way you want it to, be it a git submodule, a subdirectory, or a selection of some source files.

I would love a mention in your README, a web link to the SQLite repository, and a mention of the author,
but none of those are mandatory.

### About SQLite underlying library:

SQLite is a library that implements a serverless transactional SQL database engine.
It is the most widely deployed SQL database engine in the world.
All of the code and documentation in SQLite has been dedicated to the public domain by the authors.
http://www.sqlite.org/about.html

### The goals of SQLiteC++ are:

- to offer the best of existing simple C++ SQLite wrappers
- to be elegantly written with good C++ design, STL, exceptions and RAII idiom
- to keep dependencies to a minimum (STL and SQLite3)
- to be portable
- to be light and fast
- to be thread-safe only as much as SQLite "Multi-thread" mode (see below)
- to have a good unit test coverage
- to use API names sticking with those of the SQLite library
- to be well documented with Doxygen tags, and with some good examples
- to be well maintained
- to use a permissive MIT license, similar to BSD or Boost, for proprietary/commercial usage

It is designed using the Resource Acquisition Is Initialization (RAII) idom
(see http://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization),
and throwing exceptions in case of SQLite errors (exept in destructors,
where assert() are used instead).
Each SQLiteC++ object must be constructed with a valid SQLite database connection,
and then is always valid until destroyed.

### Supported platforms:

Developements and tests are done under the following OSs:
- Ubuntu 12.04 (Travis CI) and 14.04
- Debian 7
- Windows XP/10
And following IDEs/Compilers
- GCC 4.6.3, 4.7.2 and GCC 4.8.2
- Clang 3.4
- Eclipse CDT under Linux
- Visual Studio Express 2008 and Community 2015

### Dependencies

- a STL implementation (even an old one, like the one provided with VC6 should work)
- exception support (the class Exception inherit from std::runtime_error)
- the SQLite library, either by linking to it dynamicaly or staticaly (install the libsqlite3-dev package under Debian/Ubuntu/Mint Linux),
  or by adding its source file in your project code base (source code provided in src/sqlite3 for Windows),
  with the SQLITE_ENABLE_COLUMN_METADATA macro defined (see http://www.sqlite.org/compile.html#enable_column_metadata).

## Getting started
### Installation

To use this wrappers, you need to add the 10 SQLiteC++ source files from the src/ directory
in your project code base, and compile/link against the sqlite library.

The easiest way to do this is to add the wrapper as a library.
The proper "CMakeLists.txt" file defining the static library is provided in the src/ subdirectory,
so you simply have to add_directory(SQLiteCpp/src) to you main CMakeLists.txt
and link to the "SQLiteCpp" wrapper library.
Thus this SQLiteCpp repository can directly be used as a Git submoldule.

Under Debian/Ubuntu/Mint Linux, install the libsqlite3-dev package.

### Building examples and unit-tests:

Use git to clone the repository. Then init and update submodule "googletest".

```Shell
git clone https://github.com/SRombauts/SQLiteCpp.git
cd SQLiteCpp
git submodule init
git submodule update
```

#### CMake and tests
A CMake configuration file is also provided for multiplatform support and testing.

Typical generic build for MS Visual Studio under Windows (from [build.bat](build.bat)):

```Batchfile
mkdir build
cd build

cmake ..        # cmake .. -G "Visual Studio 10"    # for Visual Studio 2010
@REM Generate a Visual Studio solution for latest version found
cmake -DSQLITECPP_BUILD_EXAMPLES=ON -DSQLITECPP_BUILD_TESTS=ON ..

@REM Build default configuration (ie 'Debug')
cmake --build .

@REM Build and run tests
ctest --output-on-failure
```

Generating the Linux Makefile, building in Debug and executing the tests (from [build.sh](build.sh)):

```Shell
mkdir Debug
cd Debug

# Generate a Makefile for GCC (or Clang, depanding on CC/CXX envvar)
cmake -DSQLITECPP_BUILD_EXAMPLES=ON -DSQLITECPP_BUILD_TESTS=ON ..

# Build (ie 'make')
cmake --build .

# Build and run unit-tests (ie 'make test')
ctest --output-on-failure
```

#### CMake options

  * For more options on customizing the build, see the [CMakeLists.txt](https://github.com/SRombauts/SQLiteCpp/blob/master/CMakeLists.txt) file.

#### Troubleshooting

Under Linux, if you get muliple linker errors like "undefined reference to sqlite3_xxx",
it's that you lack the "sqlite3" library: install the libsqlite3-dev package.

If you get a single linker error "Column.cpp: undefined reference to sqlite3_column_origin_name",
it's that your "sqlite3" library was not compiled with
the SQLITE_ENABLE_COLUMN_METADATA macro defined (see http://www.sqlite.org/compile.html#enable_column_metadata).
You can either recompile it yourself (seek help online) or you can comment out the following line in src/Column.h:

```C++
#define SQLITE_ENABLE_COLUMN_METADATA
```

### Continuous Integration

This project is continuously tested under Ubuntu Linux with the gcc and clang compilers
using the Travis CI community service with the above CMake building and testing procedure.
It is also tested in the same way under Windows Server 2012 R2 with Visual Studio 2013 compiler
using the AppVeyor countinuous integration service.

Detailed results can be seen online:
 - https://travis-ci.org/SRombauts/SQLiteCpp
 - https://ci.appveyor.com/project/SbastienRombauts/SQLiteCpp

### Thread-safety

SQLite supports three mode of thread safety, as describe in "SQLite And Multiple Threads" :
see http://www.sqlite.org/threadsafe.html

This SQLiteC++ wrapper does no add any lock (no mutexes) nor any other thread-safety mecanism
above the SQLite library itself, by design, for lightness and speed.

Thus, SQLiteC++ naturally supports the "Multi Thread" mode of SQLite ;
"In this mode, SQLite can be safely used by multiple threads
provided that no single database connection is used simultaneously in two or more threads."

But SQLiteC++ does not support the fully thread-safe "Serialized" mode of SQLite,
because of the way it shares the underling SQLite precompiled statement
in a custom shared pointer (See the inner class "Statement::Ptr").

## Examples
### First sample demonstrates how to query a database and get results: 

```C++
try
{
    // Open a database file
    SQLite::Database    db("example.db3");
    
    // Compile a SQL query, containing one parameter (index 1)
    SQLite::Statement   query(db, "SELECT * FROM test WHERE size > ?");
    
    // Bind the integer value 6 to the first parameter of the SQL query
    query.bind(1, 6);
    
    // Loop to execute the query step by step, to get rows of result
    while (query.executeStep())
    {
        // Demonstrate how to get some typed column value
        int         id      = query.getColumn(0);
        const char* value   = query.getColumn(1);
        int         size    = query.getColumn(2);
        
        std::cout << "row: " << id << ", " << value << ", " << size << std::endl;
    }
}
catch (std::exception& e)
{
    std::cout << "exception: " << e.what() << std::endl;
}
```

### Second sample shows how to manage a transaction:

```C++
try
{
    SQLite::Database    db("transaction.db3", SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);

    db.exec("DROP TABLE IF EXISTS test");

    // Begin transaction
    SQLite::Transaction transaction(db);

    db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)");

    int nb = db.exec("INSERT INTO test VALUES (NULL, \"test\")");
    std::cout << "INSERT INTO test VALUES (NULL, \"test\")\", returned " << nb << std::endl;

    // Commit transaction
    transaction.commit();
}
catch (std::exception& e)
{
    std::cout << "exception: " << e.what() << std::endl;
}
```

### How to handle assertion in SQLiteC++:
Exceptions shall not be used in destructors, so SQLiteC++ use SQLITECPP_ASSERT() to check for errors in destructors.
If you don't want assert() to be called, you have to enable and define an assert handler as shown below,
and by setting the flag SQLITECPP_ENABLE_ASSERT_HANDLER when compiling the lib.

```C++
#ifdef SQLITECPP_ENABLE_ASSERT_HANDLER
namespace SQLite
{
/// definition of the assertion handler enabled when SQLITECPP_ENABLE_ASSERT_HANDLER is defined in the project (CMakeList.txt)
void assertion_failed(const char* apFile, const long apLine, const char* apFunc, const char* apExpr, const char* apMsg)
{
    // Print a message to the standard error output stream, and abort the program.
    std::cerr << apFile << ":" << apLine << ":" << " error: assertion failed (" << apExpr << ") in " << apFunc << "() with message \"" << apMsg << "\"\n";
    std::abort();
}
}
#endif
```

## How to contribute
### GitHub website
The most efficient way to help and contribute to this wrapper project is to
use the tools provided by GitHub:
- please fill bug reports and feature requests here: https://github.com/SRombauts/SQLiteCpp/issues
- fork the repository, make some small changes and submit them with pull-request

### Contact
You can also email me directly, I will answer any questions and requests.

### Coding Style Guidelines
The source code use the CamelCase naming style variant where :
- type names (class, struct, typedef, enums...) begins with a capital letter
- files (.cpp/.h) are named like the class they contains
- function and variable names begins with a lower case letter
- member variables begins with a 'm', function arguments begins with a 'a', boolean with a 'b', pointers with a 'p'
- each file, class, method and member variable is documented using Doxygen tags
See also http://www.appinf.com/download/CppCodingStyleGuide.pdf for good guidelines

## See also - Some other simple C++ SQLite wrappers:

See bellow a short comparison of other wrappers done at the time of the writting:
 - [sqdbcpp](http://code.google.com/p/sqdbcpp/): RAII design, simple, no dependencies, UTF-8/UTF-16, new BSD license
 - [sqlite3cc](http://ed.am/dev/sqlite3cc): uses boost, modern design, LPGPL
 - [sqlite3pp](https://github.com/iwongu/sqlite3pp): modern design inspired by boost, MIT License
 - [SQLite++](http://sqlitepp.berlios.de/): uses boost build system, Boost License 1.0 
 - [CppSQLite](http://www.codeproject.com/Articles/6343/CppSQLite-C-Wrapper-for-SQLite/): famous Code Project but old design, BSD License 
 - [easySQLite](http://code.google.com/p/easysqlite/): manages table as structured objects, complex 
 - [sqlite_modern_cpp](https://github.com/keramer/sqlite_modern_cpp): modern C++11, all in one file, MIT license
