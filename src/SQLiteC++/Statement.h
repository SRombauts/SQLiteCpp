/**
 * @file  Statement.h
 * @brief A prepared SQLite Statement is a compiled SQL query ready to be executed.
 *
 * Copyright (c) 2012-2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
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
class Column;

/**
 * @brief RAII encapsulation of a prepared SQLite Statement.
 *
 * A Statement is a compiled SQL query ready to be executed step by step
 * to provide results one row at a time.
 *
 * Resource Acquisition Is Initialization (RAII) means that the Statement
 * is compiled in the constructor and finalized in the destructor, so that there is
 * no need to worry about memory management or the validity of the underlying SQLite Statement.
 */
class Statement
{
public:
    /**
     * @brief Compile and register the SQL query for the provided SQLite Database Connection
     *
     * Exception is thrown in case of error, then the Statement object is NOT constructed.
     */
    explicit Statement(Database &aDatabase, const char* apQuery); // throw(SQLite::Exception);

    /**
     * @brief Finalize and unregister the SQL query from the SQLite Database Connection.
     */
    virtual ~Statement(void) throw(); // nothrow

    /**
     * @brief Reset the statement to make it ready for a new execution.
     */
    void reset(void); // throw(SQLite::Exception);

    ////////////////////////////////////////////////////////////////////////////
    // Bind a value to a parameter of the SQL statement,
    // in the form "?" (unnamed), "?NNN", ":VVV", "@VVV" or "$VVV".
    //
    // Can use the parameter index, starting from "1", to the higher NNN value,
    // or the complete parameter name "?NNN", ":VVV", "@VVV" or "$VVV"
    // (prefixed with the corresponding sign "?", ":", "@" or "$")

    /**
     * @brief Bind an int value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement (aIndex >= 1)
     */
    void bind(const int aIndex, const int&           aValue)  ; // throw(SQLite::Exception);
    /**
     * @brief Bind a 64bits int value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement (aIndex >= 1)
     */
    void bind(const int aIndex, const sqlite3_int64& aValue)  ; // throw(SQLite::Exception);
    /**
     * @brief Bind a double (64bits float) value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement (aIndex >= 1)
     */
    void bind(const int aIndex, const double&        aValue)  ; // throw(SQLite::Exception);
    /**
     * @brief Bind a string value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement (aIndex >= 1)
     */
    void bind(const int aIndex, const std::string&   aValue)  ; // throw(SQLite::Exception);
    /**
     * @brief Bind a text value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement (aIndex >= 1)
     */
    void bind(const int aIndex, const char*          apValue) ; // throw(SQLite::Exception);
    /**
     * @brief Bind a NULL value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement (aIndex >= 1)
     */
    void bind(const int aIndex); // throw(SQLite::Exception);

    /**
     * @brief Bind an int value to a named parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement (aIndex >= 1)
     */
    void bind(const char* apName, const int&            aValue)  ; // throw(SQLite::Exception);
    /**
     * @brief Bind a 64bits int value to a named parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement (aIndex >= 1)
     */
    void bind(const char* apName, const sqlite3_int64&  aValue)  ; // throw(SQLite::Exception);
    /**
     * @brief Bind a double (64bits float) value to a named parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement (aIndex >= 1)
     */
    void bind(const char* apName, const double&         aValue)  ; // throw(SQLite::Exception);
    /**
     * @brief Bind a string value to a named parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement (aIndex >= 1)
     */
    void bind(const char* apName, const std::string&    aValue)  ; // throw(SQLite::Exception);
    /**
     * @brief Bind a text value to a named parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement (aIndex >= 1)
     */
    void bind(const char* apName, const char*           apValue) ; // throw(SQLite::Exception);
    /**
     * @brief Bind a NULL value to a named parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement (aIndex >= 1)
     */
    void bind(const char* apName); // throw(SQLite::Exception); // bind NULL value

    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Execute a step of the prepared query to fetch one row of results.
     *
     * @see exec() execute a one-step prepared statement with no expected result
     * @see Database::exec() is a shortcut to execute one or multiple statements without results
     *
     * @return - true  (SQLITE_ROW)  if there is another row ready : you can call getColumn(N) to get it
     *                               then you have to call executeStep() again to fetch more rows until the query is finished
     *         - false (SQLITE_DONE) if the query has finished executing : there is no (more) row of result
     *                               (case of a query with no result, or after N rows fetched successfully)
     *
     * @throw SQLite::Exception in case of error
     */
    bool executeStep(void); // throw(SQLite::Exception);

    /**
     * @brief Execute a one-step query with no expected result.
     *
     *  This exec() method is to use with precompiled statement that does not fetch results (INSERT, UPDATE, DELETE...).
     * It is intended for similar usage as Database::exec(), but is able to reuse the precompiled underlying SQLite statement
     * for better performances.
     *
     * @see executeStep() execute a step of the prepared query to fetch one row of results
     * @see Database::exec() is a shortcut to execute one or multiple statements without results
     *
     * @return number of row modified by this SQL statement (INSERT, UPDATE or DELETE)
     *
     * @throw SQLite::Exception in case of error, or if row of results are returned !
     */
    int exec(void); // throw(SQLite::Exception);

    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Return a copie of the column data specified by its index starting at 0 (aIndex >= 0)
     *
     * @warning The resulting Column object must not be copied or memorized as it is only valid for a short time,
     *          only while the row from the Statement remains valid, that is only until next executeStep.
     *          Thus, you should instead extract immediately its data (getInt()...) and use or copy this data
     *          for any later usage.
     */
    Column  getColumn(const int aIndex) const; // throw(SQLite::Exception);

    /**
     * @brief Test if the column is NULL
     */
    bool    isColumnNull(const int aIndex) const; // throw(SQLite::Exception);

    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief UTF-8 SQL Query.
     */
    inline const std::string& getQuery(void) const
    {
        return mQuery;
    }

    /**
     * @brief Return the number of columns in the result set returned by the prepared statement
     */
    inline int getColumnCount(void) const
    {
        return mColumnCount;
    }

    /**
     * @brief True when the last row is fetched with executeStep().
     */
    inline bool isOk(void) const
    {
        return mbOk;
    }

    /**
     * @brief True when the last row is fetched with executeStep().
     */
    inline bool isDone(void) const
    {
        return mbDone;
    }

private:
    // Statement must not be copyable
    Statement(void);
    Statement(const Statement&);
    Statement& operator=(const Statement&);

    /**
     * @brief Check if a return code equals SQLITE_OK, else throw a SQLite::Exception with the SQLite error message
     */
    void check(const int aRet) const; // throw(SQLite::Exception);

private:
    sqlite3_stmt*   mpStmt;         //!< Pointer to SQLite Statement Object
    unsigned int*   mpStmtRefCount; //!< Pointer to the heap allocated reference counter of the sqlite3_stmt (to share it with Column objects)
    sqlite3*        mpSQLite;       //!< Pointer to SQLite Database Connection Handle
    std::string     mQuery;         //!< UTF-8 SQL Query
    int             mColumnCount;   //!< Number of column in the result of the prepared statement
    bool            mbOk;           //!< True when a row has been fetched with executeStep()
    bool            mbDone;         //!< True when the last executeStep() had no more row to fetch
};

}  // namespace SQLite
