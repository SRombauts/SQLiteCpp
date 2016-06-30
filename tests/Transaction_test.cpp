/**
 * @file    Transaction_test.cpp
 * @ingroup tests
 * @brief   Test of a SQLite Transaction.
 *
 * Copyright (c) 2012-2016 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <SQLiteCpp/Transaction.h>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Exception.h>

#include <gtest/gtest.h>

#include <cstdio>

TEST(Transaction, commitRollback) {
    // Create a new database
    SQLite::Database db(":memory:", SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
    EXPECT_EQ(SQLITE_OK, db.getErrorCode());

    {
        // Begin transaction
        SQLite::Transaction transaction(db);

        EXPECT_EQ(0, db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"));
        EXPECT_EQ(SQLITE_OK, db.getErrorCode());

        // Insert a first value
        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, \"first\")"));
        EXPECT_EQ(1, db.getLastInsertRowid());

        // Commit transaction
        transaction.commit();

        // Commit again throw an exception
        EXPECT_THROW(transaction.commit(), SQLite::Exception);
    }

    // Auto rollback of a transaction on error
    try
    {
        // Begin transaction
        SQLite::Transaction transaction(db);

        // Insert a second value (that will be rollbacked)
        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, \"second\")"));
        EXPECT_EQ(2, db.getLastInsertRowid());

        // Execute with an error to rollback
        db.exec("DesiredSyntaxError to raise an exception to rollback the transaction");
        GTEST_FATAL_FAILURE_("we should never get there");

        // Commit transaction
        transaction.commit();
    }
    catch (std::exception& e)
    {
        std::cout << "SQLite exception: " << e.what() << std::endl;
        // expected error, see above
    }

    // Check the results (expect only one row of result, as the second one has been rollbacked by the error)
    SQLite::Statement query(db, "SELECT * FROM test");
    int nbRows = 0;
    while (query.executeStep())
    {
        nbRows++;
        EXPECT_EQ(1, query.getColumn(0).getInt());
        EXPECT_STREQ("first", query.getColumn(1).getText());
    }
    EXPECT_EQ(1, nbRows);
}
