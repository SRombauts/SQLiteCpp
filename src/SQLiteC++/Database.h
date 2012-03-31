/**
 * @file  Database.h
 * @brief Management of a SQLite Database Connection.
 *
 * Copyright (c) 2012 Sebastien Rombauts (sebastien dot rombauts at gmail dot com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include <sqlite3.h>
#include <vector>
#include <algorithm>
#include "Exception.h"

namespace SQLite
{

// Forward declarations
class Statement;
class Exception;

/**
 * @brief Management of a SQLite Database Connection.
 *
 * A Database object manage a list of all SQLite Statements associated with the
 * underlying SQLite 3 database connection.
 */
class Database
{
    friend class Statement;

public:
    /**
     * @brief Open the provided database UTF-8 filename.
     *
     * Uses sqlite3_open_v2() with readonly default flag, which is the opposite behaviour
     * of the old sqlite3_open() function (READWRITE+CREATE).
     * This makes sense if you want to use it on a readonly filesystem
     * or to prenvent creation of a void file when a required file is missing.
     *
     * Exception is thrown in case of error, then the Database object is NOT constructed.
     *
     * @param[in] apFilename    UTF-8 path/uri to the database file ("filename" sqlite3 parameter)
     * @param[in] aFlags        SQLITE_OPEN_READONLY/SQLITE_OPEN_READWRITE/SQLITE_OPEN_CREATE...
     */
    explicit Database(const char* apFilename, const int aFlags = SQLITE_OPEN_READONLY) throw (SQLite::Exception);

    /**
     * @brief Close the SQLite database connection.
     *
     * All SQLite statements must have been finalized before,
     * so all Statement objects must have been unregistered.
     */
    virtual ~Database(void) throw (); // nothrow

    /**
     * @brief Register a Statement object (a SQLite query)
     */
    void registerStatement (Statement& aStatement) throw (SQLite::Exception);

    /**
     * @brief Unregister a Statement object
     */
    void unregisterStatement (Statement& aStatement) throw (SQLite::Exception);

    /**
     * @brief Filename used to open the database
     */
    inline const std::string& getFilename(void) const
    {
        return mFilename;
    }
    
private:
    sqlite3*                mpSQLite;       //!< Pointer to SQLite Database Connection Handle
    std::string             mFilename;      //!< UTF-8 filename used to open the database
    std::vector<Statement*> mStatementList; //!< Liste of SQL statements used with this database connexion
};


};  // namespace SQLite
