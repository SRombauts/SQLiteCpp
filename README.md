SQLiteC++
---------

SQLiteC++ is a smart and easy to use C++ SQLite3 wrapper.

See SQLiteC++ website http://srombauts.github.com/SQLiteCpp on GitHub.


Copyright (c) 2012 Sébastien Rombauts (sebastien.rombauts@gmail.com)

Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
or copy at http://opensource.org/licenses/MIT)


The goals of SQLiteC++ are:

- to offer the best of existing simple wrappers
- to use a permissive license like MIT or BSD
- to be elegantly written with good C++ design, STL, exceptions and RAII idiom
- to keep dependencies to a minimum (STL and SQLite3)
- to be well documented, in code with Doxygen, and online with some good examples
- to be portable
- to be light and fast
- to be monothreaded
- to use API names sticking with those of the SQLite library
- to be well maintained

It is designed with the Resource Acquisition Is Initialization (RAII) idom
(see http://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization),
and throw exceptions in case of SQLite errors.
Each SQLiteC++ object must be constructed with a valid SQLite database connexion,
and then is always valid until destroyed.

Depandancies:

 - a STL implementation (even an old one like VC6/eVC4 should work)
 - exception support (the class Exception inherite from std::runtime_error)
 - the SQLite library, either by linking to it dynamicaly or staticaly,
   or by adding its source file in your project code base.


To use it in your project, you only need to add the 6 SQLiteC++ source files
in your project code base (not the main.cpp example file).

Tot get started, look at the provided examples in main.cpp, starting by :

```C++
int main (void)
{
    try
    {
        // Open a database file
        SQLite::Database    db("example.db3");
        std::cout << "database file opened successfully\n";
        
        // Compile a SQL query, containing one parameter (index 1)
        SQLite::Statement   query(db, "SELECT * FROM test WHERE size > ?");
        std::cout << "statement: " << query.getQuery().c_str()
                  << " compiled (" << query.getColumnCount () << " columns)\n";
        
        // Bind the integer value 6 to the first parameter of the SQL query
        query.bind(1, 6);
        
        // Loop to execute the query step by step, to get rows of result
        while (query.executeStep())
        {
            // Demonstrate how to get some typed column value
            int         id      = query.getColumn(0);
            std::string value   = query.getColumn(1);
            int         size    = query.getColumn(2);
            
            std::cout << "row: " << id << ", " << value << ", " << size << "\n";
        }
        
        // Reset the query to use it again later
        query.reset();
    }
    catch (std::exception& e)
    {
        std::cout << "exception: " << e.what() << std::endl;
    }
}
```

For other simple C++ SQLite wrappers look also at:

 - **sqdbcpp**: RAII design, no depandencies, UTF-8/UTF-16, new BSD license (http://code.google.com/p/sqdbcpp/)
 - **sqlite3pp**: uses boost, MIT License (http://code.google.com/p/sqlite3pp/)
 - **SQLite++**: uses boost build system, Boost License 1.0 (http://sqlitepp.berlios.de/)
 - **sqlite3cc**: uses boost, LPGPL (http://ed.am/dev/sqlite3cc and https://launchpad.net/sqlite3cc)
 - **CppSQLite**: famous Code Project but old design, BSD License (http://www.codeproject.com/Articles/6343/CppSQLite-C-Wrapper-for-SQLite/)
