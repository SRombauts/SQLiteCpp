/**
 * @file    Statement.cpp
 * @ingroup SQLiteCpp
 * @brief   A prepared SQLite Statement is a compiled SQL query ready to be executed, pointing to a row of result.
 *
 * Copyright (c) 2012-2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include "Statement.h"

#include "Database.h"
#include "Column.h"

namespace SQLite
{

// Compile and register the SQL query for the provided SQLite Database Connection
Statement::Statement(Database &aDatabase, const char* apQuery) : // throw(SQLite::Exception)
    mQuery(apQuery),
    mStmtPtr(aDatabase.mpSQLite, mQuery), // prepare the SQL query, and ref count (needs Database friendship)
    mColumnCount(0),
    mbOk(false),
    mbDone(false)
{
    mColumnCount = sqlite3_column_count(mStmtPtr);
}

// Finalize and unregister the SQL query from the SQLite Database Connection.
Statement::~Statement(void) throw() // nothrow
{
    // the finalization will be done by the destructor of the last shared pointer
}

// Reset the statement to make it ready for a new execution
void Statement::reset(void) // throw(SQLite::Exception)
{
    mbOk = false;
    mbDone = false;
    int ret = sqlite3_reset(mStmtPtr);
    check(ret);
}

// Bind an int value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const int& aValue) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_int(mStmtPtr, aIndex, aValue);
    check(ret);
}

// Bind a 64bits int value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const sqlite3_int64& aValue) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_int64(mStmtPtr, aIndex, aValue);
    check(ret);
}

// Bind a double (64bits float) value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const double& aValue) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_double(mStmtPtr, aIndex, aValue);
    check(ret);
}

// Bind a string value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const std::string& aValue) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_text(mStmtPtr, aIndex, aValue.c_str(), aValue.size(), SQLITE_TRANSIENT);
    check(ret);
}

// Bind a text value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const char* apValue) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_text(mStmtPtr, aIndex, apValue, -1, SQLITE_TRANSIENT);
    check(ret);
}

// Bind a binary blob value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const void* apValue, const int aSize) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_blob(mStmtPtr, aIndex, apValue, aSize, SQLITE_TRANSIENT);
    check(ret);
}

// Bind a NULL value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex) // throw(SQLite::Exception)
{
    int ret = sqlite3_bind_null(mStmtPtr, aIndex);
    check(ret);
}


// Bind an int value to a parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const char* apName, const int& aValue) // throw(SQLite::Exception)
{
    int index = sqlite3_bind_parameter_index(mStmtPtr, apName);
    int ret   = sqlite3_bind_int(mStmtPtr, index, aValue);
    check(ret);
}

// Bind a 64bits int value to a parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const char* apName, const sqlite3_int64& aValue) // throw(SQLite::Exception)
{
    int index = sqlite3_bind_parameter_index(mStmtPtr, apName);
    int ret   = sqlite3_bind_int64(mStmtPtr, index, aValue);
    check(ret);
}

// Bind a double (64bits float) value to a parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const char* apName, const double& aValue) // throw(SQLite::Exception)
{
    int index = sqlite3_bind_parameter_index(mStmtPtr, apName);
    int ret   = sqlite3_bind_double(mStmtPtr, index, aValue);
    check(ret);
}

// Bind a string value to a parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const char* apName, const std::string& aValue) // throw(SQLite::Exception)
{
    int index = sqlite3_bind_parameter_index(mStmtPtr, apName);
    int ret   = sqlite3_bind_text(mStmtPtr, index, aValue.c_str(), aValue.size(), SQLITE_TRANSIENT);
    check(ret);
}

// Bind a text value to a parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const char* apName, const char* apValue) // throw(SQLite::Exception)
{
    int index = sqlite3_bind_parameter_index(mStmtPtr, apName);
    int ret   = sqlite3_bind_text(mStmtPtr, index, apValue, -1, SQLITE_TRANSIENT);
    check(ret);
}

// Bind a binary blob value to a parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const char* apName, const void* apValue, const int aSize) // throw(SQLite::Exception)
{
    int index = sqlite3_bind_parameter_index(mStmtPtr, apName);
    int ret   = sqlite3_bind_blob(mStmtPtr, index, apValue, aSize, SQLITE_TRANSIENT);
    check(ret);
}

// Bind a NULL value to a parameter "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const char* apName) // throw(SQLite::Exception)
{
    int index = sqlite3_bind_parameter_index(mStmtPtr, apName);
    int ret   = sqlite3_bind_null(mStmtPtr, index);
    check(ret);
}


// Execute a step of the query to fetch one row of results
bool Statement::executeStep(void) // throw(SQLite::Exception)
{
    if (false == mbDone)
    {
        int ret = sqlite3_step(mStmtPtr);
        if (SQLITE_ROW == ret) // one row is ready : call getColumn(N) to access it
        {
            mbOk = true;
        }
        else if (SQLITE_DONE == ret) // no (more) row ready : the query has finished executing
        {
            mbOk = false;
            mbDone = true;
        }
        else
        {
            mbOk = false;
            mbDone = false;
            throw SQLite::Exception(sqlite3_errmsg(mStmtPtr));
        }
    }
    else
    {
        throw SQLite::Exception("Statement need to be reseted");
    }

    return mbOk; // true only if one row is accessible by getColumn(N)
}

// Execute a one-step query with no expected result
int Statement::exec(void) // throw(SQLite::Exception)
{
    if (false == mbDone)
    {
        int ret = sqlite3_step(mStmtPtr);
        if (SQLITE_DONE == ret) // the statement has finished executing successfully
        {
            mbOk = false;
            mbDone = true;
        }
        else if (SQLITE_ROW == ret)
        {
            mbOk = false;
            mbDone = false;
            throw SQLite::Exception("exec() does not expect results");
        }
        else
        {
            mbOk = false;
            mbDone = false;
            throw SQLite::Exception(sqlite3_errmsg(mStmtPtr));
        }
    }
    else
    {
        throw SQLite::Exception("Statement need to be reseted");
    }

    // Return the number of rows modified by those SQL statements (INSERT, UPDATE or DELETE)
    return sqlite3_changes(mStmtPtr);
}

// Return a copy of the column data specified by its index starting at 0
// (use the Column copy-constructor)
Column Statement::getColumn(const int aIndex) // throw(SQLite::Exception)
{
    if (false == mbOk)
    {
        throw SQLite::Exception("No row to get a column from");
    }
    else if ((aIndex < 0) || (aIndex >= mColumnCount))
    {
        throw SQLite::Exception("Column index out of range");
    }

    // Share the Statement Object handle with the new Column created
    return Column(mStmtPtr, aIndex);
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

    return (SQLITE_NULL == sqlite3_column_type(mStmtPtr, aIndex));
}

// Check if aRet equal SQLITE_OK, else throw a SQLite::Exception with the SQLite error message
void Statement::check(const int aRet) const // throw(SQLite::Exception)
{
    if (SQLITE_OK != aRet)
    {
        throw SQLite::Exception(sqlite3_errmsg(mStmtPtr));
    }
}


////////////////////////////////////////////////////////////////////////////////
// Internal class : shared pointer to the sqlite3_stmt SQLite Statement Object
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Prepare the statement and initialize its reference counter
 *
 * @param[in] apSQLite  The sqlite3 database connexion
 * @param[in] aQuery    The SQL query string to prepare
 */
