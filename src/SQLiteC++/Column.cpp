/**
 * @file  Column.cpp
 * @brief A SQLite Column is a result field in a certain row.
 *
 * Copyright (c) 2012 Sebastien Rombauts (sebastien dot rombauts at gmail dot com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include "Column.h"

#include "Database.h"
#include <iostream>

namespace SQLite
{

// Encapsulation of a Column in a Row of the result.
Column::Column(sqlite3* apSQLite, sqlite3_stmt* apStmt, int aIndex) throw() : // nothrow
    mpSQLite(apSQLite),
    mpStmt(apStmt),
    mIndex(aIndex)
{
}

Column::~Column(void) throw() // nothrow
{
}

// Return the integer value of the column specified by its index starting at 0
int Column::getInt(void) const // throw(SQLite::Exception)
{
    return sqlite3_column_int(mpStmt, mIndex);
}

// Return the 64bits integer value of the column specified by its index starting at 0
sqlite3_int64 Column::getInt64(void) const // throw(SQLite::Exception)
{
    return sqlite3_column_int64(mpStmt, mIndex);
}

// Return the double value of the column specified by its index starting at 0
double Column::getDouble(void) const // throw(SQLite::Exception)
{
    return sqlite3_column_double(mpStmt, mIndex);
}

// Return the text value (NULL terminated string) of the column specified by its index starting at 0
const char * Column::getText(void) const // throw(SQLite::Exception)
{
    return (const char*)sqlite3_column_text(mpStmt, mIndex);
}


};  // namespace SQLite


// Standard std::ostream inserter
std::ostream& operator<<(std::ostream &stream, SQLite::Column const& column)
{
    stream << column.getText();
    return stream;
}
