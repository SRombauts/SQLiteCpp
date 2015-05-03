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
#include <stdint.h>


TEST(Column, basis) {
    // Create a new database
    SQLite::Database db(":memory:", SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
    EXPECT_EQ(SQLITE_OK, db.getErrorCode());
    EXPECT_EQ(SQLITE_OK, db.getExtendedErrorCode());

    // Create a new table
    EXPECT_EQ(0, db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, msg TEXT, int INTEGER, double REAL, binary BLOB, empty TEXT)"));
    EXPECT_TRUE(db.tableExists("test"));
    EXPECT_TRUE(db.tableExists(std::string("test")));
    EXPECT_EQ(0, db.getLastInsertRowid());

    // Create a first row (autoid: 1) with all kind of data and a null value
    SQLite::Statement   insert(db, "INSERT INTO test VALUES (NULL, \"first\", 123, 0.123, ?, NULL)");
    // Bind the blob value to the first parameter of the SQL query
    char  buffer[] = "blob";
    void* blob = &buffer;
    int size = sizeof(buffer);
    insert.bind(1, blob, size);
    // Execute the one-step query to insert the row
    EXPECT_EQ(1, insert.exec());

    EXPECT_EQ(1, db.getLastInsertRowid());
    EXPECT_EQ(1, db.getTotalChanges());

    // Compile a SQL query
    SQLite::Statement   query(db, "SELECT * FROM test");
    EXPECT_STREQ("SELECT * FROM test", query.getQuery().c_str());
    EXPECT_EQ(6, query.getColumnCount ());
    query.executeStep();
    EXPECT_TRUE (query.isOk());
    EXPECT_FALSE(query.isDone());

    // validates every variant of cast operators, and conversions of types
    {
        sqlite3_int64       id      = query.getColumn(0); // operator sqlite3_int64()
        int64_t             id2     = query.getColumn(0); // operator sqlite3_int64() (or long() with GCC 64bits)
        long                id3     = query.getColumn(0); // operator sqlite3_int64() (or long() with GCC 64bits)
        const char*         ptxt    = query.getColumn(1); // operator const char*()
        const std::string   msg     = query.getColumn(1); // operator std::string() (or const char* with MSVC)
        const int           integer = query.getColumn(2); // operator int()
        const double        real    = query.getColumn(3); // operator double()
        const void*         pblob   = query.getColumn(4); // operator void*()
        const void*         pempty  = query.getColumn(5); // operator void*()
        EXPECT_EQ(1,            id);
        EXPECT_EQ(1,            id2);
        EXPECT_EQ(1,            id3);
        EXPECT_STREQ("first",   ptxt);
        EXPECT_EQ("first",      msg);
        EXPECT_EQ(123,          integer);
        EXPECT_EQ(0.123,        real);
        EXPECT_EQ(0,            memcmp("blob", pblob, size));
        EXPECT_EQ(NULL,         pempty);
    }

    // validates every variant of explicit getters
    {
        int64_t             id      = query.getColumn(0).getInt64();
        const char*         ptxt    = query.getColumn(1).getText();
        const std::string   msg     = query.getColumn(1).getText();
        const int           integer = query.getColumn(2).getInt();
        const double        real    = query.getColumn(3).getDouble();
        const void*         pblob   = query.getColumn(1).getBlob();
        EXPECT_EQ(1,            id);
        EXPECT_STREQ("first",   ptxt);
        EXPECT_EQ("first",      msg);
        EXPECT_EQ(123,          integer);
        EXPECT_EQ(0.123,        real);
        EXPECT_EQ(0,            memcmp("first", pblob, 5));
    }

    // Validate getBytes(), getType(), isInteger(), isNull()...
    EXPECT_EQ(SQLITE_INTEGER,   query.getColumn(0).getType());
    EXPECT_EQ(true,             query.getColumn(0).isInteger());
    EXPECT_EQ(false,            query.getColumn(0).isFloat());
    EXPECT_EQ(false,            query.getColumn(0).isText());
    EXPECT_EQ(false,            query.getColumn(0).isBlob());
    EXPECT_EQ(false,            query.getColumn(0).isNull());
    EXPECT_STREQ("1",           query.getColumn(0).getText());  // convert to string
    EXPECT_EQ(1,                query.getColumn(0).getBytes()); // size of the string "1" without the null terminator
    EXPECT_EQ(SQLITE_TEXT,      query.getColumn(1).getType());
    EXPECT_EQ(false,            query.getColumn(1).isInteger());
    EXPECT_EQ(false,            query.getColumn(1).isFloat());
    EXPECT_EQ(true,             query.getColumn(1).isText());
    EXPECT_EQ(false,            query.getColumn(1).isBlob());
    EXPECT_EQ(false,            query.getColumn(1).isNull());
    EXPECT_STREQ("first",       query.getColumn(1).getText());  // convert to string
    EXPECT_EQ(5,                query.getColumn(1).getBytes()); // size of the string "first"
    EXPECT_EQ(SQLITE_INTEGER,   query.getColumn(2).getType());
    EXPECT_EQ(true,             query.getColumn(2).isInteger());
    EXPECT_EQ(false,            query.getColumn(2).isFloat());
    EXPECT_EQ(false,            query.getColumn(2).isText());
    EXPECT_EQ(false,            query.getColumn(2).isBlob());
    EXPECT_EQ(false,            query.getColumn(2).isNull());
    EXPECT_STREQ("123",         query.getColumn(2).getText());  // convert to string
    EXPECT_EQ(3,                query.getColumn(2).getBytes()); // size of the string "123"
    EXPECT_EQ(SQLITE_FLOAT,     query.getColumn(3).getType());
    EXPECT_EQ(false,            query.getColumn(3).isInteger());
    EXPECT_EQ(true,             query.getColumn(3).isFloat());
    EXPECT_EQ(false,            query.getColumn(3).isText());
    EXPECT_EQ(false,            query.getColumn(3).isBlob());
    EXPECT_EQ(false,            query.getColumn(3).isNull());
    EXPECT_STREQ("0.123",       query.getColumn(3).getText());  // convert to string
    EXPECT_EQ(5,                query.getColumn(3).getBytes()); // size of the string "0.123"
    EXPECT_EQ(SQLITE_BLOB,      query.getColumn(4).getType());
    EXPECT_EQ(false,            query.getColumn(4).isInteger());
    EXPECT_EQ(false,            query.getColumn(4).isFloat());
    EXPECT_EQ(false,            query.getColumn(4).isText());
    EXPECT_EQ(true,             query.getColumn(4).isBlob());
    EXPECT_EQ(false,            query.getColumn(4).isNull());
    EXPECT_STREQ("blob",        query.getColumn(4).getText());  // convert to string
    EXPECT_EQ(5,                query.getColumn(4).getBytes()); // size of the string "blob" WITH the null terminator (blob)
    EXPECT_EQ(SQLITE_NULL,      query.getColumn(5).getType());
    EXPECT_EQ(false,            query.getColumn(5).isInteger());
    EXPECT_EQ(false,            query.getColumn(5).isFloat());
    EXPECT_EQ(false,            query.getColumn(5).isText());
    EXPECT_EQ(false,            query.getColumn(5).isBlob());
    EXPECT_EQ(true,             query.getColumn(5).isNull());
    EXPECT_STREQ("",            query.getColumn(5).getText());  // convert to string
    EXPECT_EQ(0,                query.getColumn(5).getBytes()); // size of the string "" without the null terminator
}

TEST(Column, getName) {
    // Create a new database
    SQLite::Database db(":memory:", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    EXPECT_EQ(0, db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, msg TEXT)"));
    EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, \"first\")"));

    // Compile a SQL query, using the "id" column name as-is, but aliasing the "msg" column with new name "value"
    SQLite::Statement   query(db, "SELECT id, msg as value FROM test");
    query.executeStep();

    // Show how to get the aliased names of the result columns.
    const std::string name0 = query.getColumn(0).getName();
    const std::string name1 = query.getColumn(1).getName();
    EXPECT_EQ("id",     name0);
    EXPECT_EQ("value",  name1);

#ifdef SQLITE_ENABLE_COLUMN_METADATA
    // Show how to get origin names of the table columns from which theses result columns come from.
    // Requires the SQLITE_ENABLE_COLUMN_METADATA preprocessor macro to be
    // also defined at compile times of the SQLite library itself.
    const std::string oname0 = query.getColumn(0).getOriginName();
    const std::string oname1 = query.getColumn(1).getOriginName();
    EXPECT_EQ("id",     oname0);
    EXPECT_EQ("msg",    oname1);
#endif
}