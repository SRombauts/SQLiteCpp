/**
 * @file    Database.h
 * @ingroup SQLiteCpp
 * @brief   Management of a SQLite Database Connection.
 *
 * Copyright (c) 2012-2015 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include <sqlite3.h>

#include <SQLiteCpp/Column.h>

#include <string>


namespace SQLite
{


/**
 * @brief RAII management of a SQLite Database Connection.
 *
 * A Database object manage a list of all SQLite Statements associated with the
 * underlying SQLite 3 database connection.
 *
 * Resource Acquisition Is Initialization (RAII) means that the Database Connection
 * is opened in the constructor and closed in the destructor, so that there is
 * no need to worry about memory management or the validity of the underlying SQLite Connection.
 *
 * Thread-safety: a Database object shall not be shared by multiple threads, because :
 * 1) in the SQLite "Thread Safe" mode, "SQLite can be safely used by multiple threads
 *    provided that no single database connection is used simultaneously in two or more threads."
 * 2) the SQLite "Serialized" mode is not supported by SQLiteC++,
 *    because of the way it shares the underling SQLite precompiled statement
 *    in a custom shared pointer (See the inner class "Statement::Ptr").
 */
class Database
{
    friend class Statement; // Give Statement constructor access to the mpSQLite Connection Handle

public:
    /**
     * @brief Open the provided database UTF-8 filename.
     *
     * Uses sqlite3_open_v2() with readonly default flag, which is the opposite behavior
     * of the old sqlite3_open() function (READWRITE+CREATE).
     * This makes sense if you want to use it on a readonly filesystem
     * or to prevent creation of a void file when a required file is missing.
     *
     * Exception is thrown in case of error, then the Database object is NOT constructed.
     *
     * @param[in] apFilename        UTF-8 path/uri to the database file ("filename" sqlite3 parameter)
     * @param[in] aFlags            SQLITE_OPEN_READONLY/SQLITE_OPEN_READWRITE/SQLITE_OPEN_CREATE...
     * @param[in] aBusyTimeoutMs    Amount of milliseconds to wait before returning SQLITE_BUSY (see setBusyTimeout())
     * @param[in] apVfs             UTF-8 name of custom VFS to use, or nullptr for sqlite3 default
     *
     * @throw SQLite::Exception in case of error
     */
    Database(const char* apFilename,
             const int   aFlags         = SQLITE_OPEN_READONLY,
             const int   aBusyTimeoutMs = 0,
             const char* apVfs          = NULL);

    /**
     * @brief Open the provided database UTF-8 filename.
     *
     * Uses sqlite3_open_v2() with readonly default flag, which is the opposite behavior
     * of the old sqlite3_open() function (READWRITE+CREATE).
     * This makes sense if you want to use it on a readonly filesystem
     * or to prevent creation of a void file when a required file is missing.
     *
     * Exception is thrown in case of error, then the Database object is NOT constructed.
     *
     * @param[in] aFilename         UTF-8 path/uri to the database file ("filename" sqlite3 parameter)
     * @param[in] aFlags            SQLITE_OPEN_READONLY/SQLITE_OPEN_READWRITE/SQLITE_OPEN_CREATE...
     * @param[in] aBusyTimeoutMs    Amount of milliseconds to wait before returning SQLITE_BUSY (see setBusyTimeout())
     * @param[in] aVfs              UTF-8 name of custom VFS to use, or empty string for sqlite3 default
     *
     * @throw SQLite::Exception in case of error
     */
    Database(const std::string& aFilename,
             const int          aFlags          = SQLITE_OPEN_READONLY,
             const int          aBusyTimeoutMs  = 0,
             const std::string& aVfs            = "");

    /**
     * @brief Close the SQLite database connection.
     *
     * All SQLite statements must have been finalized before,
     * so all Statement objects must have been unregistered.
     *
     * @warning assert in case of error
     */
    virtual ~Database() noexcept; // nothrow

