/**
 * @file Statement.cppm
 * @brief Module file for SQLiteCpp Statement class.
 */

module;

#include <SQLiteCpp/Statement.h>

export module sqlite.statement;

export import sqlite.column;
export import sqlite.database;

import sqlite.sqlite3forward;

/**
 * @namespace SQLite
 * @brief The SQLite SQLite:: namespace
 */
export namespace SQLite {
    using SQLite::Statement;
}

export namespace sqlite = SQLite;
