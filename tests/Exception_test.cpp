/**
 * @file    Transaction_test.cpp
 * @ingroup tests
 * @brief   Test of a SQLite Transaction.
 *
 * Copyright (c) 2012-2019 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <SQLiteCpp/Exception.h>

#include <gtest/gtest.h>

#include <string>

TEST(Exception, copy)
{
    const SQLite::Exception ex1("some error", 2);
    const SQLite::Exception ex2 = ex1;
    EXPECT_STREQ(ex1.what(), ex2.what());
    EXPECT_EQ(ex1.getErrorCode(), ex2.getErrorCode());
    EXPECT_EQ(ex1.getExtendedErrorCode(), ex2.getExtendedErrorCode());
}

// see http://eel.is/c++draft/exception#2 or http://www.cplusplus.com/reference/exception/exception/operator=/
// an assignment operator is expected to be avaiable
TEST(Exception, assignment)
{
    const SQLite::Exception ex1("some error", 2);
    SQLite::Exception ex2("some error2", 3);

    ex2 = ex1;

    EXPECT_STREQ(ex1.what(), ex2.what());
    EXPECT_EQ(ex1.getErrorCode(), ex2.getErrorCode());
    EXPECT_EQ(ex1.getExtendedErrorCode(), ex2.getExtendedErrorCode());
}

TEST(Exception, throw_catch)
{
    const char message[] = "some error";
    try
    {
        throw SQLite::Exception(message);
    }
    catch (const std::runtime_error& ex)
    {
        EXPECT_STREQ(ex.what(), message);
    }
}


TEST(Exception, constructor)
{
    const char msg1[] = "error msg";
    std::string msg2 = msg1;
    {
        const SQLite::Exception ex1(msg1);
        const SQLite::Exception ex2(msg2);
        EXPECT_STREQ(ex1.what(), ex2.what());
        EXPECT_EQ(ex1.getErrorCode(), ex2.getErrorCode());
        EXPECT_EQ(ex1.getExtendedErrorCode(), ex2.getExtendedErrorCode());
    }
    {
        const SQLite::Exception ex1(msg1, 1);
        const SQLite::Exception ex2(msg2, 1);
        EXPECT_STREQ(ex1.what(), ex2.what());
        EXPECT_EQ(ex1.getErrorCode(), ex2.getErrorCode());
        EXPECT_EQ(ex1.getExtendedErrorCode(), ex2.getExtendedErrorCode());
    }
}