    /**
     * @brief Set a busy handler that sleeps for a specified amount of time when a table is locked.
     *
     *  This is usefull in multithreaded program to handle case where a table is locked for writting by a thread.
     * Any other thread cannot access the table and will receive a SQLITE_BUSY error:
     * setting a timeout will wait and retry up to the time specified before returning this SQLITE_BUSY error.
     *  Reading the value of timeout for current connection can be done with SQL query "PRAGMA busy_timeout;".
     *  Default busy timeout is 0ms.
     *
     * @param[in] aBusyTimeoutMs    Amount of milliseconds to wait before returning SQLITE_BUSY
     *
     * @throw SQLite::Exception in case of error
     */
    void setBusyTimeout(const int aBusyTimeoutMs) noexcept; // nothrow

    /**
     * @brief Shortcut to execute one or multiple statements without results.
     *
     *  This is useful for any kind of statements other than the Data Query Language (DQL) "SELECT" :
     *  - Data Manipulation Language (DML) statements "INSERT", "UPDATE" and "DELETE"
     *  - Data Definition Language (DDL) statements "CREATE", "ALTER" and "DROP"
     *  - Data Control Language (DCL) statements "GRANT", "REVOKE", "COMMIT" and "ROLLBACK"
     *
     * @see Statement::exec() to handle precompiled statements (for better performances) without results
     * @see Statement::executeStep() to handle "SELECT" queries with results
     *
     * @param[in] apQueries  one or multiple UTF-8 encoded, semicolon-separate SQL statements
     *
     * @return number of rows modified by the *last* INSERT, UPDATE or DELETE statement (beware of multiple statements)
     * @warning undefined for CREATE or DROP table: returns the value of a previous INSERT, UPDATE or DELETE statement.
     *
     * @throw SQLite::Exception in case of error
     */
    int exec(const char* apQueries);

    /**
     * @brief Shortcut to execute one or multiple statements without results.
     *
     *  This is useful for any kind of statements other than the Data Query Language (DQL) "SELECT" :
     *  - Data Manipulation Language (DML) statements "INSERT", "UPDATE" and "DELETE"
     *  - Data Definition Language (DDL) statements "CREATE", "ALTER" and "DROP"
     *  - Data Control Language (DCL) statements "GRANT", "REVOKE", "COMMIT" and "ROLLBACK"
     *
     * @see Statement::exec() to handle precompiled statements (for better performances) without results
     * @see Statement::executeStep() to handle "SELECT" queries with results
     *
     * @param[in] aQueries  one or multiple UTF-8 encoded, semicolon-separate SQL statements
     *
     * @return number of rows modified by the *last* INSERT, UPDATE or DELETE statement (beware of multiple statements)
     * @warning undefined for CREATE or DROP table: returns the value of a previous INSERT, UPDATE or DELETE statement.
     *
     * @throw SQLite::Exception in case of error
     */
    inline int exec(const std::string& aQueries)
    {
        return exec(aQueries.c_str());
    }

    /**
     * @brief Shortcut to execute a one step query and fetch the first column of the result.
     *
     *  This is a shortcut to execute a simple statement with a single result.
     * This should be used only for non reusable queries (else you should use a Statement with bind()).
     * This should be used only for queries with expected results (else an exception is fired).
     *
     * @warning WARNING: Be very careful with this dangerous method: you have to
     *          make a COPY OF THE result, else it will be destroy before the next line
     *          (when the underlying temporary Statement and Column objects are destroyed)
     *
     * @see also Statement class for handling queries with multiple results
     *
     * @param[in] apQuery  an UTF-8 encoded SQL query
     *
     * @return a temporary Column object with the first column of result.
     *
     * @throw SQLite::Exception in case of error
     */
    Column execAndGet(const char* apQuery);

