/**
 * @file  Column.cpp
 * @brief Encapsulation of a Column in a Row of the result.
 *
 * Copyright (c) 2012 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include "Column.h"

#include <iostream>

namespace SQLite
{

// Encapsulation of a Column in a Row of the result.
Column::Column(sqlite3* apSQLite, sqlite3_stmt* apStmt, unsigned int* apStmtRefCount, int aIndex) throw() : // nothrow
    mpSQLite(apSQLite),
    mpStmt(apStmt),
    mpStmtRefCount(apStmtRefCount),
    mIndex(aIndex)
{
    (*mpStmtRefCount)++;
}

// Finalize and unregister the SQL query from the SQLite Database Connection.
Column::~Column(void) throw() // nothrow
{
    // Decrement and check the reference counter
    (*mpStmtRefCount)--;
    if (0 == *mpStmtRefCount)
    {
        // When count reaches zero, dealloc and finalize the statement
        delete mpStmtRefCount;

        int ret = sqlite3_finalize(mpStmt);
        if (SQLITE_OK != ret)
        {
            // Never throw an exception in a destructor
            //std::cout << sqlite3_errmsg(mpSQLite);
        }
        mpStmt = NULL;
    }
}

// Return the integer value of the column specified by its index starting at 0
int Column::getInt(void) const throw() // nothrow
{
    return sqlite3_column_int(mpStmt, mIndex);
}

// Return the 64bits integer value of the column specified by its index starting at 0
sqlite3_int64 Column::getInt64(void) const throw() // nothrow
{
    return sqlite3_column_int64(mpStmt, mIndex);
}

// Return the double value of the column specified by its index starting at 0
double Column::getDouble(void) const throw() // nothrow
{
    return sqlite3_column_double(mpStmt, mIndex);
}

// Return a pointer to the text value (NULL terminated string) of the column specified by its index starting at 0
const char* Column::getText(void) const throw() // nothrow
{
    return (const char*)sqlite3_column_text(mpStmt, mIndex);
}


// Standard std::ostream inserter
std::ostream& operator<<(std::ostream &stream, const Column& column)
{
    stream << column.getText();
    return stream;
}

}  // namespace SQLite
