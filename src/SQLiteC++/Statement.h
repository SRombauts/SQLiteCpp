/**
 * @file  Statement.h
 * @brief A prepared SQLite Statement is a compiled SQL query ready to be executed.
 *
 * Copyright (c) 2012 Sebastien Rombauts (sebastien dot rombauts at gmail dot com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include <sqlite3.h>
#include "Exception.h"

namespace SQLite
{

// Forward declaration
class Database;

/**
 * @brief Encapsulation of a prepared SQLite Statement.
 *
 * A Statement is a compiled SQL query ready to be executed step by step
 * to provide results one row at a time.
 */
class Statement
{
public:
    /**
     * @brief Compile and register the SQL query for the provided SQLite Database Connection
     *
     * Exception is thrown in case of error, then the Statement object is NOT constructed.
     */
    explicit Statement(Database &aDatabase, const char* apQuery) throw (SQLite::Exception);

    /**
     * @brief Finalize and unregister the SQL query from the SQLite Database Connection.
     */
    virtual ~Statement(void) throw (); // nothrow

    /**
     * @brief Reset the statement to make it ready for a new execution.
     */
    void reset (void) throw (SQLite::Exception);

    // TODO bind 

    /**
     * @brief Execute a step of the query to fetch one row of results.
     */
    bool executeStep (void) throw (SQLite::Exception);

    /**
     * @brief UTF-8 SQL Query.
     */
    inline const std::string& getQuery(void) const
    {
        return mQuery;
    }

    /**
     * @brief True when the last row is fetched with executeStep().
     */
    inline bool isDone(void) const
    {
        return mbDone;
    }

private:
    sqlite3_stmt*   mpStmt;     //!< Pointeur to SQLite Statement Object
    Database&       mDatabase;  //!< Reference to the SQLite Database Connection
    std::string     mQuery;     //!< UTF-8 SQL Query
    bool            mbDone;     //!< True when the last row is fetched with executeStep()
};


};  // namespace SQLite
