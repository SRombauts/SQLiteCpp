/**
 * @file    Backup.cpp
 * @ingroup SQLiteCpp
 * @brief   Backup is used to backup a database file in a safe and online way.
 *
 * Copyright (c) 2015 Shibao HONG (shibaohong@outlook.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <SQLiteCpp/Backup.h>

#include <SQLiteCpp/Exception.h>

#include <string>

namespace SQLite
{

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

Backup::~Backup() noexcept
{
    if (NULL != mpSQLiteBackup)
    {
        sqlite3_backup_finish(mpSQLiteBackup);
    }
}

int Backup::executeStep(const int aNumPage /* = -1 */)
{
    const int res = sqlite3_backup_step(mpSQLiteBackup, aNumPage);
    if (SQLITE_OK != res && SQLITE_DONE != res &&
            SQLITE_BUSY != res && SQLITE_LOCKED != res)
    {
        std::string strerr("Backup executeStep error with message ");
        strerr += sqlite3_errstr(res);
        throw SQLite::Exception(strerr);
    }
    return res;
}

int Backup::remainingPageCount()
{
    return sqlite3_backup_remaining(mpSQLiteBackup);
}

int Backup::totalPageCount()
{
    return sqlite3_backup_pagecount(mpSQLiteBackup);
}

}  // namespace SQLite
