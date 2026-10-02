/**
 * @file    Transaction_test.cpp
 * @ingroup tests
 * @brief   Test of a SQLite Transaction.
 *
 * Copyright (c) 2012-2026 Sebastien Rombauts (sebastien.rombauts@gmail.com)
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

TEST(Transaction, commitRollback)
{
    // Create a new database
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
    EXPECT_EQ(SQLite::OK, db.getErrorCode());

    {
        // Begin transaction
        SQLite::Transaction transaction(db);

        EXPECT_EQ(0, db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"));
        EXPECT_EQ(SQLite::OK, db.getErrorCode());

        // Insert a first value
        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, 'first')"));
        EXPECT_EQ(1, db.getLastInsertRowid());

        // Commit transaction
        transaction.commit();

        // Committing an already finished transaction throws.
        EXPECT_THROW(transaction.commit(), SQLite::Exception);

        // Rolling back an already finished transaction also throws.
        EXPECT_THROW(transaction.rollback(), SQLite::Exception);
    }

    // ensure transactions with different types are well-formed
    {
        for (auto behavior : {
            SQLite::TransactionBehavior::DEFERRED,
            SQLite::TransactionBehavior::IMMEDIATE,
            SQLite::TransactionBehavior::EXCLUSIVE })
        {
            SQLite::Transaction transaction(db, behavior);
            transaction.commit();
        }

        EXPECT_THROW(SQLite::Transaction(db, static_cast<SQLite::TransactionBehavior>(-1)), SQLite::Exception);
    }

    // Automatic rollback if commit() is not called before the end of scope.
    {
        // Begin transaction
        SQLite::Transaction transaction(db);

        // Insert a second value (that will be rolled back)
        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, 'third')"));
        EXPECT_EQ(2, db.getLastInsertRowid());

        // end of scope: automatic rollback
    }

    // Automatic rollback when leaving scope because of an exception.
    try
    {
        // Begin transaction
        SQLite::Transaction transaction(db);

        // Insert a second value (that will be rolled back)
        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, 'second')"));
        EXPECT_EQ(2, db.getLastInsertRowid());

        // Trigger an exception; stack unwinding destroys the transaction and rolls it back.
        db.exec("DesiredSyntaxError to raise an exception to rollback the transaction");

        GTEST_FATAL_FAILURE_("we should never get there");
        transaction.commit(); // We should never get there
    }
    catch (std::exception& e)
    {
        std::cout << "SQLite exception: " << e.what() << std::endl;
        // expected error, see above
    }

    // Manual rollback before the end of scope
    {
        SQLite::Transaction transaction(db);

        // Insert a second value that will be rolled back.
        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, 'third')"));
        EXPECT_EQ(2, db.getLastInsertRowid());

        // A manual rollback finishes the transaction; the destructor has nothing left to do.
        transaction.rollback();
    }

    // Only the explicitly committed first row should remain; all later rows were rolled back.
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

TEST(Transaction, manualRollbackFinishesTransaction)
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)");

    {
        SQLite::Transaction transaction(db);
        transaction.rollback();

        // The Transaction object is finished after rollback(). A new transaction
        // on the same connection must therefore be left untouched by its destructor.
        db.exec("BEGIN TRANSACTION");
        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, 'kept')"));
    }

    // If the finished Transaction destructor issued another ROLLBACK, this COMMIT
    // would fail and the inserted row would be lost.
    EXPECT_NO_THROW(db.exec("COMMIT TRANSACTION"));

    SQLite::Statement query(db, "SELECT value FROM test");
    ASSERT_TRUE(query.executeStep());
    EXPECT_STREQ("kept", query.getColumn(0).getText());
}
