/**
 * @file    Statement_test.cpp
 * @ingroup tests
 * @brief   Test of a SQLiteCpp Statement.
 *
 * Copyright (c) 2014 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>

#include <gtest/gtest.h>

#include <cstdio>


TEST(Statement, exec) {
    remove("test.db3");
    {
        // Create a new database
        SQLite::Database db("test.db3", SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);

        // Compile a SQL query, but without any table in the database
        EXPECT_THROW(SQLite::Statement query(db, "SELECT * FROM test"), SQLite::Exception);

        EXPECT_EQ(0, db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"));
        
        // Compile a SQL query with no parameter
        SQLite::Statement   query(db, "SELECT * FROM test");
        EXPECT_STREQ("SELECT * FROM test", query.getQuery().c_str());
        EXPECT_EQ(2, query.getColumnCount ());


    } // Close DB test.db3
    remove("test.db3");
}
