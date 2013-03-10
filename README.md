SQLiteC++
---------

SQLiteC++ (SQLiteCpp) is a smart and easy to use C++ SQLite3 wrapper.

See SQLiteC++ website http://srombauts.github.com/SQLiteCpp on GitHub.

### License

Copyright (c) 2012-2013 Sébastien Rombauts (sebastien.rombauts@gmail.com)

Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
or copy at http://opensource.org/licenses/MIT)

### The goals of SQLiteC++ are:

- to offer the best of existing simple C++ SQLite wrappers
- to be elegantly written with good C++ design, STL, exceptions and RAII idiom
- to keep dependencies to a minimum (STL and SQLite3)
- to be portable
- to be light and fast
- to be monothreaded (not thread-safe)
- to use API names sticking with those of the SQLite library
- to be well documented in code with Doxygen, and online with some good examples
- to be well maintained
- to use a permissive MIT license, similar to BSD or Boost, for proprietary/commercial usage

It is designed using the Resource Acquisition Is Initialization (RAII) idom
(see http://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization),
and throwing exceptions in case of SQLite errors (exept in destructors,
where assert() are used instead).
Each SQLiteC++ object must be constructed with a valid SQLite database connection,
and then is always valid until destroyed.

### Suported platforms:

Developements and tests are done under the following OSs :
- Debian 7 (testing)
- Ubuntu 12.04
- Windows XP/7/8
And following IDEs/Compilers
- GCC 4.7.x with a provided Makefile
- Eclipse CDT under Linux, using the provided Makefile
- Visual Studio Express 2008/2010/2012 for testing compatibility purpose

### Depandancies:

 - a STL implementation (even an old one, like those provided with VC6/eVC4 should work)
 - exception support (the class Exception inherit from std::runtime_error)
 - the SQLite library, either by linking to it dynamicaly or staticaly (libsqlite3-dev under Linux),
   or by adding its source file in your project code base (source code provided in src/sqlite3 for Windows).

To use it in your project, you only need to add the 10 SQLiteC++ source files
in your project code base (not the main.cpp example file).

## Getting started
### About SQLite:
SQLite is a library that implements a serverless transactional SQL database engine.
It is the most widely deployed SQL database engine in the world.
The source code for SQLite is in the public domain.
http://www.sqlite.org/about.html

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

## See also - Some other simple C++ SQLite wrappers:

See also the file WRAPPERS.md offering a more complete comparison of other wrappers.
 - [sqdbcpp](http://code.google.com/p/sqdbcpp/): RAII design, simple, no depandencies, UTF-8/UTF-16, new BSD license
 - [sqlite3cc](http://ed.am/dev/sqlite3cc): uses boost, modern design, LPGPL
 - [sqlite3pp](http://code.google.com/p/sqlite3pp/): uses boost, but never updated since initial publication in may 2012, MIT License
 - [SQLite++](http://sqlitepp.berlios.de/): uses boost build system, Boost License 1.0 
 - [CppSQLite](http://www.codeproject.com/Articles/6343/CppSQLite-C-Wrapper-for-SQLite/): famous Code Project but old design, BSD License 
 - [easySQLite](http://code.google.com/p/easysqlite/): manages table as structured objects, complex 
