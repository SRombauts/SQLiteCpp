/**
 * @file Transaction.cppm
 * @brief Module file for SQLiteCpp Transaction class.
 */

module;

#include <SQLiteCpp/Transaction.h>

export module sqlite.transaction;

export import sqlite.database;

import sqlite.sqlite3forward;

/**
 * @namespace SQLite
 * @brief The SQLite SQLite:: namespace
 */
export namespace SQLite {
    using SQLite::TransactionBehavior;
    using SQLite::Transaction;
}

export namespace sqlite = SQLite;