Statement::Ptr::Ptr(sqlite3* apSQLite, std::string& aQuery) :
    mpSQLite(apSQLite),
    mpStmt(NULL),
    mpRefCount(NULL)
{
    int ret = sqlite3_prepare_v2(apSQLite, aQuery.c_str(), aQuery.size(), &mpStmt, NULL);
    if (SQLITE_OK != ret)
    {
        throw SQLite::Exception(sqlite3_errmsg(mpSQLite));
    }
    // Initialize the reference counter of the sqlite3_stmt :
    // used to share the mStmtPtr between Statement and Column objects;
    // This is needed to enable Column objects to live longer than the Statement objet it refers to.
    mpRefCount = new unsigned int(1);
}

/**
 * @brief Copy constructor increments the ref counter
 *
 * @param[in] aPtr Pointer to copy
 */
Statement::Ptr::Ptr(const Statement::Ptr& aPtr) :
    mpSQLite(aPtr.mpSQLite),
    mpStmt(aPtr.mpStmt),
    mpRefCount(aPtr.mpRefCount)
{
    assert(NULL != mpRefCount);
    assert(0 != *mpRefCount);

    // Increment the reference counter of the sqlite3_stmt,
    // asking not to finalize the sqlite3_stmt during the lifetime of the new objet
    ++(*mpRefCount);
}

/**
 * @brief Decrement the ref counter and finalize the sqlite3_stmt when it reaches 0
 */
Statement::Ptr::~Ptr(void) throw() // nothrow
{
    assert(NULL != mpRefCount);
    assert(0 != *mpRefCount);

    // Decrement and check the reference counter of the sqlite3_stmt
    --(*mpRefCount);
    if (0 == *mpRefCount)
    {
        // If count reaches zero, finalize the sqlite3_stmt,
        // as no Statement not Column objet use it anymore
        int ret = sqlite3_finalize(mpStmt);
        // Never throw an exception in a destructor
        //std::cout << sqlite3_errmsg(mpSQLite) << std::endl;
        SQLITE_CPP_ASSERT (SQLITE_OK == ret);

        // and delete the reference counter
        delete mpRefCount;
        mpRefCount = NULL;
        mpStmt = NULL;
    }
    // else, the finalization will be done later, by the last object
}

}  // namespace SQLite
