/**
 * @file  main.cpp
 * @brief A few short examples in a row.
 *
 *  Demonstrate how-to use the SQLite++ wrapper
 *
 * Copyright (c) 2012-2014 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

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

/// Get example path
static inline std::string getExamplePath()
{
    std::string filePath(__FILE__);
    return filePath.substr( 0, filePath.length() - std::string("main.cpp").length());
}

/// Example Database
static const std::string filename_example_db3 = getExamplePath() + "/example.db3";
/// Image
static const std::string filename_logo_png    = getExamplePath() + "/logo.png";


/// Object Oriented Basic example
class Example
{
public:
    // Constructor
    Example() :
        mDb(filename_example_db3),                                  // Open a database file in readonly mode
        mQuery(mDb, "SELECT * FROM test WHERE weight > :min_weight")// Compile a SQL query, containing one parameter (index 1)
    {
    }
    virtual ~Example()
    {
    }

    /// List the rows where the "weight" column is greater than the provided aParamValue
    void ListGreaterThan (const int aParamValue)
    {
        std::cout << "ListGreaterThan (" << aParamValue << ")\n";

        // Bind the integer value provided to the first parameter of the SQL query
        mQuery.bind(":min_weight", aParamValue); // same as mQuery.bind(1, aParamValue);

        // Loop to execute the query step by step, to get one a row of results at a time
        while (mQuery.executeStep())
        {
            std::cout << "row (" << mQuery.getColumn(0) << ", \"" << mQuery.getColumn(1) << "\", " << mQuery.getColumn(2) << ")\n";
        }

        // Reset the query to be able to use it again later
        mQuery.reset();
    }

private:
    SQLite::Database    mDb;    ///< Database connection
    SQLite::Statement   mQuery; ///< Database prepared SQL query
};


int main ()
{
    // Basic example (1/6) :
    try
    {
        // Open a database file in readonly mode
        SQLite::Database    db(filename_example_db3);  // SQLITE_OPEN_READONLY
        std::cout << "SQLite database file '" << db.getFilename().c_str() << "' opened successfully\n";

        // Test if the 'test' table exists
        bool bExists = db.tableExists("test");
        std::cout << "SQLite table 'test' exists=" << bExists << "\n";

        // Get a single value result with an easy to use shortcut
        std::string value = db.execAndGet("SELECT value FROM test WHERE id=2");
        std::cout << "execAndGet=" << value.c_str() << std::endl;

        // Compile a SQL query, containing one parameter (index 1)
        SQLite::Statement   query(db, "SELECT id as test_id, value as test_val, weight as test_weight FROM test WHERE weight > ?");
        std::cout << "SQLite statement '" << query.getQuery().c_str() << "' compiled (" << query.getColumnCount () << " columns in the result)\n";
        // Bind the integer value 2 to the first parameter of the SQL query
        query.bind(1, 2);
        std::cout << "binded with integer value '2' :\n";

        // Loop to execute the query step by step, to get one a row of results at a time
        while (query.executeStep())
        {
            // Demonstrate how to get some typed column value (and the equivalent explicit call)
            int         id      = query.getColumn(0); // = query.getColumn(0).getInt()
          //const char* pvalue  = query.getColumn(1); // = query.getColumn(1).getText()
            std::string value2  = query.getColumn(1); // = query.getColumn(1).getText()
            int         bytes   = query.getColumn(1).getBytes();
            double      weight  = query.getColumn(2); // = query.getColumn(2).getInt()

            static bool bFirst = true;
            if (bFirst)
            {
                // Show how to get the aliased names of the result columns.
                std::string name0 = query.getColumn(0).getName();
                std::string name1 = query.getColumn(1).getName();
                std::string name2 = query.getColumn(2).getName();
                std::cout << "aliased result [\"" << name0.c_str() << "\", \"" << name1.c_str() << "\", \"" << name2.c_str() << "\"]\n";
#ifdef SQLITE_ENABLE_COLUMN_METADATA
                // Show how to get origin names of the table columns from which theses result columns come from.
                // Requires the SQLITE_ENABLE_COLUMN_METADATA preprocessor macro to be
                // also defined at compile times of the SQLite library itself.
                name0 = query.getColumn(0).getOriginName();
                name1 = query.getColumn(1).getOriginName();
                name2 = query.getColumn(2).getOriginName();
                std::cout << "origin table 'test' [\"" << name0.c_str() << "\", \"" << name1.c_str() << "\", \"" << name2.c_str() << "\"]\n";
#endif
                bFirst = false;
            }
            std::cout << "row (" << id << ", \"" << value2.c_str() << "\" "  << bytes << " bytes, " << weight << ")\n";
        }

        // Reset the query to use it again
        query.reset();
        std::cout << "SQLite statement '" << query.getQuery().c_str() << "' reseted (" << query.getColumnCount () << " columns in the result)\n";
        // Bind the string value "6" to the first parameter of the SQL query
        query.bind(1, "6");
        std::cout << "binded with string value \"6\" :\n";

        while (query.executeStep())
        {
            // Demonstrate that inserting column value in a std:ostream is natural
            std::cout << "row (" << query.getColumn(0) << ", \"" << query.getColumn(1) << "\", " << query.getColumn(2) << ")\n";
        }
    }
    catch (std::exception& e)
    {
        std::cout << "SQLite exception: " << e.what() << std::endl;
        return EXIT_FAILURE; // unexpected error : exit the example program
    }

    ////////////////////////////////////////////////////////////////////////////
    // Object Oriented Basic example (2/6) :
    try
    {
        // Open the database and compile the query
        Example example;

        // Demonstrate the way to use the same query with different parameter values
        example.ListGreaterThan(8);
        example.ListGreaterThan(6);
        example.ListGreaterThan(2);
    }
    catch (std::exception& e)
    {
        std::cout << "SQLite exception: " << e.what() << std::endl;
        return EXIT_FAILURE; // unexpected error : exit the example program
    }

    // The execAndGet wrapper example (3/6) :
    try
    {
        // Open a database file in readonly mode
        SQLite::Database    db(filename_example_db3);  // SQLITE_OPEN_READONLY
        std::cout << "SQLite database file '" << db.getFilename().c_str() << "' opened successfully\n";

        // WARNING: Be very careful with this dangerous method: you have to
        // make a COPY OF THE result, else it will be destroy before the next line
        // (when the underlying temporary Statement and Column objects are destroyed)
        std::string value = db.execAndGet("SELECT value FROM test WHERE id=2");
        std::cout << "execAndGet=" << value.c_str() << std::endl;
    }
    catch (std::exception& e)
    {
        std::cout << "SQLite exception: " << e.what() << std::endl;
        return EXIT_FAILURE; // unexpected error : exit the example program
    }

    ////////////////////////////////////////////////////////////////////////////
    // Simple batch queries example (4/6) :
    try
    {
        // Open a database file in create/write mode
        SQLite::Database    db("test.db3", SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
        std::cout << "SQLite database file '" << db.getFilename().c_str() << "' opened successfully\n";

        // Create a new table with an explicit "id" column aliasing the underlying rowid
        db.exec("DROP TABLE IF EXISTS test");
        db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)");

        // first row
        int nb = db.exec("INSERT INTO test VALUES (NULL, \"test\")");
        std::cout << "INSERT INTO test VALUES (NULL, \"test\")\", returned " << nb << std::endl;

        // second row
        nb = db.exec("INSERT INTO test VALUES (NULL, \"second\")");
        std::cout << "INSERT INTO test VALUES (NULL, \"second\")\", returned " << nb << std::endl;

        // update the second row
        nb = db.exec("UPDATE test SET value=\"second-updated\" WHERE id='2'");
        std::cout << "UPDATE test SET value=\"second-updated\" WHERE id='2', returned " << nb << std::endl;

        // Check the results : expect two row of result
        SQLite::Statement   query(db, "SELECT * FROM test");
        std::cout << "SELECT * FROM test :\n";
        while (query.executeStep())
        {
            std::cout << "row (" << query.getColumn(0) << ", \"" << query.getColumn(1) << "\")\n";
        }

        db.exec("DROP TABLE test");
    }
    catch (std::exception& e)
    {
        std::cout << "SQLite exception: " << e.what() << std::endl;
        return EXIT_FAILURE; // unexpected error : exit the example program
    }
    remove("test.db3");

    ////////////////////////////////////////////////////////////////////////////
    // RAII transaction example (5/6) :
    try
    {
        // Open a database file in create/write mode
        SQLite::Database    db("transaction.db3", SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
        std::cout << "SQLite database file '" << db.getFilename().c_str() << "' opened successfully\n";

        db.exec("DROP TABLE IF EXISTS test");

        // Exemple of a successful transaction :
        try
        {
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
            std::cout << "SQLite exception: " << e.what() << std::endl;
            return EXIT_FAILURE; // unexpected error : exit the example program
        }

        // Exemple of a rollbacked transaction :
        try
        {
            // Begin transaction
            SQLite::Transaction transaction(db);

            int nb = db.exec("INSERT INTO test VALUES (NULL, \"second\")");
            std::cout << "INSERT INTO test VALUES (NULL, \"second\")\", returned " << nb << std::endl;

            nb = db.exec("INSERT INTO test ObviousError");
            std::cout << "INSERT INTO test \"error\", returned " << nb << std::endl;

            return EXIT_FAILURE; // unexpected success : exit the example program

            // Commit transaction
            transaction.commit();
        }
        catch (std::exception& e)
        {
            std::cout << "SQLite exception: " << e.what() << std::endl;
            // expected error, see above
        }

        // Check the results (expect only one row of result, as the second one has been rollbacked by the error)
        SQLite::Statement   query(db, "SELECT * FROM test");
        std::cout << "SELECT * FROM test :\n";
        while (query.executeStep())
        {
            std::cout << "row (" << query.getColumn(0) << ", \"" << query.getColumn(1) << "\")\n";
        }
    }
    catch (std::exception& e)
    {
        std::cout << "SQLite exception: " << e.what() << std::endl;
        return EXIT_FAILURE; // unexpected error : exit the example program
    }
    remove("transaction.db3");

    ////////////////////////////////////////////////////////////////////////////
    // Binary blob and in-memory database example (6/6) :
    try
    {
        // Open a database file in create/write mode
        SQLite::Database    db(":memory:", SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
        std::cout << "SQLite database file '" << db.getFilename().c_str() << "' opened successfully\n";

        db.exec("DROP TABLE IF EXISTS test");
        db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value BLOB)");

        FILE* fp = fopen(filename_logo_png.c_str(), "rb");
        if (NULL != fp)
        {
            char  buffer[16*1024];
            void* blob = &buffer;
            int size = static_cast<int>(fread(blob, 1, 16*1024, fp));
            buffer[size] = '\0';
            fclose (fp);
            std::cout << "blob size=" << size << " :\n";

            // Insert query
            SQLite::Statement   query(db, "INSERT INTO test VALUES (NULL, ?)");
            // Bind the blob value to the first parameter of the SQL query
            query.bind(1, blob, size);
            std::cout << "blob binded successfully\n";

            // Execute the one-step query to insert the blob
            int nb = query.exec ();
            std::cout << "INSERT INTO test VALUES (NULL, ?)\", returned " << nb << std::endl;
        }
        else
        {
            std::cout << "file " << filename_logo_png << " not found !\n";
            return EXIT_FAILURE; // unexpected error : exit the example program
        }

        fp = fopen("out.png", "wb");
        if (NULL != fp)
        {
            const void* blob = NULL;
            size_t size;

            SQLite::Statement   query(db, "SELECT * FROM test");
            std::cout << "SELECT * FROM test :\n";
            if (query.executeStep())
            {
                SQLite::Column colBlob = query.getColumn(1);
                blob = colBlob.getBlob ();
                size = colBlob.getBytes ();
                std::cout << "row (" << query.getColumn(0) << ", size=" << size << ")\n";
                size_t sizew = fwrite(blob, 1, size, fp);
                SQLITECPP_ASSERT(sizew == size, "fwrite failed");   // See SQLITECPP_ENABLE_ASSERT_HANDLER
                fclose (fp);
            }
        }
        else
        {
            std::cout << "file out.png not created !\n";
            return EXIT_FAILURE; // unexpected error : exit the example program
        }
    }
    catch (std::exception& e)
    {
        std::cout << "SQLite exception: " << e.what() << std::endl;
        return EXIT_FAILURE; // unexpected error : exit the example program
    }
    remove("out.png");

    std::cout << "everything ok, quitting\n";

    return EXIT_SUCCESS;
}
