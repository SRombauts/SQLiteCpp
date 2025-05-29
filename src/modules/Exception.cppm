/**
 * @file Exception.cppm
 * @brief Module file for SQLiteCpp Exception class.
 */

module;

#include <SQLiteCpp/Exception.h>

export module sqlitecpp.exception;

import sqlitecpp.sqlite3forward;

/**
 * @namespace sqlite
 * @brief The SQLite SQLite:: namespace
 */
export namespace SQLite {
    using SQLite::Exception;
}
