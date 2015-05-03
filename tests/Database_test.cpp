/**
 * @file    Database_test.cpp
 * @ingroup tests
 * @brief   Test of a SQLiteCpp Database.
 *
 * Copyright (c) 2012-2015 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <SQLiteCpp/Database.h>

#include <gtest/gtest.h>

#include <cstdio>

#ifdef SQLITECPP_ENABLE_ASSERT_HANDLER
namespace SQLite
{
/// definition of the assertion handler enabled when SQLITECPP_ENABLE_ASSERT_HANDLER is defined in the project (CMakeList.txt)
void assertion_failed(const char* apFile, const long apLine, const char* apFunc, const char* apExpr, const char* apMsg)
{
    // TODO: unit test that this assertion callback get called (already tested manually)
    std::cout << "assertion_failed(" << apFile << ", " << apLine << ", " << apFunc << ", " << apExpr << ", " << apMsg << ")\n";
}
}
#endif


TEST(Database, ctorExecCreateDropExist) {
    remove("test.db3");
    {
        // Try to open an unexisting database
        std::string filename = "test.db3";
        EXPECT_THROW(SQLite::Database not_found(filename), SQLite::Exception);

        // Create a new database
        SQLite::Database db("test.db3", SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
        EXPECT_STREQ("test.db3", db.getFilename().c_str());
        EXPECT_FALSE(db.tableExists("test"));
        EXPECT_FALSE(db.tableExists(std::string("test")));
        EXPECT_EQ(0, db.getLastInsertRowid());

        EXPECT_EQ(0, db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"));
        EXPECT_TRUE(db.tableExists("test"));
        EXPECT_TRUE(db.tableExists(std::string("test")));
        EXPECT_EQ(0, db.getLastInsertRowid());
        
        EXPECT_EQ(0, db.exec("DROP TABLE IF EXISTS test"));
        EXPECT_FALSE(db.tableExists("test"));
        EXPECT_FALSE(db.tableExists(std::string("test")));
        EXPECT_EQ(0, db.getLastInsertRowid());
    } // Close DB test.db3
    remove("test.db3");
}

TEST(Database, createCloseReopen) {
    remove("test.db3");
    {
        // Try to open the unexisting database
        EXPECT_THROW(SQLite::Database not_found("test.db3"), SQLite::Exception);

        // Create a new database
        SQLite::Database db("test.db3", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
        EXPECT_FALSE(db.tableExists("test"));
        db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)");
        EXPECT_TRUE(db.tableExists("test"));
    } // Close DB test.db3
    {
        // Reopen the database file
        SQLite::Database db("test.db3", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
        EXPECT_TRUE(db.tableExists("test"));
    } // Close DB test.db3
    remove("test.db3");
}

TEST(Database, inMemory) {
    {
        // Create a new database
        SQLite::Database db(":memory:", SQLITE_OPEN_READWRITE);
        EXPECT_FALSE(db.tableExists("test"));
        db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)");
        EXPECT_TRUE(db.tableExists("test"));
        // Create a new database: not shared with the above db
        SQLite::Database db2(":memory:");
        EXPECT_FALSE(db2.tableExists("test"));
    } // Close an destroy DBs
    {
        // Create a new database: no more "test" table
        SQLite::Database db(":memory:");
        EXPECT_FALSE(db.tableExists("test"));
    } // Close an destroy DB
}

#if SQLITE_VERSION_NUMBER >= 3007015 // SQLite v3.7.15 is first version with PRAGMA busy_timeout
TEST(Database, busyTimeout) {
    {
        // Create a new database with default timeout of 0ms
        SQLite::Database db(":memory:");
        // Busy timeout default to 0ms: any contention between threads or process leads to SQLITE_BUSY error
        EXPECT_EQ(0, db.execAndGet("PRAGMA busy_timeout").getInt());

        // Set a non null busy timeout: any contention between threads will leads to as much retry as possible during the time
        db.setBusyTimeout(5000);
        EXPECT_EQ(5000, db.execAndGet("PRAGMA busy_timeout").getInt());

        // Reset timeout to 0
        db.setBusyTimeout(0);
        EXPECT_EQ(0, db.execAndGet("PRAGMA busy_timeout").getInt());
    }
    {
        // Create a new database with a non null busy timeout
        SQLite::Database db(":memory:", SQLITE_OPEN_READWRITE, 5000);
        EXPECT_EQ(5000, db.execAndGet("PRAGMA busy_timeout").getInt());

        // Reset timeout to null
        db.setBusyTimeout(0);
        EXPECT_EQ(0, db.execAndGet("PRAGMA busy_timeout").getInt());
    }
}
#endif // SQLITE_VERSION_NUMBER >= 3007015

TEST(Database, exec) {
    // Create a new database
    SQLite::Database db(":memory:", SQLITE_OPEN_READWRITE);

    // Create a new table with an explicit "id" column aliasing the underlying rowid
    // NOTE: here exec() returns 0 only because it is the first statements since database connexion,
    //       but its return is an undefined value for "CREATE TABLE" statements.
    db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)");
    EXPECT_EQ(0, db.getLastInsertRowid());
    EXPECT_EQ(0, db.getTotalChanges());

    // first row : insert the "first" text value into new row of id 1
    EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, \"first\")"));
    EXPECT_EQ(1, db.getLastInsertRowid());
    EXPECT_EQ(1, db.getTotalChanges());

    // second row : insert the "second" text value into new row of id 2
    EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, \"second\")"));
    EXPECT_EQ(2, db.getLastInsertRowid());
    EXPECT_EQ(2, db.getTotalChanges());

    // third row : insert the "third" text value into new row of id 3
    EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, \"third\")"));
    EXPECT_EQ(3, db.getLastInsertRowid());
    EXPECT_EQ(3, db.getTotalChanges());

    // update the second row : update text value to "second_updated"
    EXPECT_EQ(1, db.exec("UPDATE test SET value=\"second-updated\" WHERE id='2'"));
    EXPECT_EQ(3, db.getLastInsertRowid()); // last inserted row ID is still 3
    EXPECT_EQ(4, db.getTotalChanges());

    // delete the third row
    EXPECT_EQ(1, db.exec("DELETE FROM test WHERE id='3'"));
    EXPECT_EQ(3, db.getLastInsertRowid());
    EXPECT_EQ(5, db.getTotalChanges());

    // drop the whole table, ie the two remaining columns
    // NOTE: here exec() returns 1, like the last time, as it is an undefined value for "DROP TABLE" statements
    db.exec("DROP TABLE IF EXISTS test");
    EXPECT_FALSE(db.tableExists("test"));
    EXPECT_EQ(5, db.getTotalChanges());

    // Re-Create the same table
    // NOTE: here exec() returns 1, like the last time, as it is an undefined value for "CREATE TABLE" statements
    db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)");
    EXPECT_EQ(5, db.getTotalChanges());

    // insert two rows with two *different* statements => returns only 1, ie. for the second INSERT statement
    EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, \"first\");INSERT INTO test VALUES (NULL, \"second\");"));
    EXPECT_EQ(2, db.getLastInsertRowid());
    EXPECT_EQ(7, db.getTotalChanges());

#if (SQLITE_VERSION_NUMBER >= 3007011)
    // insert two rows with only one statement (starting with SQLite 3.7.11) => returns 2
    EXPECT_EQ(2, db.exec("INSERT INTO test VALUES (NULL, \"third\"), (NULL, \"fourth\");"));
    EXPECT_EQ(4, db.getLastInsertRowid());
    EXPECT_EQ(9, db.getTotalChanges());
#endif
}

TEST(Database, execAndGet) {
    // Create a new database
    SQLite::Database db(":memory:", SQLITE_OPEN_READWRITE);

    // Create a new table with an explicit "id" column aliasing the underlying rowid
    db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT, weight INTEGER)");

    // insert a few rows
    EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, \"first\",  3)"));
    EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, \"second\", 5)"));
    EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, \"third\",  7)"));

    // Get a single value result with an easy to use shortcut
    EXPECT_STREQ("second",  db.execAndGet("SELECT value FROM test WHERE id=2"));
    EXPECT_STREQ("third",   db.execAndGet("SELECT value FROM test WHERE weight=7"));
    EXPECT_EQ(3,            db.execAndGet("SELECT weight FROM test WHERE value=\"first\"").getInt());
}

TEST(Database, execException) {
    // Create a new database
    SQLite::Database db(":memory:", SQLITE_OPEN_READWRITE);
    EXPECT_EQ(SQLITE_OK, db.getErrorCode());
    EXPECT_EQ(SQLITE_OK, db.getExtendedErrorCode());

    // exception with SQL error: "no such table"
    EXPECT_THROW(db.exec("INSERT INTO test VALUES (NULL, \"first\",  3)"), SQLite::Exception);
    EXPECT_EQ(SQLITE_ERROR, db.getErrorCode());
    EXPECT_EQ(SQLITE_ERROR, db.getExtendedErrorCode());

    // Create a new table
    db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT, weight INTEGER)");
    EXPECT_EQ(SQLITE_OK, db.getErrorCode());
    EXPECT_EQ(SQLITE_OK, db.getExtendedErrorCode());

    // exception with SQL error: "table test has 3 columns but 2 values were supplied"
    EXPECT_THROW(db.exec("INSERT INTO test VALUES (NULL,  3)"), SQLite::Exception);
    EXPECT_EQ(SQLITE_ERROR, db.getErrorCode());
    EXPECT_EQ(SQLITE_ERROR, db.getExtendedErrorCode());

    // exception with SQL error: "No row to get a column from"
    EXPECT_THROW(db.execAndGet("SELECT weight FROM test WHERE value=\"first\""), SQLite::Exception);

    EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, \"first\",  3)"));
    // exception with SQL error: "No row to get a column from"
    EXPECT_THROW(db.execAndGet("SELECT weight FROM test WHERE value=\"second\""), SQLite::Exception);

    // Add a row with more values than columns in the table: "table test has 3 columns but 4 values were supplied"
    EXPECT_THROW(db.exec("INSERT INTO test VALUES (NULL, \"first\", 123, 0.123)"), SQLite::Exception);
    EXPECT_EQ(SQLITE_ERROR, db.getErrorCode());
    EXPECT_EQ(SQLITE_ERROR, db.getExtendedErrorCode());
}

// TODO: test Database::createFunction()
