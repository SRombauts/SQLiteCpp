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
    // Forward declaration of the inner class "Column"
    class Column;

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
    void bind(const int aIndex); // throw(SQLite::Exception); // bind NULL value

    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Execute a step of the query to fetch one row of results.
     */
    bool executeStep(void); // throw(SQLite::Exception);

    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Return a copie of the column data specified by its index starting at 0 (aIndex >= 0)
     *
     * @warning The resulting Column object must not be copied or memorized as it is only valid for a short time,
     *          only while the row from the Statement remains valid, that is only until next executeStep
     */
    Column  getColumn (const int aIndex) const; // throw(SQLite::Exception);

    /**
     * @brief Test if the column is NULL
     */
    bool    isColumnNull   (const int aIndex) const; // throw(SQLite::Exception);

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

public:
    /**
     * @brief Encapsulation of a Column in a Row of the result.
     *
     * A Column is a particular field of SQLite data in the current row of result of the Statement.
     *
     * @warning A Column object must not be copied or memorized as it is only valid for a short time,
     *          only while the row from the Statement remains valid, that is only until next executeStep
     */
    class Column
    {
        /// Standard std::ostream inserter
        friend std::ostream& operator<<(std::ostream &stream, const Column& column);

    public:
        /**
         * @brief Compile and register the SQL query for the provided SQLite Database Connection
         */
        explicit Column(sqlite3* apSQLite, sqlite3_stmt* apStmt, int aIndex) throw(); // nothrow
        /// Basic destructor
        virtual ~Column(void) throw(); // nothrow

        /// Return the integer value of the column.
        int             getInt   (void) const throw();
        // Return the 64bits integer value of the column.
        sqlite3_int64   getInt64 (void) const throw();
        // Return the double (64bits float) value of the column.
        double          getDouble(void) const throw();
        // Return the text value (NULL terminated string) of the column.
        const char*     getText  (void) const throw();

        /// Inline cast operator to int
        inline operator const int() const
        {
            return getInt();
        }
        /// Inline cast operator to 64bits integer
        inline operator const sqlite3_int64() const
        {
            return getInt64();
        }
        /// Inline cast operator to double
        inline operator const double() const
        {
            return getDouble();
        }
        /// Inline cast operator to char*
        inline operator const char*() const
        {
            return getText();
        }
        /// Inline cast operator to std::string
        inline operator const std::string() const
        {
            return getText();
        }

    private:
        // Column is copyable, but copy should not be used elsewhere than in return form getColumn
        Column(void);
        Column& operator=(const Column&);

    private:
        sqlite3*        mpSQLite;   //!< Pointer to SQLite Database Connection Handle
        sqlite3_stmt*   mpStmt;     //!< Pointeur to SQLite Statement Object
        int             mIndex;     //!< Index of the column in the row of result
    };

private:
    // Statement must not be copyable
    Statement(void);
    Statement(const Statement&);
    Statement& operator=(const Statement&);

    /**
     * @brief Check if aRet equal SQLITE_OK, else throw a SQLite::Exception with the SQLite error message
     */
    void check(const int aRet) const; // throw(SQLite::Exception);

private:
    sqlite3_stmt*   mpStmt;         //!< Pointeur to SQLite Statement Object
    Database&       mDatabase;      //!< Reference to the SQLite Database Connection
    std::string     mQuery;         //!< UTF-8 SQL Query
    int             mColumnCount;   //!< Number of column in the result of the prepared statement
    bool            mbOk;           //!< True when a row has been fetched with executeStep()
    bool            mbDone;         //!< True when the last executeStep() had no more row to fetch
};


};  // namespace SQLite
