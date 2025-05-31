/**
 * @file Exception.cppm
 * @brief Module file for SQLiteCpp Exception class.
 */

module;

#include <SQLiteCpp/Exception.h>

export module sqlite.exception;

import sqlite.sqlite3forward;

/**
 * @namespace SQLite
 * @brief The SQLite SQLite:: namespace
 */
export namespace SQLite {
    using SQLite::Exception;
}

export namespace sqlite = SQLite;
