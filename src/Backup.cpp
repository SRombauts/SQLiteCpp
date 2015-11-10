/**
 * @file    Backup.cpp
 * @ingroup SQLiteCpp
 * @brief   Backup is used to backup a database file in a safe and online way.
 *
 * Copyright (c) 2015-2015 Shibao HONG (shibaohong@outlook.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <SQLiteCpp/Backup.h>

#include <SQLiteCpp/Exception.h>

#include <string>

namespace SQLite
{

// Initialize resource for SQLite database backup
Backup::Backup(Database&  aDestDatabase,
               const char *apDestDatabaseName,
               Database&  aSrcDatabase,
               const char *apSrcDatabaseName) :
    mpSQLiteBackup(NULL)
{
    mpSQLiteBackup = sqlite3_backup_init(aDestDatabase.getHandle(),
                                         apDestDatabaseName,
                                         aSrcDatabase.getHandle(),
                                         apSrcDatabaseName);
    if (NULL == mpSQLiteBackup)
    {
        std::string strerr = sqlite3_errmsg(aDestDatabase.getHandle());
        throw SQLite::Exception(strerr);
    }
}

// Initialize resource for SQLite database backup
Backup::Backup(Database &aDestDatabase,
               const std::string &aDestDatabaseName,
               Database &aSrcDatabase,
               const std::string &aSrcDatabaseName) :
    mpSQLiteBackup(NULL)
{
    mpSQLiteBackup = sqlite3_backup_init(aDestDatabase.getHandle(),
                                         aDestDatabaseName.c_str(),
                                         aSrcDatabase.getHandle(),
                                         aSrcDatabaseName.c_str());
    if (NULL == mpSQLiteBackup)
    {
        std::string strerr = sqlite3_errmsg(aDestDatabase.getHandle());
        throw SQLite::Exception(strerr);
    }
}

// Initialize resource for SQLite database backup
Backup::Backup(Database &aDestDatabase, Database &aSrcDatabase) :
    mpSQLiteBackup(NULL)
{
    mpSQLiteBackup = sqlite3_backup_init(aDestDatabase.getHandle(),
                                         "main",
                                         aSrcDatabase.getHandle(),
                                         "main");
    if (NULL == mpSQLiteBackup)
    {
        std::string strerr = sqlite3_errmsg(aDestDatabase.getHandle());
        throw SQLite::Exception(strerr);
    }
}

// Release resource for SQLite database backup
Backup::~Backup() noexcept
{
    if (NULL != mpSQLiteBackup)
    {
        sqlite3_backup_finish(mpSQLiteBackup);
    }
}

// Execute backup step with a given number of source pages to be copied
int Backup::executeStep(const int aNumPage /* = -1 */)
{
    const int res = sqlite3_backup_step(mpSQLiteBackup, aNumPage);
    if (SQLITE_OK != res && SQLITE_DONE != res &&
            SQLITE_BUSY != res && SQLITE_LOCKED != res)
    {
        std::string strerr = sqlite3_errstr(res);
        throw SQLite::Exception(strerr);
    }
    return res;
}

// Get the number of remaining source pages to be copied in this backup process
int Backup::remainingPageCount()
{
    return sqlite3_backup_remaining(mpSQLiteBackup);
}

// Get the number of total source pages to be copied in this backup process
int Backup::totalPageCount()
{
    return sqlite3_backup_pagecount(mpSQLiteBackup);
}

}  // namespace SQLite
