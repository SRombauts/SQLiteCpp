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
    mpSQLite        (apSQLite),
    mpStmt          (apStmt),
    mpStmtRefCount  (apStmtRefCount),
    mIndex          (aIndex)
{
    // Increment the reference counter of the sqlite3_stmt,
    // telling the Statement object not to finalize the sqlite3_stmt during the lifetime of this Column objet
    (*mpStmtRefCount)++;
}

// Copy constructor
Column::Column(const Column& copy) throw() : // nothrow
    mpSQLite        (copy.mpSQLite),
    mpStmt          (copy.mpStmt),
    mpStmtRefCount  (copy.mpStmtRefCount),
    mIndex          (copy.mIndex)
{
    // Increment the reference counter of the sqlite3_stmt,
    // telling the Statement object not to finalize the sqlite3_stmt during the lifetime of this Column objet
    (*mpStmtRefCount)++;
}

// Finalize and unregister the SQL query from the SQLite Database Connection.
Column::~Column(void) throw() // nothrow
{
    // Decrement and check the reference counter of the sqlite3_stmt
    (*mpStmtRefCount)--;
    if (0 == *mpStmtRefCount)
    {
        // When count reaches zero, finalize the sqlite3_stmt, as no Column nor Statement object use it any more
        int ret = sqlite3_finalize(mpStmt);
        if (SQLITE_OK != ret)
        {
            // Never throw an exception in a destructor
            //std::cout << sqlite3_errmsg(mpSQLite);
        }
        mpStmt = NULL;

        // and delete the reference counter
        delete mpStmtRefCount;
    }
    // else, the finalization will be done by the Statement or another Column object (the last one)
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
