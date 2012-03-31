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
    mbDone(false)
{
    int ret = sqlite3_prepare_v2(mDatabase.mpSQLite, mQuery.c_str(), mQuery.size(), &mpStmt, NULL);
    if (SQLITE_OK != ret)
    {
        throw SQLite::Exception(sqlite3_errmsg(mDatabase.mpSQLite));
    }
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
    bool bOk = false;

    if (false == mbDone)
    {
        int ret = sqlite3_step(mpStmt);
        if (SQLITE_ROW == ret)
        {
            bOk = true;
        }
        else if (SQLITE_DONE == ret)
        {
            bOk = false;
            mbDone = true;
        }
        else
        {
            throw SQLite::Exception(sqlite3_errmsg(mDatabase.mpSQLite));
        }
    }

    return bOk;
}

// Return the number of columns in the result set returned by the prepared statement
int Statement::getColumnCount(void) const throw() // nothrow
{
    return sqlite3_column_count(mpStmt);
}

};  // namespace SQLite
