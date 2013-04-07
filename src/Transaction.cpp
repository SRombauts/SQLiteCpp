/**
 * @file    Transaction.cpp
 * @ingroup SQLiteCpp
 * @brief   A Transaction is way to group multiple SQL statements into an atomic secured operation.
 *
 * Copyright (c) 2012-2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include "Transaction.h"

#include "Database.h"


namespace SQLite
{


// Begins the SQLite transaction
Transaction::Transaction(Database& aDatabase) : // throw(SQLite::Exception)
    mDatabase(aDatabase),
    mbCommited(false)
{
    mDatabase.exec("BEGIN");
}

// Safely rollback the transaction if it has not been committed.
Transaction::~Transaction(void) throw() // nothrow
{
    if (false == mbCommited)
    {
        try
        {
            mDatabase.exec("ROLLBACK");
        }
        catch (SQLite::Exception& /*e*/)
        {
            // Never throw an exception in a destructor
            //std::cout << e.what() << std::endl;
            SQLITE_CPP_ASSERT(false);
        }
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
