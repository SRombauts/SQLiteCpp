/**
 * @file    Backup.h
 * @ingroup SQLiteCpp
 * @brief   Management of a SQLite Database Backup.
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
