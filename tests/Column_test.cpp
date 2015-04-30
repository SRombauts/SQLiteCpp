/**
 * @file    Column_test.cpp
 * @ingroup tests
 * @brief   Test of a SQLiteCpp Column.
 *
 * Copyright (c) 2012-2015 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Column.h>

#include <gtest/gtest.h>

#include <cstdio>


TEST(Column, basics) {
    remove("test.db3");
    {
        // Create a new database
        SQLite::Database db("test.db3", SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
        EXPECT_EQ(SQLITE_OK, db.getErrorCode());
        EXPECT_EQ(SQLITE_OK, db.getExtendedErrorCode());

        // Create a new table
        EXPECT_EQ(0, db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, msg TEXT, int INTEGER, double REAL)"));
        EXPECT_TRUE(db.tableExists("test"));
        EXPECT_TRUE(db.tableExists(std::string("test")));
        EXPECT_EQ(0, db.getLastInsertRowid());

        // Create a first row
        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, \"first\", 123, 0.123)"));
        EXPECT_EQ(1, db.getLastInsertRowid());
        EXPECT_EQ(1, db.getTotalChanges());

        // Compile a SQL query
        SQLite::Statement   query(db, "SELECT * FROM test");
        EXPECT_STREQ("SELECT * FROM test", query.getQuery().c_str());
        EXPECT_EQ(4, query.getColumnCount ());
        query.executeStep();
        EXPECT_TRUE (query.isOk());
        EXPECT_FALSE(query.isDone());

        // TODO 
		const std::string   msg     = query.getColumn("msg");
		const int           integer = query.getColumn("int");
		const double        real    = query.getColumn("double");
        EXPECT_EQ("first",  msg);
        EXPECT_EQ(123,      integer);
        EXPECT_EQ(0.123,    real);

    } // Close DB test.db3
    remove("test.db3");
}