    /**
     * @brief Shortcut to execute a one step query and fetch the first column of the result.
     *
     *  This is a shortcut to execute a simple statement with a single result.
     * This should be used only for non reusable queries (else you should use a Statement with bind()).
     * This should be used only for queries with expected results (else an exception is fired).
     *
     * @warning WARNING: Be very careful with this dangerous method: you have to
     *          make a COPY OF THE result, else it will be destroy before the next line
     *          (when the underlying temporary Statement and Column objects are destroyed)
     *
     * @see also Statement class for handling queries with multiple results
     *
     * @param[in] aQuery  an UTF-8 encoded SQL query
     *
     * @return a temporary Column object with the first column of result.
     *
     * @throw SQLite::Exception in case of error
     */
    inline Column execAndGet(const std::string& aQuery)
    {
        return execAndGet(aQuery.c_str());
    }

    /**
     * @brief Shortcut to test if a table exists.
     *
     *  Table names are case sensitive.
     *
     * @param[in] apTableName an UTF-8 encoded case sensitive Table name
     *
     * @return true if the table exists.
     *
     * @throw SQLite::Exception in case of error
     */
    bool tableExists(const char* apTableName);

    /**
     * @brief Shortcut to test if a table exists.
     *
     *  Table names are case sensitive.
     *
     * @param[in] aTableName an UTF-8 encoded case sensitive Table name
     *
     * @return true if the table exists.
     *
     * @throw SQLite::Exception in case of error
     */
    inline bool tableExists(const std::string& aTableName)
    {
        return tableExists(aTableName.c_str());
    }

    /**
     * @brief Get the rowid of the most recent successful INSERT into the database from the current connection.
     *
     *  Each entry in an SQLite table always has a unique 64-bit signed integer key called the rowid.
     * If the table has a column of type INTEGER PRIMARY KEY, then it is an alias for the rowid.
     *
     * @return Rowid of the most recent successful INSERT into the database, or 0 if there was none.
     */
    inline sqlite3_int64 getLastInsertRowid() const noexcept // nothrow
    {
        return sqlite3_last_insert_rowid(mpSQLite);
    }

    /**
     * @brief Get total number of rows modified by all INSERT, UPDATE or DELETE statement since connection.
     *
     * @return Total number of rows modified since connection to the database. DROP tables does not count.
     */
    inline int getTotalChanges() const noexcept // nothrow
    {
        return sqlite3_total_changes(mpSQLite);
    }

    /// @brief Return the filename used to open the database.
    inline const std::string& getFilename() const noexcept // nothrow
    {
        return mFilename;
    }

    /// @brief Return the numeric result code for the most recent failed API call (if any).
    inline int getErrorCode() const noexcept // nothrow
    {
        return sqlite3_errcode(mpSQLite);
    }

    /// @brief Return the extended numeric result code for the most recent failed API call (if any).
    inline int getExtendedErrorCode() const noexcept // nothrow
    {
        return sqlite3_extended_errcode(mpSQLite);
    }

    /// @brief Return UTF-8 encoded English language explanation of the most recent failed API call (if any).
    inline const char* errmsg() const noexcept // nothrow
    {
        return sqlite3_errmsg(mpSQLite);
    }

    /**
     * @brief Return raw pointer to SQLite Database Connection Handle.
     *
     * This is often needed to mix this wrapper with other libraries or for advance usage not supported by SQLiteCpp.
    */
    inline sqlite3* getHandle() const noexcept // nothrow
    {
        return mpSQLite;
    }

