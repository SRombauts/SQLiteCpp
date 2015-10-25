/**
 * @file    Backup.h
 * @ingroup SQLiteCpp
 * @brief   Backup is used to backup a database file in a safe and online way.
 *
 * Copyright (c) 2015 Shibao HONG (shibaohong@outlook.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include <sqlite3.h>

#include <SQLiteCpp/Database.h>

#include <string>

namespace SQLite
{

/**
 * @brief RAII encapsulation of a SQLite Database Backup process.
 *
 * A Backup object is used to backup a source database file to a destination database file
 * in a safe and online way.
 *
 * Resource Acquisition Is Initialization (RAII) means that the Backup Resource
 * is allocated in the constructor and released in the destructor, so that there is
 * no need to worry about memory management or the validity of the underlying SQLite Backup.
 *
 * Thread-safety: a Backup object shall not be shared by multiple threads, because :
 * 1) in the SQLite "Thread Safe" mode, "SQLite can be safely used by multiple threads
 *    provided that no single database connection is used simultaneously in two or more threads."
 * 2) the SQLite "Serialized" mode is not supported by SQLiteC++,
 *    because of the way it shares the underling SQLite precompiled statement
 *    in a custom shared pointer (See the inner class "Statement::Ptr").
 */
class Backup
{
public:
    Backup(Database&   aDestDatabase,
           const char* apDestDatabaseName,
           Database&   aSrcDatabase,
           const char* apSrcDatabaseName);

    Backup(Database&          aDestDatabase,
           const std::string& aDestDatabaseName,
           Database&          aSrcDatabase,
           const std::string& aSrcDatabaseName);

    Backup(Database& aDestDatabase,
           Database& aSrcDatabase);

    virtual ~Backup() noexcept;

    int executeStep(const int aNumPage = -1);

    int remainingPageCount();

    int totalPageCount();

    /**
     * @brief Return raw pointer to SQLite Database Backup Handle.
     *
     * This is often needed to mix this wrapper with other libraries or for advance usage not supported by SQLiteCpp.
    */
    inline sqlite3_backup* getHandle() const noexcept // nothrow
    {
        return mpSQLiteBackup;
    }

private:
    /// @{ Backup must be non-copyable
    Backup(const Backup&);
    Backup& operator=(const Backup&);
    /// @}

private:
    sqlite3_backup* mpSQLiteBackup;
};

}  // namespace SQLite
