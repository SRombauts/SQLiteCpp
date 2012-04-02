/**
 * @file  Column.h
 * @brief A SQLite Column is a result field in a certain row.
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
 * @brief Encapsulation of a Column in a Row of the result.
 *
 * A Column is a particular field of SQLite data in the current row of result of the Statement.
 *
 * @warning A Column object must not be copied or memorized as it is only valid for a short time,
 *          only while the row from the Statement remains valid, that is only until next executeStep
 */
class Column
{
public:
    /**
     * @brief Compile and register the SQL query for the provided SQLite Database Connection
     */
    explicit Column(sqlite3* apSQLite, sqlite3_stmt* apStmt, int aIndex) throw(); // nothrow
    /// Basic destructor
    virtual ~Column(void) throw(); // nothrow

    /// Return the integer value of the column.
    int             getInt   (void) const; // throw(SQLite::Exception);
    // Return the 64bits integer value of the column.
    sqlite3_int64   getInt64 (void) const; // throw(SQLite::Exception);
    // Return the double (64bits float) value of the column.
    double          getDouble(void) const; // throw(SQLite::Exception);
    // Return the text value (NULL terminated string) of the column.
    const char*     getText  (void) const; // throw(SQLite::Exception);

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


};  // namespace SQLite


/// Standard std::ostream inserter
std::ostream& operator<<(std::ostream &stream, SQLite::Column const& column);
