/**
 * @file    Statement_test.cpp
 * @ingroup tests
 * @brief   Test of a SQLiteCpp Statement.
 *
 * Copyright (c) 2012-2015 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>

#include <gtest/gtest.h>

#include <cstdio>


TEST(Statement, invalid) {
    // Create a new database
    SQLite::Database db(":memory:", SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
    EXPECT_EQ(SQLITE_OK, db.getErrorCode());
    EXPECT_EQ(SQLITE_OK, db.getExtendedErrorCode());

    // Compile a SQL query, but without any table in the database
    EXPECT_THROW(SQLite::Statement query(db, "SELECT * FROM test"), SQLite::Exception);
    EXPECT_EQ(SQLITE_ERROR, db.getErrorCode());
    EXPECT_EQ(SQLITE_ERROR, db.getExtendedErrorCode());

    EXPECT_EQ(0, db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"));
    EXPECT_EQ(SQLITE_OK, db.getErrorCode());
    EXPECT_EQ(SQLITE_OK, db.getExtendedErrorCode());

    // Compile a SQL query with no parameter
    SQLite::Statement   query(db, "SELECT * FROM test");
    EXPECT_STREQ("SELECT * FROM test", query.getQuery().c_str());
    EXPECT_EQ(2, query.getColumnCount ());
    EXPECT_FALSE(query.isOk());
    EXPECT_FALSE(query.isDone());
    EXPECT_THROW(query.isColumnNull(-1), SQLite::Exception);
    EXPECT_THROW(query.isColumnNull(0), SQLite::Exception);
    EXPECT_THROW(query.isColumnNull(1), SQLite::Exception);
    EXPECT_THROW(query.isColumnNull(2), SQLite::Exception);
    EXPECT_THROW(query.getColumn(-1), SQLite::Exception);
    EXPECT_THROW(query.getColumn(0), SQLite::Exception);
    EXPECT_THROW(query.getColumn(1), SQLite::Exception);
    EXPECT_THROW(query.getColumn(2), SQLite::Exception);

    query.reset();
    EXPECT_FALSE(query.isOk());
    EXPECT_FALSE(query.isDone());

    query.executeStep();
    EXPECT_FALSE(query.isOk());
    EXPECT_TRUE( query.isDone());
    query.reset();
    EXPECT_FALSE(query.isOk());
    EXPECT_FALSE(query.isDone());

    query.reset();
    EXPECT_THROW(query.bind(-1, 123), SQLite::Exception);
    EXPECT_THROW(query.bind(0, 123), SQLite::Exception);
    EXPECT_THROW(query.bind(1, 123), SQLite::Exception);
    EXPECT_THROW(query.bind(2, 123), SQLite::Exception);
    EXPECT_THROW(query.bind(0, "abc"), SQLite::Exception);
    EXPECT_THROW(query.bind(0), SQLite::Exception);
    EXPECT_EQ(SQLITE_RANGE, db.getErrorCode());
    EXPECT_EQ(SQLITE_RANGE, db.getExtendedErrorCode());

    query.exec(); // exec() instead of executeStep() as there is no result
    EXPECT_THROW(query.isColumnNull(0), SQLite::Exception);
    EXPECT_THROW(query.getColumn(0), SQLite::Exception);

    // Add a first row
    EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, \"first\")"));
    EXPECT_EQ(1, db.getLastInsertRowid());
    EXPECT_EQ(1, db.getTotalChanges());

    query.reset();
    EXPECT_FALSE(query.isOk());
    EXPECT_FALSE(query.isDone());

    EXPECT_THROW(query.exec(), SQLite::Exception); // exec() shall throw as it does not expect a result
}

// TODO: test every kind of binding + clearBindings()

TEST(Statement, getColumnByName) {
    // Create a new database
    SQLite::Database db(":memory:", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    EXPECT_EQ(SQLITE_OK, db.getErrorCode());
    EXPECT_EQ(SQLITE_OK, db.getExtendedErrorCode());

    // Create a new table
    EXPECT_EQ(0, db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, msg TEXT, int INTEGER, double REAL)"));
    EXPECT_EQ(SQLITE_OK, db.getErrorCode());
    EXPECT_EQ(SQLITE_OK, db.getExtendedErrorCode());

    // Create a first row
    EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, \"first\", 123, 0.123)"));
    EXPECT_EQ(1, db.getLastInsertRowid());
    EXPECT_EQ(1, db.getTotalChanges());

    // Compile a SQL query
    SQLite::Statement   query(db, "SELECT * FROM test");
    EXPECT_STREQ("SELECT * FROM test", query.getQuery().c_str());
    EXPECT_EQ(4, query.getColumnCount());
    query.executeStep();
    EXPECT_TRUE (query.isOk());
    EXPECT_FALSE(query.isDone());

    // Look for unexisting columns
    EXPECT_THROW(query.getColumn("unknown"), SQLite::Exception);
    EXPECT_THROW(query.getColumn(""), SQLite::Exception);

	const std::string   msg     = query.getColumn("msg");
	const int           integer = query.getColumn("int");
	const double        real    = query.getColumn("double");
    EXPECT_EQ("first",  msg);
    EXPECT_EQ(123,      integer);
    EXPECT_EQ(0.123,    real);
}

TEST(Statement, getName) {
    // Create a new database
    SQLite::Database db(":memory:", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    EXPECT_EQ(0, db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, msg TEXT)"));

    // Compile a SQL query, using the "id" column name as-is, but aliasing the "msg" column with new name "value"
    SQLite::Statement   query(db, "SELECT id, msg as value FROM test");
    query.executeStep();

    const std::string name0 = query.getColumnName(0);
    const std::string name1 = query.getColumnName(1);
    EXPECT_EQ("id", name0);
    EXPECT_EQ("value", name1);

#ifdef SQLITE_ENABLE_COLUMN_METADATA
    // Show how to get origin names of the table columns from which theses result columns come from.
    // Requires the SQLITE_ENABLE_COLUMN_METADATA preprocessor macro to be
    // also defined at compile times of the SQLite library itself.
    const std::string oname0 = query.getColumnOriginName(0);
    const std::string oname1 = query.getColumnOriginName(1);
    EXPECT_EQ("id", oname0);
    EXPECT_EQ("msg", oname1);
#endif
}