    /**
     * @brief Create or redefine a SQL function or aggregate in the sqlite database. 
     *
     *  This is the equivalent of the sqlite3_create_function_v2 command.
     * @see http://www.sqlite.org/c3ref/create_function.html
     *
     * @note UTF-8 text encoding assumed.
     *
     * @param[in] apFuncName    Name of the SQL function to be created or redefined
     * @param[in] aNbArg        Number of arguments in the function
     * @param[in] abDeterministic Optimize for deterministic functions (most are). A random number generator is not.
     * @param[in] apApp         Arbitrary pointer of user data, accessible with sqlite3_user_data().
     * @param[in] apFunc        Pointer to a C-function to implement a scalar SQL function (apStep & apFinal NULL)
     * @param[in] apStep        Pointer to a C-function to implement an aggregate SQL function (apFunc NULL)
     * @param[in] apFinal       Pointer to a C-function to implement an aggregate SQL function (apFunc NULL)
     * @param[in] apDestroy     If not NULL, then it is the destructor for the application data pointer.
     *
     * @throw SQLite::Exception in case of error
     */
    void createFunction(const char* apFuncName,
                        int         aNbArg,
                        bool        abDeterministic,
                        void*       apApp,
                        void      (*apFunc)(sqlite3_context *, int, sqlite3_value **),
                        void      (*apStep)(sqlite3_context *, int, sqlite3_value **),
                        void      (*apFinal)(sqlite3_context *),  // NOLINT(readability/casting)
                        void      (*apDestroy)(void *));

    /**
     * @brief Create or redefine a SQL function or aggregate in the sqlite database. 
     *
     *  This is the equivalent of the sqlite3_create_function_v2 command.
     * @see http://www.sqlite.org/c3ref/create_function.html
     *
     * @note UTF-8 text encoding assumed.
     *
     * @param[in] aFuncName     Name of the SQL function to be created or redefined
     * @param[in] aNbArg        Number of arguments in the function
     * @param[in] abDeterministic Optimize for deterministic functions (most are). A random number generator is not.
     * @param[in] apApp         Arbitrary pointer of user data, accessible with sqlite3_user_data().
     * @param[in] apFunc        Pointer to a C-function to implement a scalar SQL function (apStep & apFinal NULL)
     * @param[in] apStep        Pointer to a C-function to implement an aggregate SQL function (apFunc NULL)
     * @param[in] apFinal       Pointer to a C-function to implement an aggregate SQL function (apFunc NULL)
     * @param[in] apDestroy     If not NULL, then it is the destructor for the application data pointer.
     *
     * @throw SQLite::Exception in case of error
     */
    inline void createFunction(const std::string&   aFuncName,
                               int                  aNbArg,
                               bool                 abDeterministic,
                               void*                apApp,
                               void               (*apFunc)(sqlite3_context *, int, sqlite3_value **),
                               void               (*apStep)(sqlite3_context *, int, sqlite3_value **),
                               void               (*apFinal)(sqlite3_context *), // NOLINT(readability/casting)
                               void               (*apDestroy)(void *))
    {
        return createFunction(aFuncName.c_str(), aNbArg, abDeterministic,
                              apApp, apFunc, apStep, apFinal, apDestroy);
    }


    /**
     * @brief Load a module into the current sqlite database instance. 
     *
     *  This is the equivalent of the sqlite3_load_extension call, but additionally enables
     *  module loading support prior to loading the requested module.
     *
     * @see http://www.sqlite.org/c3ref/load_extension.html
     *
     * @note UTF-8 text encoding assumed.
     *
     * @param[in] apExtensionName   Name of the shared library containing extension
     * @param[in] apEntryPointName  Name of the entry point (NULL to let sqlite work it out)
     *
     * @throw SQLite::Exception in case of error
     */
    void loadExtension(const char* apExtensionName,
         const char *apEntryPointName);

private:
    /// @{ Database must be non-copyable
    Database(const Database&);
    Database& operator=(const Database&);
    /// @}

    /**
     * @brief Check if aRet equal SQLITE_OK, else throw a SQLite::Exception with the SQLite error message
     */
    inline void check(const int aRet) const
    {
        if (SQLITE_OK != aRet)
        {
            throw SQLite::Exception(sqlite3_errstr(aRet));
        }
    }

private:
    sqlite3*    mpSQLite;   //!< Pointer to SQLite Database Connection Handle
    std::string mFilename;  //!< UTF-8 filename used to open the database
};


}  // namespace SQLite
