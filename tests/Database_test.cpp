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
    SQLite::Database database("test.db3", SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
// TODO test
//    EXPECT_FALSE(database.hasEntity(entity1));
//    EXPECT_EQ((size_t)0, database.unregisterEntity(entity1));
}
