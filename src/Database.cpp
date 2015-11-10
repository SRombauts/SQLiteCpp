/**
 * @file    Database.cpp
 * @ingroup SQLiteCpp
 * @brief   Management of a SQLite Database Connection.
 *
 * Copyright (c) 2012-2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include <SQLiteCpp/Database.h>

#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Assertion.h>
#include <SQLiteCpp/Exception.h>

#include <string>

#ifndef SQLITE_DETERMINISTIC
#define SQLITE_DETERMINISTIC 0x800
#endif // SQLITE_DETERMINISTIC


namespace SQLite
{


// Open the provided database UTF-8 filename with SQLITE_OPEN_xxx provided flags.
Database::Database(const char* apFilename,
                   const int   aFlags         /* = SQLITE_OPEN_READONLY*/,
                   const int   aBusyTimeoutMs /* = 0 */,
                   const char* apVfs          /* = NULL*/) :
    mpSQLite(NULL),
    mFilename(apFilename)
{
    const int ret = sqlite3_open_v2(apFilename, &mpSQLite, aFlags, apVfs);
    if (SQLITE_OK != ret)
    {
        std::string strerr = sqlite3_errstr(ret);
        sqlite3_close(mpSQLite); // close is required even in case of error on opening
        throw SQLite::Exception(strerr);
    }

    if (aBusyTimeoutMs > 0)
    {
        setBusyTimeout(aBusyTimeoutMs);
    }
}

// Open the provided database UTF-8 filename with SQLITE_OPEN_xxx provided flags.
Database::Database(const std::string& aFilename,
                   const int          aFlags         /* = SQLITE_OPEN_READONLY*/,
                   const int          aBusyTimeoutMs /* = 0 */,
                   const std::string& aVfs           /* = "" */) :
    mpSQLite(NULL),
    mFilename(aFilename)
{
    const int ret = sqlite3_open_v2(aFilename.c_str(), &mpSQLite, aFlags, aVfs.empty() ? NULL : aVfs.c_str());
    if (SQLITE_OK != ret)
    {
        std::string strerr = sqlite3_errstr(ret);
        sqlite3_close(mpSQLite); // close is required even in case of error on opening
        throw SQLite::Exception(strerr);
    }

    if (aBusyTimeoutMs > 0)
    {
        setBusyTimeout(aBusyTimeoutMs);
    }
}

// Close the SQLite database connection.
Database::~Database() noexcept // nothrow
{
    const int ret = sqlite3_close(mpSQLite);

    // Avoid unreferenced variable warning when build in release mode
    (void) ret;

    // Only case of error is SQLITE_BUSY: "database is locked" (some statements are not finalized)
    // Never throw an exception in a destructor :
    SQLITECPP_ASSERT(SQLITE_OK == ret, "database is locked");  // See SQLITECPP_ENABLE_ASSERT_HANDLER
}

/**
 * @brief Set a busy handler that sleeps for a specified amount of time when a table is locked.
 *
 *  This is useful in multithreaded program to handle case where a table is locked for writting by a thread.
 * Any other thread cannot access the table and will receive a SQLITE_BUSY error:
 * setting a timeout will wait and retry up to the time specified before returning this SQLITE_BUSY error.
 *  Reading the value of timeout for current connection can be done with SQL query "PRAGMA busy_timeout;".
 *  Default busy timeout is 0ms.
 *
 * @param[in] aBusyTimeoutMs    Amount of milliseconds to wait before returning SQLITE_BUSY
 *
 * @throw SQLite::Exception in case of error
 */
void Database::setBusyTimeout(const int aBusyTimeoutMs) noexcept // nothrow
{
    const int ret = sqlite3_busy_timeout(mpSQLite, aBusyTimeoutMs);
    check(ret);
}

// Shortcut to execute one or multiple SQL statements without results (UPDATE, INSERT, ALTER, COMMIT, CREATE...).
int Database::exec(const char* apQueries)
{
    const int ret = sqlite3_exec(mpSQLite, apQueries, NULL, NULL, NULL);
    check(ret);

    // Return the number of rows modified by those SQL statements (INSERT, UPDATE or DELETE only)
    return sqlite3_changes(mpSQLite);
}

// Shortcut to execute a one step query and fetch the first column of the result.
// WARNING: Be very careful with this dangerous method: you have to
// make a COPY OF THE result, else it will be destroy before the next line
// (when the underlying temporary Statement and Column objects are destroyed)
// this is an issue only for pointer type result (ie. char* and blob)
// (use the Column copy-constructor)
Column Database::execAndGet(const char* apQuery)
{
    Statement query(*this, apQuery);
    (void)query.executeStep(); // Can return false if no result, which will throw next line in getColumn()
    return query.getColumn(0);
}

// Shortcut to test if a table exists.
bool Database::tableExists(const char* apTableName)
{
    Statement query(*this, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?");
    query.bind(1, apTableName);
    (void)query.executeStep(); // Cannot return false, as the above query always return a result
    const int Nb = query.getColumn(0);
    return (1 == Nb);
}

// Attach a custom function to your sqlite database. Assumes UTF8 text representation.
// Parameter details can be found here: http://www.sqlite.org/c3ref/create_function.html
void Database::createFunction(const char*   apFuncName,
                              int           aNbArg,
                              bool          abDeterministic,
                              void*         apApp,
                              void        (*apFunc)(sqlite3_context *, int, sqlite3_value **),
                              void        (*apStep)(sqlite3_context *, int, sqlite3_value **),
                              void        (*apFinal)(sqlite3_context *),   // NOLINT(readability/casting)
                              void        (*apDestroy)(void *))
{
    int TextRep = SQLITE_UTF8;
    // optimization if deterministic function (e.g. of nondeterministic function random())
    if (abDeterministic) {
        TextRep = TextRep|SQLITE_DETERMINISTIC;
    }
    const int ret = sqlite3_create_function_v2(mpSQLite, apFuncName, aNbArg, TextRep,
                                               apApp, apFunc, apStep, apFinal, apDestroy);
    check(ret);
}

// Load an extension into the sqlite database. Only affects the current connection.
// Parameter details can be found here: http://www.sqlite.org/c3ref/load_extension.html
void Database::loadExtension(const char* apExtensionName,
         const char *apEntryPointName)
{
#ifdef SQLITE_OMIT_LOAD_EXTENSION
#
    throw std::runtime_error("sqlite extensions are disabled");
#
#else
#
    int ret = sqlite3_enable_load_extension(mpSQLite, 1);

    check(ret);

    ret = sqlite3_load_extension(mpSQLite, apExtensionName, apEntryPointName, 0);

    check(ret);
#
#endif
}

}  // namespace SQLite
