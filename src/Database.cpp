/**
 * @file    Database.cpp
 * @ingroup SQLiteCpp
 * @brief   Management of a SQLite Database Connection.
 *
 * Copyright (c) 2012-2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include "Database.h"

#include "Statement.h"


namespace SQLite
{


// Open the provided database UTF-8 filename with SQLITE_OPEN_xxx provided flags.
Database::Database(const char* apFilename, const int aFlags /*= SQLITE_OPEN_READONLY*/) : // throw(SQLite::Exception)
    mpSQLite(NULL),
    mFilename(apFilename)
{
    int ret = sqlite3_open_v2(apFilename, &mpSQLite, aFlags, NULL);
    if (SQLITE_OK != ret)
    {
        std::string strerr = sqlite3_errmsg(mpSQLite);
        sqlite3_close(mpSQLite); // close is required even in case of error on opening
        throw SQLite::Exception(strerr);
    }
}

// Close the SQLite database connection.
Database::~Database(void) throw() // nothrow
{
    int ret = sqlite3_close(mpSQLite);
    // Never throw an exception in a destructor
    //std::cout << sqlite3_errmsg(mpSQLite) << std::endl;
    SQLITE_CPP_ASSERT (SQLITE_OK == ret);
}

// Shortcut to execute one or multiple SQL statements without results (UPDATE, INSERT, ALTER, COMMIT...).
int Database::exec(const char* apQueries) // throw(SQLite::Exception);
{
    int ret = sqlite3_exec(mpSQLite, apQueries, NULL, NULL, NULL);
    check(ret);

    // Return the number of rows modified by those SQL statements (INSERT, UPDATE or DELETE)
    return sqlite3_changes(mpSQLite);
}

// Shortcut to execute a one step query and fetch the first column of the result.
// WARNING: Be very careful with this dangerous method: you have to
// make a COPY OF THE result, else it will be destroy before the next line
// (when the underlying temporary Statement and Column objects are destroyed)
// this is an issue only for pointer type result (ie. char* and blob)
// (use the Column copy-constructor)
Column Database::execAndGet(const char* apQuery) // throw(SQLite::Exception)
{
    Statement query(*this, apQuery);
    query.executeStep();
    return query.getColumn(0);
}

// Shortcut to test if a table exists.
bool Database::tableExists(const char* apTableName) // throw(SQLite::Exception)
{
    Statement query(*this, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?");
    query.bind(1, apTableName);
    query.executeStep();
    int Nb = query.getColumn(0);
    return (1 == Nb);
}

// Check if aRet equal SQLITE_OK, else throw a SQLite::Exception with the SQLite error message
void Database::check(const int aRet) const // throw(SQLite::Exception)
{
    if (SQLITE_OK != aRet)
    {
        throw SQLite::Exception(sqlite3_errmsg(mpSQLite));
    }
}


}  // namespace SQLite
