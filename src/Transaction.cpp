/**
 * @file    Transaction.cpp
 * @ingroup SQLiteCpp
 * @brief   A Transaction is way to group multiple SQL statements into an atomic secured operation.
 *
 * Copyright (c) 2012-2025 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include <SQLiteCpp/Transaction.h>

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Assertion.h>

#include <sqlite3.h>

namespace SQLite
{

// Begins the SQLite transaction
Transaction::Transaction(Database& aDatabase, TransactionBehavior behavior) :
    mDatabase(aDatabase)
{
    const char *stmt;
    switch (behavior) {
        case TransactionBehavior::DEFERRED:
            stmt = "BEGIN DEFERRED";
            break;
        case TransactionBehavior::IMMEDIATE:
            stmt = "BEGIN IMMEDIATE";
            break;
        case TransactionBehavior::EXCLUSIVE:
            stmt = "BEGIN EXCLUSIVE";
            break;
        default:
            throw SQLite::Exception("invalid/unknown transaction behavior", SQLITE_ERROR);
    }
    mDatabase.exec(stmt);
}

// Begins the SQLite transaction
Transaction::Transaction(Database &aDatabase) :
    mDatabase(aDatabase)
{
    mDatabase.exec("BEGIN TRANSACTION");
}

// Safely rollback the transaction if it has not been committed.
Transaction::~Transaction()
{
    if (false == mbFinished)
    {
        try
        {
            mDatabase.exec("ROLLBACK TRANSACTION");
        }
        catch (SQLite::Exception& e)
        {
            // Never throw an exception in a destructor: report it through the assertion handler instead.
            SQLITECPP_ASSERT(false, e.what());
        }
        catch (...)
        {
            // Never throw an exception in a destructor.
        }
    }
}

// Commit the transaction.
void Transaction::commit()
{
    if (false == mbFinished)
    {
        mDatabase.exec("COMMIT TRANSACTION");
        mbFinished = true;
    }
    else
    {
        throw SQLite::Exception("Transaction already committed.");
    }
}

// Rollback the transaction
void Transaction::rollback()
{
    if (false == mbFinished)
    {
        mDatabase.exec("ROLLBACK TRANSACTION");
        mbFinished = true;
    }
    else
    {
        throw SQLite::Exception("Transaction already committed.");
    }
}

}  // namespace SQLite
