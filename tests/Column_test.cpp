/**
 * @file    Column_test.cpp
 * @ingroup tests
 * @brief   Test of a SQLiteCpp Column.
 *
 * Copyright (c) 2012-2016 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Column.h>

#include <sqlite3.h> // for sqlite3_int64

#include <gtest/gtest.h>

#include <cstdio>
#include <stdint.h>


TEST(Column, basis) {
    // Create a new database
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
    EXPECT_EQ(SQLite::OK, db.getErrorCode());
    EXPECT_EQ(SQLite::OK, db.getExtendedErrorCode());

    // Create a new table
    EXPECT_EQ(0, db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, msg TEXT, int INTEGER, double REAL, binary BLOB, empty TEXT)"));
    EXPECT_TRUE(db.tableExists("test"));
    EXPECT_TRUE(db.tableExists(std::string("test")));
    EXPECT_EQ(0, db.getLastInsertRowid());

    // Create a first row (autoid: 1) with all kind of data and a null value
    SQLite::Statement   insert(db, "INSERT INTO test VALUES (NULL, \"first\", -123, 0.123, ?, NULL)");
    // Bind the blob value to the first parameter of the SQL query
    const char  buffer[] = {'b', 'l', '\0', 'b'}; // "bl\0b" : 4 char, with a null byte inside
    const int   size = sizeof(buffer); // size = 4
    const void* blob = &buffer;
    insert.bind(1, blob, size);
    // Execute the one-step query to insert the row
    EXPECT_EQ(1, insert.exec());
    EXPECT_EQ(1, db.getLastInsertRowid());
    EXPECT_EQ(1, db.getTotalChanges());

    EXPECT_THROW(insert.exec(), SQLite::Exception); // exec() shall throw as it needs to be reseted

    // Compile a SQL query
    SQLite::Statement   query(db, "SELECT * FROM test");
    EXPECT_STREQ("SELECT * FROM test", query.getQuery().c_str());
    EXPECT_EQ(6, query.getColumnCount ());
    query.executeStep();
    EXPECT_TRUE (query.isOk());
    EXPECT_FALSE(query.isDone());

    // validates every variant of cast operators, and conversions of types
    {
        const sqlite3_int64 id1     = query.getColumn(0); // operator int64_t()
        const int64_t       id2     = query.getColumn(0); // operator int64_t()
        const long long     id3     = query.getColumn(0); // operator int64_t()
        const long          id4     = query.getColumn(0); // operator int64_t() or long() depending on compiler/architecture
        const unsigned int  uint1   = query.getColumn(0); // operator uint32_t()
        const uint32_t      uint2   = query.getColumn(0); // operator uint32_t()
        const char*         ptxt    = query.getColumn(1); // operator const char*()
        const std::string   msg     = query.getColumn(1); // operator std::string() (or const char* with MSVC)
        const int           integer = query.getColumn(2); // operator int()
        const double        real    = query.getColumn(3); // operator double()
        const void*         pblob   = query.getColumn(4); // operator void*()
        const std::string   sblob   = query.getColumn(4); // operator std::string() (or const char* with MSVC)
        const void*         pempty  = query.getColumn(5); // operator void*()
        EXPECT_EQ(1,            id1);
        EXPECT_EQ(1,            id2);
        EXPECT_EQ(1,            id3);
        EXPECT_EQ(1,            id4);
        EXPECT_EQ(1,            uint1);
        EXPECT_EQ(1,            uint2);
        EXPECT_STREQ("first",   ptxt);
        EXPECT_EQ("first",      msg);
        EXPECT_EQ(-123,         integer);
        EXPECT_EQ(0.123,        real);
        EXPECT_EQ(0,            memcmp("bl\0b", pblob, size));
        EXPECT_EQ(size,         sblob.size());
        EXPECT_EQ(0,            memcmp("bl\0b", &sblob[0], size));
        EXPECT_EQ(NULL,         pempty);
    }

    // validates every variant of explicit getters
    {
        int64_t             id      = query.getColumn(0).getInt64();
        const unsigned int  uint1   = query.getColumn(0).getUInt();
        const uint32_t      uint2   = query.getColumn(0).getUInt();
        const char*         ptxt    = query.getColumn(1).getText();
        const std::string   msg1    = query.getColumn(1).getText();
        const std::string   msg2    = query.getColumn(1).getString();
        const int           integer = query.getColumn(2).getInt();
        const double        real    = query.getColumn(3).getDouble();
        const void*         pblob   = query.getColumn(4).getBlob();
        const std::string   sblob   = query.getColumn(4).getString();
        EXPECT_EQ(1,            id);
        EXPECT_EQ(1,            uint1);
        EXPECT_EQ(1,            uint2);
        EXPECT_STREQ("first",   ptxt);
        EXPECT_EQ("first",      msg1);
        EXPECT_EQ("first",      msg2);
        EXPECT_EQ(-123,         integer);
        EXPECT_EQ(0.123,        real);
        EXPECT_EQ(0,            memcmp("bl\0b", pblob, 4));
        EXPECT_EQ(0,            memcmp("bl\0b", &sblob[0], 4));
    }

    // Validate getBytes(), getType(), isInteger(), isNull()...
    EXPECT_EQ(SQLite::INTEGER,  query.getColumn(0).getType());
    EXPECT_EQ(true,             query.getColumn(0).isInteger());
    EXPECT_EQ(false,            query.getColumn(0).isFloat());
    EXPECT_EQ(false,            query.getColumn(0).isText());
    EXPECT_EQ(false,            query.getColumn(0).isBlob());
    EXPECT_EQ(false,            query.getColumn(0).isNull());
    EXPECT_STREQ("1",           query.getColumn(0).getText());  // convert to string
    EXPECT_EQ(1,                query.getColumn(0).getBytes()); // size of the string "1" without the null terminator
    EXPECT_EQ(SQLite::TEXT,     query.getColumn(1).getType());
    EXPECT_EQ(false,            query.getColumn(1).isInteger());
    EXPECT_EQ(false,            query.getColumn(1).isFloat());
    EXPECT_EQ(true,             query.getColumn(1).isText());
    EXPECT_EQ(false,            query.getColumn(1).isBlob());
    EXPECT_EQ(false,            query.getColumn(1).isNull());
    EXPECT_STREQ("first",       query.getColumn(1).getText());  // convert to string
    EXPECT_EQ(5,                query.getColumn(1).getBytes()); // size of the string "first"
    EXPECT_EQ(SQLite::INTEGER,  query.getColumn(2).getType());
    EXPECT_EQ(true,             query.getColumn(2).isInteger());
    EXPECT_EQ(false,            query.getColumn(2).isFloat());
    EXPECT_EQ(false,            query.getColumn(2).isText());
    EXPECT_EQ(false,            query.getColumn(2).isBlob());
    EXPECT_EQ(false,            query.getColumn(2).isNull());
    EXPECT_STREQ("-123",        query.getColumn(2).getText());  // convert to string
    EXPECT_EQ(4,                query.getColumn(2).getBytes()); // size of the string "-123"
    EXPECT_EQ(SQLite::FLOAT,    query.getColumn(3).getType());
    EXPECT_EQ(false,            query.getColumn(3).isInteger());
    EXPECT_EQ(true,             query.getColumn(3).isFloat());
    EXPECT_EQ(false,            query.getColumn(3).isText());
    EXPECT_EQ(false,            query.getColumn(3).isBlob());
    EXPECT_EQ(false,            query.getColumn(3).isNull());
    EXPECT_STREQ("0.123",       query.getColumn(3).getText());  // convert to string
    EXPECT_EQ(5,                query.getColumn(3).getBytes()); // size of the string "0.123"
    EXPECT_EQ(SQLite::BLOB,     query.getColumn(4).getType());
    EXPECT_EQ(false,            query.getColumn(4).isInteger());
    EXPECT_EQ(false,            query.getColumn(4).isFloat());
    EXPECT_EQ(false,            query.getColumn(4).isText());
    EXPECT_EQ(true,             query.getColumn(4).isBlob());
    EXPECT_EQ(false,            query.getColumn(4).isNull());
    EXPECT_STREQ("bl\0b",       query.getColumn(4).getText());  // convert to string
    EXPECT_EQ(4,                query.getColumn(4).getBytes()); // size of the blob "bl\0b" with the null char
    EXPECT_EQ(SQLite::Null,     query.getColumn(5).getType());
    EXPECT_EQ(false,            query.getColumn(5).isInteger());
    EXPECT_EQ(false,            query.getColumn(5).isFloat());
    EXPECT_EQ(false,            query.getColumn(5).isText());
    EXPECT_EQ(false,            query.getColumn(5).isBlob());
    EXPECT_EQ(true,             query.getColumn(5).isNull());
    EXPECT_STREQ("",            query.getColumn(5).getText());  // convert to string
    EXPECT_EQ(0,                query.getColumn(5).getBytes()); // size of the string "" without the null terminator

    // Use intermediate Column objects (this is not the recommended way to use the API)
    {
        const SQLite::Column id = query.getColumn(0);
        EXPECT_EQ(1, id.getInt64());
        const SQLite::Column msg = query.getColumn(1);
        EXPECT_EQ("first", msg.getString());
        const SQLite::Column integer = query.getColumn(2);
        EXPECT_EQ(-123, integer.getInt());
        const SQLite::Column dbl = query.getColumn(3);
        EXPECT_EQ(0.123, dbl.getDouble());
    }
}

TEST(Column, getName) {
    // Create a new database
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
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
