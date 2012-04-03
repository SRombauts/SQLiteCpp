/**
 * @file  Database.cpp
 * @brief Management of a SQLite Database Connection.
 *
 * Copyright (c) 2012 Sebastien Rombauts (sebastien dot rombauts at gmail dot com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include "Database.h"

#include "Statement.h"
#include <iostream>

namespace SQLite
{

// Open the provided database UTF-8 filename.
Database::Database(const char* apFilename, const int aFlags /*= SQLITE_OPEN_READONLY*/) : // throw(SQLite::Exception)
    mpSQLite(NULL),
    mFilename(apFilename)
{
    int ret = sqlite3_open_v2(apFilename, &mpSQLite, aFlags, NULL);
    if (SQLITE_OK != ret)
    {
        std::string strerr = sqlite3_errmsg(mpSQLite);
        sqlite3_close(mpSQLite);
        throw SQLite::Exception(strerr);
    }
}

// Close the SQLite database connection.
Database::~Database(void) throw() // nothrow
{
    // check for undestroyed statements
    std::vector<Statement*>::iterator   iStatement;
    for (iStatement  = mStatementList.begin();
         iStatement != mStatementList.end();
         iStatement++)
    {
        // TODO (*iStatement)->Finalize(); ?
        //std::cout << "Unregistered statement: " << (*iStatement)->getQuery().c_str() << " !\n";
    }

    int ret = sqlite3_close(mpSQLite);
    if (SQLITE_OK != ret)
    {
        // Never throw an exception in a destructor
        //std::cout << sqlite3_errmsg(mpSQLite);
    }
}

// Register a Statement object (a SQLite query)
void Database::registerStatement(Statement& aStatement) // throw(SQLite::Exception)
{
    mStatementList.push_back(&aStatement);
}

// Unregister a Statement object
void Database::unregisterStatement(Statement& aStatement) // throw(SQLite::Exception)
{
    TStatementList::iterator   iStatement;
    iStatement = std::find(mStatementList.begin(), mStatementList.end(), &aStatement);
    if (mStatementList.end() != iStatement)
    {
        mStatementList.erase(iStatement);
    }
}


};  // namespace SQLite
