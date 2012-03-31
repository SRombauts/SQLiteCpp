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
#include <stdexcept>
#include <vector>
#include <algorithm>

namespace SQLite
{

// Forward declaration
class Statement;

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
     * Exception is thrown in case of error, then the Database object is NOT constructed.
     */
    explicit Database(const char* apFilename, const bool abReadOnly = true, const bool abCreate = false);

    /**
     * @brief Close the SQLite database connection.
     *
     * All SQLite statements must have been finalized before,
     * so all Statement objects must have been unregistered.
     */
    virtual ~Database(void);

    /**
     * @brief Register a Statement object (a SQLite query)
     */
    void registerStatement (Statement& aStatement);

    /**
     * @brief Unregister a Statement object
     */
    void unregisterStatement (Statement& aStatement);

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
