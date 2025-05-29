/**
 * @file Transaction.cppm
 * @brief Module file for SQLiteCpp Transaction class.
 */

module;

#include <SQLiteCpp/Transaction.h>

export module sqlitecpp.transaction;

export import sqlitecpp.database;

import sqlitecpp.sqlite3forward;

/**
 * @namespace sqlite
 * @brief The SQLite SQLite:: namespace
 */
export namespace SQLite {
    using SQLite::TransactionBehavior;
    using SQLite::Transaction;
}
