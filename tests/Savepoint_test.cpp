/**
 * @file    Savepoint_test.cpp
 * @ingroup tests
 * @brief   Test of a SQLite Savepoint.
 *
 * Copyright (c) 2020 Kelvin Hammond (hammond.kelvin@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt or
 * copy at http://opensource.org/licenses/MIT)
 */

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Savepoint.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
#include <gtest/gtest.h>

#include <cstdio>

TEST(Savepoint, releaseAndRollback)
{
    // Create a new database
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    EXPECT_EQ(SQLite::OK, db.getErrorCode());
    
    {
        // Begin savepoint
        SQLite::Savepoint savepoint(db, "sp1");

        EXPECT_EQ(0, db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"));
        EXPECT_EQ(SQLite::OK, db.getErrorCode());

        // Insert a first value
        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, 'first')"));
        EXPECT_EQ(1, db.getLastInsertRowid());

        // release savepoint
        savepoint.release();

        // Releasing or rolling back an already released savepoint throws.
        EXPECT_THROW(savepoint.release(), SQLite::Exception);
        EXPECT_THROW(savepoint.rollbackTo(), SQLite::Exception);
    }

    // Automatic rollback if release() is not called before the end of scope.
    {
        // Begin savepoint
        SQLite::Savepoint savepoint(db, "sp2");

        // Insert a second value that will be rolled back.
        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, 'third')"));
        EXPECT_EQ(2, db.getLastInsertRowid());

        // end of scope: automatic rollback
    }

    // Automatic rollback of a savepoint when leaving scope because of an exception.
    try
    {
        // Begin savepoint
        SQLite::Savepoint savepoint(db, "sp3");

        // Insert a second value that will be rolled back.
        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, 'second')"));
        EXPECT_EQ(2, db.getLastInsertRowid());

        // Trigger an exception; stack unwinding destroys the savepoint and rolls it back.
        db.exec("DesiredSyntaxError to raise an exception and rollback the savepoint");

        GTEST_FATAL_FAILURE_("we should never get there");
        savepoint.release();  // We should never get there
    }
    catch (std::exception& e)
    {
        std::cout << "SQLite exception: " << e.what() << std::endl;
        // expected error, see above
    }

    // Manual rollback before the end of scope
    {
        // Begin savepoint
        SQLite::Savepoint savepoint(db, "sp4");

        // Insert a second value (that will be rolled back)
        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, 'third')"));
        EXPECT_EQ(2, db.getLastInsertRowid());

        // Roll back everything done since sp4, while keeping the savepoint active.
        savepoint.rollbackTo();

        // end of scope: normal automatic rollback/release cleanup
    }

    // Only the explicitly released first row should remain; all later rows were rolled back.
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

TEST(Savepoint, rollbackTo)
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)");

    SQLite::Savepoint savepoint(db, "sp");

    EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, 'first')"));
    EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, 'second')"));

    savepoint.rollbackTo();

    // ROLLBACK TO must undo all changes made since the savepoint was created.
    SQLite::Statement query(db, "SELECT COUNT(*) FROM test");
    ASSERT_TRUE(query.executeStep());
    EXPECT_EQ(0, query.getColumn(0).getInt());

    // ROLLBACK TO keeps the savepoint active, so release it explicitly.
    EXPECT_NO_THROW(savepoint.release());
}

TEST(Savepoint, rollbackToThenRelease)
{
    // Create a new database
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    EXPECT_EQ(SQLite::OK, db.getErrorCode());
    db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)");

    {
        SQLite::Savepoint savepoint(db, "sp");

        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, 'rolled back')"));

        // ROLLBACK TO leaves the savepoint active. New changes can still be made
        // and explicitly committed by releasing the savepoint.
        savepoint.rollbackTo();
        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, 'kept')"));
        EXPECT_NO_THROW(savepoint.release());

        // end of scope: already released, the destructor must do nothing and not throw
    }

    // The pre-rollback row is gone; only the row added after ROLLBACK TO was committed.
    SQLite::Statement count(db, "SELECT COUNT(*) FROM test");
    ASSERT_TRUE(count.executeStep());
    EXPECT_EQ(1, count.getColumn(0).getInt());

    SQLite::Statement query(db, "SELECT value FROM test");
    ASSERT_TRUE(query.executeStep());
    EXPECT_STREQ("kept", query.getColumn(0).getText());
}

TEST(Savepoint, autoRollbackAfterManualRollbackTo)
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)");

    {
        SQLite::Savepoint savepoint(db, "sp");

        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, 'before rollback')"));
        savepoint.rollbackTo();

        // ROLLBACK TO restarts the savepoint instead of ending it. Changes made
        // afterwards must therefore still be rolled back on scope exit.
        EXPECT_EQ(1, db.exec("INSERT INTO test VALUES (NULL, 'after rollback')"));
    }

    SQLite::Statement query(db, "SELECT COUNT(*) FROM test");
    ASSERT_TRUE(query.executeStep());
    EXPECT_EQ(0, query.getColumn(0).getInt());
}

TEST(Savepoint, destructorSwallowsException)
{
    // Create a new database
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    EXPECT_EQ(SQLite::OK, db.getErrorCode());
    db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)");

    // A destructor must never throw: if the rollback/release fails while the Savepoint
    // is being destroyed, the exception has to be caught and swallowed internally.
    EXPECT_NO_THROW({
        SQLite::Savepoint savepoint(db, "sp");
        // The name is quoted to 'sp' internally; release it directly behind the object's
        // back so that the destructor's ROLLBACK TO / RELEASE SAVEPOINT will fail and throw.
        db.exec("RELEASE SAVEPOINT 'sp'");
    }); // end of scope: the automatic rollback must not let the exception escape
}
