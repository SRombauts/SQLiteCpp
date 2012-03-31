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
Statement::Statement(Database &aDatabase, const char* apQuery) :
    mDatabase(aDatabase),
    mQuery(apQuery),
    mbDone(false)
{
    int ret = sqlite3_prepare_v2(mDatabase.mpSQLite, mQuery.c_str(), mQuery.size(), &mpStmt, NULL);
    if (SQLITE_OK != ret)
    {
        throw std::runtime_error(sqlite3_errmsg(mDatabase.mpSQLite));
    }
    mDatabase.registerStatement(*this);
}

//Finalize and unregister the SQL query from the SQLite Database Connection.
Statement::~Statement(void)
{
    int ret = sqlite3_finalize(mpStmt);
    if (SQLITE_OK != ret)
    {
        std::cout << sqlite3_errmsg(mDatabase.mpSQLite);
    }
    mDatabase.unregisterStatement(*this);
}

// Reset the statement to make it ready for a new execution
void Statement::reset (void)
{
    mbDone = false;
    int ret = sqlite3_reset(mpStmt);
    if (SQLITE_OK != ret)
    {
        throw std::runtime_error(sqlite3_errmsg(mDatabase.mpSQLite));
    }
}

// Execute a step of the query to fetch one row of results
bool Statement::executeStep (void)
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
            bOk = true;
            mbDone = true;
        }
        else
        {
            throw std::runtime_error(sqlite3_errmsg(mDatabase.mpSQLite));
        }
    }

    return bOk;
}


};  // namespace SQLite
