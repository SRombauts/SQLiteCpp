/**
 * @file  Statement.cpp
 * @brief A prepared SQLite Statement is a compiled SQL query ready to be executed.
 *
 * Copyright (c) 2012 Sebastien Rombauts (sebastien dot rombauts at gmail dot com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include "Statement.h"

#include "Database.h"
#include <iostream>

namespace SQLite
{

// Compile and register the SQL query for the provided SQLite Database Connection
Statement::Statement(Database &aDatabase, const char* apQuery) : // throw(SQLite::Exception)
    mpStmt(NULL),
    mDatabase(aDatabase),
    mQuery(apQuery),
    mbOk(false),
    mbDone(false)
{
    int ret = sqlite3_prepare_v2(mDatabase.mpSQLite, mQuery.c_str(), mQuery.size(), &mpStmt, NULL);
    if (SQLITE_OK != ret)
    {
        throw SQLite::Exception(sqlite3_errmsg(mDatabase.mpSQLite));
    }
    mColumnCount = sqlite3_column_count(mpStmt);
    mDatabase.registerStatement(*this);
}

// Finalize and unregister the SQL query from the SQLite Database Connection.
Statement::~Statement(void) throw() // nothrow
{
    int ret = sqlite3_finalize(mpStmt);
    if (SQLITE_OK != ret)
    {
        std::cout << sqlite3_errmsg(mDatabase.mpSQLite);
    }
    mpStmt = NULL;
    mDatabase.unregisterStatement(*this);
}

// Reset the statement to make it ready for a new execution
void Statement::reset(void) // throw(SQLite::Exception)
{
    mbOk = false;
    mbDone = false;
    int ret = sqlite3_reset(mpStmt);
    if (SQLITE_OK != ret)
    {
        throw SQLite::Exception(sqlite3_errmsg(mDatabase.mpSQLite));
    }
}

// Bind an int value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const int& aValue) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_int(mpStmt, aIndex, aValue);
    if (SQLITE_OK != ret)
    {
        throw SQLite::Exception(sqlite3_errmsg(mDatabase.mpSQLite));
    }
}

// Bind a 64bits int value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const sqlite3_int64& aValue) // throw(SQLite::Exception)

{
    int ret = sqlite3_bind_int64(mpStmt, aIndex, aValue);
    if (SQLITE_OK != ret)
    {
        throw SQLite::Exception(sqlite3_errmsg(mDatabase.mpSQLite));
    }
}

// Bind a double (64bits float) value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const double& aValue) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_double(mpStmt, aIndex, aValue);
    if (SQLITE_OK != ret)
    {
        throw SQLite::Exception(sqlite3_errmsg(mDatabase.mpSQLite));
    }
}

// Bind a string value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const std::string& aValue) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_text(mpStmt, aIndex, aValue.c_str(), aValue.size(), SQLITE_TRANSIENT);
    if (SQLITE_OK != ret)
    {
        throw SQLite::Exception(sqlite3_errmsg(mDatabase.mpSQLite));
    }
}

// Bind a text value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const char* apValue) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_text(mpStmt, aIndex, apValue, -1, SQLITE_TRANSIENT);
    if (SQLITE_OK != ret)
    {
        throw SQLite::Exception(sqlite3_errmsg(mDatabase.mpSQLite));
    }
}

// Bind a NULL value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_null(mpStmt, aIndex);
    if (SQLITE_OK != ret)
    {
        throw SQLite::Exception(sqlite3_errmsg(mDatabase.mpSQLite));
    }
}

// Execute a step of the query to fetch one row of results
bool Statement::executeStep(void) // throw(SQLite::Exception)
{
    if (false == mbDone)
    {
        int ret = sqlite3_step(mpStmt);
        if (SQLITE_ROW == ret)
        {
            mbOk = true;
        }
        else if (SQLITE_DONE == ret)
        {
            mbOk = false;
            mbDone = true;
        }
        else
        {
            throw SQLite::Exception(sqlite3_errmsg(mDatabase.mpSQLite));
        }
    }

    return mbOk;
}

// Return the integer value of the column specified by its index starting at 0
int Statement::getColumnInt(const int aIndex) const // throw(SQLite::Exception)
{
    if (false == mbOk)
    {
        throw SQLite::Exception("No row to get a column from");
    }
    else if ((aIndex < 0) || (aIndex >= mColumnCount))
    {
        throw SQLite::Exception("Column index out of range");
    }

    return sqlite3_column_int(mpStmt, aIndex);
}


// Return the 64bits integer value of the column specified by its index starting at 0
sqlite3_int64 Statement::getColumnInt64(const int aIndex) const // throw(SQLite::Exception)
{
    if (false == mbOk)
    {
        throw SQLite::Exception("No row to get a column from");
    }
    else if ((aIndex < 0) || (aIndex >= mColumnCount))
    {
        throw SQLite::Exception("Column index out of range");
    }

    return sqlite3_column_int64(mpStmt, aIndex);
}

// Return the double value of the column specified by its index starting at 0
double Statement::getColumnDouble(const int aIndex) const // throw(SQLite::Exception)
{
    if (false == mbOk)
    {
        throw SQLite::Exception("No row to get a column from");
    }
    else if ((aIndex < 0) || (aIndex >= mColumnCount))
    {
        throw SQLite::Exception("Column index out of range");
    }

    return sqlite3_column_double(mpStmt, aIndex);
}

// Return the text value (NULL terminated string) of the column specified by its index starting at 0
const char * Statement::getColumnText(const int aIndex) const // throw(SQLite::Exception)
{
    if (false == mbOk)
    {
        throw SQLite::Exception("No row to get a column from");
    }
    else if ((aIndex < 0) || (aIndex >= mColumnCount))
    {
        throw SQLite::Exception("Column index out of range");
    }

    return (const char*)sqlite3_column_text(mpStmt, aIndex);
}

// Test if the column is NULL
bool Statement::isColumnNull(const int aIndex) const // throw(SQLite::Exception)
{
    if (false == mbOk)
    {
        throw SQLite::Exception("No row to get a column from");
    }
    else if ((aIndex < 0) || (aIndex >= mColumnCount))
    {
        throw SQLite::Exception("Column index out of range");
    }

    return (SQLITE_NULL == sqlite3_column_type(mpStmt, aIndex));
}

};  // namespace SQLite
