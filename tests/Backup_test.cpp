/**
 * @file    Backup_test.cpp
 * @ingroup tests
 * @brief   Test of a SQLite Backup.
 *
 * Copyright (c) 2015-2015 Shibao HONG (shibaohong@outlook.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <SQLiteCpp/Backup.h>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Exception.h>

#include <gtest/gtest.h>

#include <cstdio>

TEST(Backup, executeStep) {
    remove("backup_test.db3");
    remove("backup_test.db3.backup");
    SQLite::Database srcDB("backup_test.db3", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    srcDB.exec("CREATE TABLE backup_test (id INTEGER PRIMARY KEY, value TEXT)");
    ASSERT_EQ(1, srcDB.exec("INSERT INTO backup_test VALUES (1, \"first\")"));
    ASSERT_EQ(1, srcDB.exec("INSERT INTO backup_test VALUES (2, \"second\")"));

    SQLite::Database destDB("backup_test.db3.backup", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    SQLite::Backup backup(destDB, srcDB);
    const int res = backup.executeStep();
    ASSERT_EQ(res, SQLITE_DONE);

    SQLite::Statement query(destDB, "SELECT * FROM backup_test ORDER BY id ASC");
    ASSERT_TRUE(query.executeStep());
    EXPECT_EQ(1, query.getColumn(0).getInt());
    EXPECT_STREQ("first", query.getColumn(1));
    ASSERT_TRUE(query.executeStep());
    EXPECT_EQ(2, query.getColumn(0).getInt());
    EXPECT_STREQ("second", query.getColumn(1));
    remove("backup_test.db3");
    remove("backup_test.db3.backup");
}

TEST(Backup, executeStepException) {
    remove("backup_test.db3");
    remove("backup_test.db3.backup");
    SQLite::Database srcDB("backup_test.db3", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    srcDB.exec("CREATE TABLE backup_test (id INTEGER PRIMARY KEY, value TEXT)");
    ASSERT_EQ(1, srcDB.exec("INSERT INTO backup_test VALUES (1, \"first\")"));
    ASSERT_EQ(1, srcDB.exec("INSERT INTO backup_test VALUES (2, \"second\")"));
    {
        SQLite::Database destDB("backup_test.db3.backup", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
        (void)destDB;
    }
    {
        SQLite::Database destDB("backup_test.db3.backup", SQLITE_OPEN_READONLY);
        SQLite::Backup backup(destDB, srcDB);
        EXPECT_THROW(backup.executeStep(), SQLite::Exception);
    }
    remove("backup_test.db3");
    remove("backup_test.db3.backup");
}
