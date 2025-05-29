/**
 * @file Statement.cppm
 * @brief Module file for SQLiteCpp Statement class.
 */

module;

#include <SQLiteCpp/Statement.h>

export module sqlitecpp.statement;

export import sqlitecpp.column;
export import sqlitecpp.database;

import sqlitecpp.sqlite3forward;

/**
 * @namespace SQLite
 * @brief The SQLite SQLite:: namespace
 */
export namespace SQLite {
    using SQLite::Statement;
}
