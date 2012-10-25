/**
 * @file  Transaction.h
 * @brief A Transaction is way to group multiple SQL statements into an atomic secured operation.
 *
 * Copyright (c) 2012 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include <sqlite3.h>
#include "Exception.h"

namespace SQLite
{

// Forward declaration
class Database;

/**
 * @brief RAII encapsulation of a SQLite Transaction.
 *
 * A Transaction is a way to group multiple SQL statements into an atomic secured operation;
 * either it succeeds, with all the changes committed to the database file,
 * or if it fails, all the changes are rolled back to the initial state.
 *
 * Resource Acquisition Is Initialization (RAII) means that the Transaction
 * begins in the constructor and is rollbacked in the destructor, so that there is
 * no need to worry about memory management or the validity of the underlying SQLite Connection.
 */
class Transaction
{
public:
    /**
     * @brief Begins the SQLite transaction
     *
     * Exception is thrown in case of error, then the Transaction is NOT initiated.
     */
    explicit Transaction(Database &aDatabase); // throw(SQLite::Exception);

    /**
     * @brief Safely rollback the transaction if it has not been committed.
     */
    virtual ~Transaction(void) throw(); // nothrow

    /**
     * @brief Commit the transaction.
     */
    void commit(void); // throw(SQLite::Exception);

private:
    // Transaction must not be copyable
    Transaction(void);
    Transaction(const Transaction&);
    Transaction& operator=(const Transaction&);

private:
    Database&   mDatabase;  //!< Reference to the SQLite Database Connection
    bool        mbCommited; //!< True when the last executeStep() had no more row to fetch
};


}  // namespace SQLite
