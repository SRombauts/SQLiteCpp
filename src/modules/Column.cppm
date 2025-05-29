/**
 * @file Column.cppm
 * @brief Module file for SQLiteCpp Column class.
 */

module;

#include <SQLiteCpp/Column.h>

export module sqlitecpp.column;

import sqlitecpp.sqlite3forward;

/**
 * @namespace sqlite
 * @brief The SQLite SQLite:: namespace
 */
export namespace SQLite {
    using SQLite::INTEGER;
    using SQLite::FLOAT;
    using SQLite::TEXT;
    using SQLite::BLOB;
    using SQLite::Null;

    using SQLite::Column;

    using SQLite::operator<<;
}
