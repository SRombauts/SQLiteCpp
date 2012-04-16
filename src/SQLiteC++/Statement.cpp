/**
 * @file  Statement.cpp
 * @brief A prepared SQLite Statement is a compiled SQL query ready to be executed.
 *
 * Copyright (c) 2012 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include "Statement.h"

#include "Database.h"
#include <iostream>

namespace SQLite
{

// Compile and register the SQL query for the provided SQLite Database Connection
Statement::Statement(Database &aDatabase, const char* apQuery) : // throw(SQLite::Exception)
    mpStmt(NULL),
    mpStmtRefCount(NULL),
    mpSQLite(aDatabase.mpSQLite),
    mQuery(apQuery),
    mbOk(false),
    mbDone(false)
{
    int ret = sqlite3_prepare_v2(mpSQLite, mQuery.c_str(), mQuery.size(), &mpStmt, NULL);
    check(ret);
    mColumnCount = sqlite3_column_count(mpStmt);
    mpStmtRefCount = new unsigned int;
    *mpStmtRefCount = 1;
}

// Finalize and unregister the SQL query from the SQLite Database Connection.
Statement::~Statement(void) throw() // nothrow
{
    (*mpStmtRefCount)--;
    if (0 == *mpStmtRefCount)
    {
        int ret = sqlite3_finalize(mpStmt);
        if (SQLITE_OK != ret)
        {
            // Never throw an exception in a destructor
            //std::cout << sqlite3_errmsg(mpSQLite);
        }
        mpStmt = NULL;
    }
}

// Reset the statement to make it ready for a new execution
void Statement::reset(void) // throw(SQLite::Exception)
{
    mbOk = false;
    mbDone = false;
    int ret = sqlite3_reset(mpStmt);
    check(ret);
}

// Bind an int value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const int& aValue) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_int(mpStmt, aIndex, aValue);
    check(ret);
}

// Bind a 64bits int value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const sqlite3_int64& aValue) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_int64(mpStmt, aIndex, aValue);
    check(ret);
}

// Bind a double (64bits float) value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const double& aValue) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_double(mpStmt, aIndex, aValue);
    check(ret);
}

// Bind a string value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const std::string& aValue) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_text(mpStmt, aIndex, aValue.c_str(), aValue.size(), SQLITE_TRANSIENT);
    check(ret);
}

// Bind a text value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const char* apValue) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_text(mpStmt, aIndex, apValue, -1, SQLITE_TRANSIENT);
    check(ret);
}

// Bind a NULL value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_null(mpStmt, aIndex);
    check(ret);
}


// Bind an int value to a parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const char* apName, const int& aValue) // throw(SQLite::Exception)
{
    int index = sqlite3_bind_parameter_index(mpStmt, apName);
    int ret   = sqlite3_bind_int(mpStmt, index, aValue);
    check(ret);
}

// Bind a 64bits int value to a parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const char* apName, const sqlite3_int64& aValue) // throw(SQLite::Exception)
{
    int index = sqlite3_bind_parameter_index(mpStmt, apName);
    int ret   = sqlite3_bind_int64(mpStmt, index, aValue);
    check(ret);
}

// Bind a double (64bits float) value to a parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const char* apName, const double& aValue) // throw(SQLite::Exception)
{
    int index = sqlite3_bind_parameter_index(mpStmt, apName);
    int ret   = sqlite3_bind_double(mpStmt, index, aValue);
    check(ret);
}

// Bind a string value to a parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const char* apName, const std::string& aValue) // throw(SQLite::Exception)
{
    int index = sqlite3_bind_parameter_index(mpStmt, apName);
    int ret   = sqlite3_bind_text(mpStmt, index, aValue.c_str(), aValue.size(), SQLITE_TRANSIENT);
    check(ret);
}

// Bind a text value to a parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const char* apName, const char* apValue) // throw(SQLite::Exception)
{
    int index = sqlite3_bind_parameter_index(mpStmt, apName);
    int ret   = sqlite3_bind_text(mpStmt, index, apValue, -1, SQLITE_TRANSIENT);
    check(ret);
}

// Bind a NULL value to a parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const char* apName) // throw(SQLite::Exception)
{
    int index = sqlite3_bind_parameter_index(mpStmt, apName);
    int ret   = sqlite3_bind_null(mpStmt, index);
    check(ret);
}


// Execute a step of the query to fetch one row of results
bool Statement::executeStep(void) // throw(SQLite::Exception)
{
    if (false == mbDone)
    {
        int ret = sqlite3_step(mpStmt);
        if (SQLITE_ROW == ret)
        {
            mbOk = true;
        }
        else if (SQLITE_DONE == ret)
        {
            mbOk = false;
            mbDone = true;
        }
        else
        {
            throw SQLite::Exception(sqlite3_errmsg(mpSQLite));
        }
    }
    else
    {
        throw SQLite::Exception("Statement need to be reseted");
    }

    return mbOk;
}

// Return a copy of the column data specified by its index starting at 0
Statement::Column Statement::getColumn(const int aIndex) const // throw(SQLite::Exception)
{
    if (false == mbOk)
    {
        throw SQLite::Exception("No row to get a column from");
    }
    else if ((aIndex < 0) || (aIndex >= mColumnCount))
    {
        throw SQLite::Exception("Column index out of range");
    }

    return Column(mpSQLite, mpStmt, mpStmtRefCount, aIndex);
}

// Test if the column is NULL
bool Statement::isColumnNull(const int aIndex) const // throw(SQLite::Exception)
{
    if (false == mbOk)
    {
        throw SQLite::Exception("No row to get a column from");
    }
    else if ((aIndex < 0) || (aIndex >= mColumnCount))
    {
        throw SQLite::Exception("Column index out of range");
    }

    return (SQLITE_NULL == sqlite3_column_type(mpStmt, aIndex));
}


// Check if aRet equal SQLITE_OK, else throw a SQLite::Exception with the SQLite error message
void Statement::check(const int aRet) const // throw(SQLite::Exception)
{
    if (SQLITE_OK != aRet)
    {
        throw SQLite::Exception(sqlite3_errmsg(mpSQLite));
    }
}


////////////////////////////////////////////////////////////////////////////////
// Implementation of the inner class Statement::Column
//

// Encapsulation of a Column in a Row of the result.
Statement::Column::Column(sqlite3* apSQLite, sqlite3_stmt* apStmt, unsigned int* apStmtRefCount, int aIndex) throw() : // nothrow
    mpSQLite(apSQLite),
    mpStmt(apStmt),
    mpStmtRefCount(apStmtRefCount),
    mIndex(aIndex)
{
    (*mpStmtRefCount)++;
}

Statement::Column::~Column(void) throw() // nothrow
{
    (*mpStmtRefCount)--;
    if (0 == *mpStmtRefCount)
    {
        int ret = sqlite3_finalize(mpStmt);
        if (SQLITE_OK != ret)
        {
            throw SQLite::Exception(sqlite3_errmsg(mpSQLite));
        }
        mpStmt = NULL;
    }
}

// Return the integer value of the column specified by its index starting at 0
int Statement::Column::getInt(void) const throw() // nothrow
{
    return sqlite3_column_int(mpStmt, mIndex);
}

// Return the 64bits integer value of the column specified by its index starting at 0
sqlite3_int64 Statement::Column::getInt64(void) const throw() // nothrow
{
    return sqlite3_column_int64(mpStmt, mIndex);
}

// Return the double value of the column specified by its index starting at 0
double Statement::Column::getDouble(void) const throw() // nothrow
{
    return sqlite3_column_double(mpStmt, mIndex);
}

// Return a pointer to the text value (NULL terminated string) of the column specified by its index starting at 0
const char* Statement::Column::getText(void) const throw() // nothrow
{
    return (const char*)sqlite3_column_text(mpStmt, mIndex);
}

// Standard std::ostream inserter
std::ostream& operator<<(std::ostream &stream, const Statement::Column& column)
{
    stream << column.getText();
    return stream;
}

};  // namespace SQLite
