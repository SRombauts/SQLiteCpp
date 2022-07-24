/**
 * @file    RowExecutor.h
 * @ingroup SQLiteCpp
 * @brief   Step executor for SQLite prepared Statement Object
 *
 * Copyright (c) 2015 Shibao HONG (shibaohong@outlook.com)
 * Copyright (c) 2015-2021 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

 //#include <SQLiteCpp/Database.h>
 #include <SQLiteCpp/Exception.h>

#include <memory>
#include <string>
#include <map>

// Forward declaration to avoid inclusion of <sqlite3.h> in a header
struct sqlite3_stmt;

namespace SQLite
{

extern const int OK; ///< SQLITE_OK


class RowExecutor
{
public:
    /// Shared pointer to SQLite Prepared Statement Object
    using TStatementPtr = std::shared_ptr<sqlite3_stmt>;

    /// Weak pointer to SQLite Prepared Statement Object
    using TStatementWeakPtr = std::weak_ptr<sqlite3_stmt>;

    /// Shared pointer to SQLite RowExecutor
    using TRowPtr = std::shared_ptr<RowExecutor>;

    /// Weak pointer to SQLite RowExecutor
    using TRowWeakPtr = std::weak_ptr<RowExecutor>;
    
    /// Type to store columns names and indexes
    using TColumnsMap = std::map<std::string, int, std::less<>>;

    RowExecutor(const RowExecutor&) = delete;
    RowExecutor(RowExecutor&&) = default;
    RowExecutor& operator=(const RowExecutor&) = delete;
    RowExecutor& operator=(RowExecutor&&) = default;

    /// Reset the statement to make it ready for a new execution. Throws an exception on error.
    void reset();

    /// Reset the statement. Returns the sqlite result code instead of throwing an exception on error.
    int tryReset() noexcept;

    /**
     * @brief Execute a step of the prepared query to fetch one row of results.
     *
     *  While true is returned, a row of results is available, and can be accessed
     * through the getColumn() method
     *
     * @see exec() execute a one-step prepared statement with no expected result
     * @see tryExecuteStep() try to execute a step of the prepared query to fetch one row of results, returning the sqlite result code.
     * @see Database::exec() is a shortcut to execute one or multiple statements without results
     *
     * @return - true  (SQLITE_ROW)  if there is another row ready : you can call getColumn(N) to get it
     *                               then you have to call executeStep() again to fetch more rows until the query is finished
     *         - false (SQLITE_DONE) if the query has finished executing : there is no (more) row of result
     *                               (case of a query with no result, or after N rows fetched successfully)
     *
     * @throw SQLite::Exception in case of error
     */
    bool executeStep();

    /**
     * @brief Try to execute a step of the prepared query to fetch one row of results, returning the sqlite result code.
     *
     *
     *
     * @see exec() execute a one-step prepared statement with no expected result
     * @see executeStep() execute a step of the prepared query to fetch one row of results
     * @see Database::exec() is a shortcut to execute one or multiple statements without results
     *
     * @return the sqlite result code.
     */
    int tryExecuteStep() noexcept;

    /**
     * @brief Execute a one-step query with no expected result, and return the number of changes.
     *
     *  This method is useful for any kind of statements other than the Data Query Language (DQL) "SELECT" :
     *  - Data Definition Language (DDL) statements "CREATE", "ALTER" and "DROP"
     *  - Data Manipulation Language (DML) statements "INSERT", "UPDATE" and "DELETE"
     *  - Data Control Language (DCL) statements "GRANT", "REVOKE", "COMMIT" and "ROLLBACK"
     *
     * It is similar to Database::exec(), but using a precompiled statement, it adds :
     * - the ability to bind() arguments to it (best way to insert data),
     * - reusing it allows for better performances (efficient for multiple insertion).
     *
     * @see executeStep() execute a step of the prepared query to fetch one row of results
     * @see tryExecuteStep() try to execute a step of the prepared query to fetch one row of results, returning the sqlite result code.
     * @see Database::exec() is a shortcut to execute one or multiple statements without results
     *
     * @return number of row modified by this SQL statement (INSERT, UPDATE or DELETE)
     *
     * @throw SQLite::Exception in case of error, or if row of results are returned while they are not expected!
     */
    int exec();

    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Execute a step of the prepared query to fetch one row of results.
     *
     *  While true is returned, a row of results is available, and can be accessed
     * through the getColumn() method
     *
     * @see exec() execute a one-step prepared statement with no expected result
     * @see tryExecuteStep() try to execute a step of the prepared query to fetch one row of results, returning the sqlite result code.
     * @see Database::exec() is a shortcut to execute one or multiple statements without results
     *
     * @return - true  (SQLITE_ROW)  if there is another row ready : you can call getColumn(N) to get it
     *                               then you have to call executeStep() again to fetch more rows until the query is finished
     *         - false (SQLITE_DONE) if the query has finished executing : there is no (more) row of result
     *                               (case of a query with no result, or after N rows fetched successfully)
     *
     * @throw SQLite::Exception in case of error
     */
    const TColumnsMap& getColumnsNames() const;

    /// Get number of rows modified by last INSERT, UPDATE or DELETE statement (not DROP table).
    int getChanges() const noexcept;

    /// Return the number of columns in the result set returned by the prepared statement
    int getColumnCount() const noexcept
    {
        return mColumnCount;
    }
    /// true when a row has been fetched with executeStep()
    bool hasRow() const noexcept
    {
        return mbHasRow;
    }
    /// true when the last executeStep() had no more row to fetch
    bool isDone() const noexcept
    {
        return mbDone;
    }

    ////////////////////////////////////////////////////////////////////////////

    /// Return the numeric result code for the most recent failed API call (if any).
    int getErrorCode() const noexcept;

    /// Return the extended numeric result code for the most recent failed API call (if any).
    int getExtendedErrorCode() const noexcept;

    /// Return UTF-8 encoded English language explanation of the most recent failed API call (if any).
    const char* getErrorMsg() const noexcept;

