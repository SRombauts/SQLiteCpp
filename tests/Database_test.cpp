/**
 * @file    Database_test.cpp
 * @ingroup tests
 * @brief   Test of a SQLiteCpp Database.
 *
 * Copyright (c) 2014 Sebastien Rombauts (sebastien.rombauts@gmail.com)
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
    // TODO test that this assertion callback get called
    std::cout << "assertion_failed(" << apFile << ", " << apLine << ", " << apFunc << ", " << apExpr << ", " << apMsg << ")\n";
}
}
#endif


// Constructor
TEST(Database, ctorExecCreateDropExist) {
    remove("test.db3");
    {
        // Try to open an unexisting database
        EXPECT_THROW(SQLite::Database not_found("test.db3"), SQLite::Exception);

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


// Constructor
TEST(Database, ctorExecAndGet) {
    remove("test.db3");
    {
        // Create a new database
        SQLite::Database db("test.db3", SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);

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

        // insert two rows with only one statement => returns 2
        EXPECT_EQ(2, db.exec("INSERT INTO test VALUES (NULL, \"third\"), (NULL, \"fourth\");"));
        EXPECT_EQ(4, db.getLastInsertRowid());
        EXPECT_EQ(9, db.getTotalChanges());

    } // Close DB test.db3
    remove("test.db3");
}
