/**
 * @file    StatementExecutor.h
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

#include <SQLiteCpp/Row.h>
#include <SQLiteCpp/Exception.h>

#include <memory>
#include <string>
#include <map>

// Forward declaration to avoid inclusion of <sqlite3.h> in a header
struct sqlite3_stmt;

namespace SQLite
{

extern const int OK; ///< SQLITE_OK

/**
* @brief Base class for prepared SQLite Statement.
* 
* You should use SQLite::Statement or (if you had a reson)
* inherit this class to create your own Statement executor class.
* Either way you should look at SQLite::Statement documentation
* 
* Thread-safety: a StatementExecutor object shall not be shared by multiple threads, because :
* 1) in the SQLite "Thread Safe" mode, "SQLite can be safely used by multiple threads
*    provided that no single database connection is used simultaneously in two or more threads."
* 2) the SQLite "Serialized" mode is not supported by SQLiteC++,
*    because of the way it shares the underling SQLite precompiled statement
*    in a custom shared pointer (See the inner class "Statement::Ptr").
*    TODO Test Serialized mode after all changes to pointers
*/
class StatementExecutor
{
public:
    /// Shared pointer to SQLite Prepared Statement Object
    using TStatementPtr = std::shared_ptr<sqlite3_stmt>;

    /// Weak pointer to SQLite Prepared Statement Object
    using TStatementWeakPtr = std::weak_ptr<sqlite3_stmt>;

    /// Shared pointer to SQLite StatementExecutor
    using TRowPtr = std::shared_ptr<StatementExecutor>;

    /// Weak pointer to SQLite StatementExecutor
    using TRowWeakPtr = std::weak_ptr<StatementExecutor>;

    /// Type to store columns names and indexes
    using TColumnsMap = std::map<std::string, int, std::less<>>;

    StatementExecutor(const StatementExecutor&) = delete;
    StatementExecutor(StatementExecutor&&) = default;
    StatementExecutor& operator=(const StatementExecutor&) = delete;
    StatementExecutor& operator=(StatementExecutor&&) = default;

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

    /// Get number of rows modified by last INSERT, UPDATE or DELETE statement (not DROP table).
    int getChanges() const noexcept;

    /// Return the number of columns in the result set returned by the prepared statement
    int getColumnCount() const noexcept
    {
        return mColumnCount;
    }

    /// Get columns names with theirs ids
    const TColumnsMap& getColumnsNames() const
    {
        return mColumnNames;
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

    ////////////////////////////////////////////////////////////////////////////

    /**
    * @brief InputIterator for statement steps.
    * 
    * Remember that this iterator is changing state of StatementExecutor.
    */
    class RowIterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = Row;
        using reference = const Row&;
        using pointer = const Row*;
        using difference_type = std::ptrdiff_t;

        RowIterator() = default;
        RowIterator(TStatementWeakPtr apStatement, TRowWeakPtr apRow, uint16_t aID) :
            mpStatement(apStatement), mpRow(apRow), mID(aID), mRow(apStatement, aID) {}

        reference operator*() const
        {
            return mRow;
        }
        pointer operator->() const noexcept
        {
            return &mRow;
        }

        reference operator++() noexcept
        {
            mRow = Row(mpStatement, ++mID);
            advance();
            return mRow;
        }
        value_type operator++(int)
        {
            Row copy{ mRow };
            mRow = Row(mpStatement, ++mID);
            advance();
            return copy;
        }

        bool operator==(const RowIterator& aIt) const;
        bool operator!=(const RowIterator& aIt) const
        {
            return !(*this == aIt);
        }

    private:
        /// Executing next statement step
        void advance() noexcept;

        TStatementWeakPtr   mpStatement{};  //!< Weak pointer to SQLite Statement Object
        TRowWeakPtr         mpRow{};        //!< Weak pointer to StatementExecutor Object
        uint16_t            mID{};          //!< Current row number

        /// Internal row object storage
        Row mRow{ mpStatement, mID };
    };

    /**
    * @brief Start execution of SQLite Statement Object and return iterator to first row.
    * 
    * This function calls resets SQLite Statement Object.
    * 
    * @return RowIterator for first row of this prepared statement
    * 
    * @throws Exception is thrown in case of error, then the RowIterator object is NOT constructed.
    */
    RowIterator begin();

    /**
    * @return RowIterator to non-exisitng step
    */
    RowIterator end();

protected:
    /**
     * @brief Proteced construtor to ensure that this class is only created in derived objects
     *
     * @param[in] apSQLite  the SQLite Database Connection
     * @param[in] aQuery    an UTF-8 encoded query string
     *
     * @throws Exception is thrown in case of error, then the StatementExecutor object is NOT constructed.
     */
    explicit StatementExecutor(sqlite3* apSQLite, const std::string& aQuery);

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

    sqlite3*        mpSQLite{};     //!< Pointer to SQLite Database Connection Handle
    TStatementPtr   mpStatement{};  //!< Shared Pointer to the prepared SQLite Statement Object

    /// Shared Pointer to this object.
    /// Allows RowIterator to execute next step
    TRowPtr mpRowExecutor{};

    int     mColumnCount = 0;   //!< Number of columns in the result of the prepared statement
    bool    mbHasRow = false;   //!< true when a row has been fetched with executeStep()
    bool    mbDone = false;     //!< true when the last executeStep() had no more row to fetch

    /// Map of columns index by name (mutable so getColumnIndex can be const)
    mutable TColumnsMap mColumnNames{};
};


}  // namespace SQLite
