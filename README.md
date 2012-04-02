SQLiteC++
---------

Copyright (c) 2012 Sebastien Rombauts (sebastien dot rombauts at gmail dot com)

Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
or copy at [http://opensource.org/licenses/MIT])


SQLiteC++ is a smart and simple C++ SQLite3 wrapper, easy to use and efficient.

It is designed with the Resource Acquisition Is Initialization (RAII) idom
(see http://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization),
and throw exceptions in case of SQLite errors.

Each SQLiteC++ object must be constructed with a valid SQLite database connexion, and then is always valid until destroyed.

Depandancies:

 - a STL implementation (even an old one like VC6/eVC4 should work)
 - exception support (the class Exception inherite from std::runtime_error)
 - the SQLite library, either by linking to it dynamicaly or staticaly,
   or by adding its source file in your project code base.


To use it in your project, you only need to add the 6 SQLiteC++ source files
in your project code base (not the main.cpp example file).

Tot get started, look at the provided examples in main.cpp, starting by :
<pre>
int main (void)
{
    try
    {
        // Open a database file
        SQLite::Database    db("example.db3");
        std::cout << "SQLite database file " << db.getFilename().c_str() << " opened successfully\n";

        // Compile a SQL query, containing one parameter (index 1)
        SQLite::Statement   query(db, "SELECT * FROM test WHERE size > ?");
        std::cout << "SQLite statement " << query.getQuery().c_str() << " compiled (" << query.getColumnCount () << " columns in the result)\n";
        
        // Bind the integer value 6 to the first parameter of the SQL query
        query.bind(1, 6);

        // Loop to execute the query step by step, to get one a row of results at a time
        while (query.executeStep())
        {
            // Demonstrate how to get some typed column value
            int         id      = query.getColumn(0); // = query.getColumn(0).getInt()
            std::string value   = query.getColumn(1); // = query.getColumn(1).getText()
            int         size    = query.getColumn(2); // = query.getColumn(2).getInt()

            std::cout << "row : (" << id << ", " << value << ", " << size << ")\n";
        }

        // Reset the query to use it again later
        query.reset();
    }
    catch (std::exception& e)
    {
        std::cout << "SQLite exception: " << e.what() << std::endl;
    }
}
</pre>

For other simple C++ SQLite wrappers look also at:

 - sqdbcpp also using RAII [http://code.google.com/p/sqdbcpp/]
 - CppSQLite [http://www.codeproject.com/Articles/6343/CppSQLite-C-Wrapper-for-SQLite/]
   or a git fork at [https://github.com/NeoSmart/CppSQLite/]
 - sqlite3pp [http://code.google.com/p/sqlite3pp/]
 - SQLite++ [http://sqlitepp.berlios.de/]


  [http://opensource.org/licenses/MIT]: http://opensource.org/licenses/MIT
  [http://code.google.com/p/sqdbcpp/]: http://code.google.com/p/sqdbcpp/
  [http://www.codeproject.com/Articles/6343/CppSQLite-C-Wrapper-for-SQLite/]: http://www.codeproject.com/Articles/6343/CppSQLite-C-Wrapper-for-SQLite/
  [https://github.com/NeoSmart/CppSQLite/]: https://github.com/NeoSmart/CppSQLite/
  [http://code.google.com/p/sqlite3pp/]: http://code.google.com/p/sqlite3pp/
  [http://sqlitepp.berlios.de/]: http://sqlitepp.berlios.de/

