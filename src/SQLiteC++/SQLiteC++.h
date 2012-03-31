/**
 * @brief SQLiteC++ is a smart and simple C++ SQLite3 wrapper.
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
    explicit Database(const char* apFilename, const bool abReadOnly = true, const bool abCreate = false) :
        mpSQLite(NULL),
        mFilename (apFilename)
    {
        int flags = abReadOnly?SQLITE_OPEN_READONLY:SQLITE_OPEN_READWRITE;
        if (abCreate)
        {
            flags |= SQLITE_OPEN_CREATE;
        }
        
        int ret = sqlite3_open_v2(apFilename, &mpSQLite, flags, NULL);
        if (SQLITE_OK != ret)
        {
            std::string strerr = sqlite3_errmsg(mpSQLite);
            sqlite3_close(mpSQLite);
            throw std::runtime_error(strerr);
        }
    }
    /**
     * @brief Close the SQLite database connection.
     *
     * All SQLite statements must have been finalized before,
     * so all Statement objects must have been unregistered.
     */
    virtual ~Database(void)
    {
        // check for undestroyed statements
        std::vector<Statement*>::iterator   iStatement;
        for (iStatement  = mStatementList.begin();
             iStatement != mStatementList.end();
             iStatement++)
        {
            // TODO (*iStatement)->Finalize(); ?
            std::cout << "Unregistered statement!\n";
        }

        int ret = sqlite3_close(mpSQLite);
        if (SQLITE_OK != ret)
        {
            std::cout << sqlite3_errmsg(mpSQLite);
        }
    }

    /// Register a Statement object (a SQLite query)
    void registerStatement (Statement& aStatement)
    {
        mStatementList.push_back (&aStatement);
    }
    /// Unregister a Statement object
    void unregisterStatement (Statement& aStatement)
    {
        std::vector<Statement*>::iterator   iStatement;
        iStatement = std::find (mStatementList.begin(), mStatementList.end(), &aStatement);
        if (mStatementList.end() != iStatement)
        {
            mStatementList.erase (iStatement);
        }
    }

    /// Filename used to open the database
    inline const std::string& getFilename(void) const
    {
        return mFilename;
    }
    
private:
    sqlite3*                mpSQLite;       //!< Pointer to SQLite Database Connection Handle
    std::string             mFilename;      //!< UTF-8 filename used to open the database
    std::vector<Statement*> mStatementList; //!< Liste of SQL statements used with this database connexion
};


/**
 * @brief Encapsulation of a SQLite Statement.
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
    explicit Statement(Database &aDatabase, const char* apQuery) :
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
    /**
     * @brief Finalize and unregister the SQL query from the SQLite Database Connection.
     */
    virtual ~Statement(void)
    {
        int ret = sqlite3_finalize(mpStmt);
        if (SQLITE_OK != ret)
        {
            std::cout << sqlite3_errmsg(mDatabase.mpSQLite);
        }
        mDatabase.unregisterStatement(*this);
    }

    /// Reset the statement to make it ready for a new execution
    void reset (void)
    {
        mbDone = false;
        int ret = sqlite3_reset(mpStmt);
        if (SQLITE_OK != ret)
        {
            throw std::runtime_error(sqlite3_errmsg(mDatabase.mpSQLite));
        }
    }

    // TODO bind 

    /// Execute a step of the query to fetch one row of results
    bool executeStep (void)
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

    /// UTF-8 SQL Query
    inline const std::string& getQuery(void) const
    {
        return mQuery;
    }
    /// True when the last row is fetched with executeStep()
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
