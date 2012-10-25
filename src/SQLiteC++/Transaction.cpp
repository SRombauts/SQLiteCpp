/**
 * @file  Transaction.cpp
 * @brief A prepared SQLite Transaction is a compiled SQL query ready to be executed.
 *
 * Copyright (c) 2012 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include "Transaction.h"

#include "Database.h"

namespace SQLite
{

// Compile and register the SQL query for the provided SQLite Database Connection
Transaction::Transaction(Database &aDatabase) : // throw(SQLite::Exception)
    mDatabase(aDatabase),
    mbCommited(false)
{
    mDatabase.exec("BEGIN");
}

// Finalize and unregister the SQL query from the SQLite Database Connection.
Transaction::~Transaction(void) throw() // nothrow
{
    if (false == mbCommited)
    {
        mDatabase.exec("ROLLBACK");
    }
}

// Commit the transaction.
void Transaction::commit(void) // throw(SQLite::Exception)
{
    if (false == mbCommited)
    {
        mDatabase.exec("COMMIT");
        mbCommited = true;
    }
    else
    {
        throw SQLite::Exception("Transaction already commited");
    }
}


}  // namespace SQLite
