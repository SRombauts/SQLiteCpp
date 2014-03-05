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
    // TODO test
}
}
#endif


// Constructor
TEST(Database, ctor) {
    remove("test.db3");

    EXPECT_THROW(SQLite::Database unknow("_unknow.db3"), SQLite::Exception);
    SQLite::Database db("test.db3", SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
    EXPECT_STREQ("test.db3", db.getFilename().c_str());
    EXPECT_FALSE(db.tableExists("test"));
    EXPECT_FALSE(db.tableExists(std::string("test")));
    EXPECT_EQ(0, db.getLastInsertRowid());
    EXPECT_EQ(0, db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"));
    EXPECT_TRUE(db.tableExists("test"));
    EXPECT_TRUE(db.tableExists(std::string("test")));
    EXPECT_EQ(0, db.getLastInsertRowid());

    remove("test.db3");

// TODO test
//    EXPECT_FALSE(db.hasEntity(entity1));
//    EXPECT_EQ((size_t)0, db.unregisterEntity(entity1));
}