protected:
    /**
    *
    */
    explicit RowExecutor(sqlite3* apSQLite, const std::string& aQuery);

    /**
     * @brief Return a std::shared_ptr with SQLite Statement Object.
     *
     * @return raw pointer to Statement Object
     */
    TStatementPtr getStatement() const noexcept
    {
        return mpStatement;
    }
    
    /**
     * @brief Return a prepared SQLite Statement Object.
     *
     * Throw an exception if the statement object was not prepared.
     * @return raw pointer to Prepared Statement Object
     */
    sqlite3_stmt* getPreparedStatement() const;
    
    /**
     * @brief Return a prepared SQLite Statement Object.
     *
     * Throw an exception if the statement object was not prepared.
     * @return raw pointer to Prepared Statement Object
     */
    TRowWeakPtr getExecutorWeakPtr() const
    {
        return mpRowExecutor;
    }

    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Check if a return code equals SQLITE_OK, else throw a SQLite::Exception with the SQLite error message
     *
     * @param[in] aRet SQLite return code to test against the SQLITE_OK expected value
     */
    void check(const int aRet) const
    {
        if (SQLite::OK != aRet)
        {
            throw SQLite::Exception(mpSQLite, aRet);
        }
    }

    /**
     * @brief Check if there is a row of result returned by executeStep(), else throw a SQLite::Exception.
     */
    void checkRow() const
    {
        if (false == mbHasRow)
        {
            throw SQLite::Exception("No row to get a column from. executeStep() was not called, or returned false.");
        }
    }

    /**
     * @brief Check if there is a Column index is in the range of columns in the result.
     */
    void checkIndex(const int aIndex) const
    {
        if ((aIndex < 0) || (aIndex >= mColumnCount))
        {
            throw SQLite::Exception("Column index out of range.");
        }
    }

private:
    ///  Create prepared SQLite Statement Object
    void prepareStatement(const std::string& aQuery);
    
    ///  Get column number and create map with columns names
    void createColumnInfo();

    sqlite3* mpSQLite{}; //!< Pointer to SQLite Database Connection Handle
    TStatementPtr mpStatement{}; //!< Shared Pointer to the prepared SQLite Statement Object

    /// Shared Pointer to this object.
    /// Allows RowIterator to execute next step
    TRowPtr mpRowExecutor{};

    int mColumnCount = 0; //!< Number of columns in the result of the prepared statement
    bool mbHasRow = false; //!< true when a row has been fetched with executeStep()
    bool mbDone = false; //!< true when the last executeStep() had no more row to fetch

    /// Map of columns index by name (mutable so getColumnIndex can be const)
    mutable TColumnsMap mColumnNames{};
};


}  // namespace SQLite
