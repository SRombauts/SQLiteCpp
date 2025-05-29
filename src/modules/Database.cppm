/**
 * @file Exception.cppm
 * @brief Module file for SQLiteCpp Exception class.
 */

module;

#include <SQLiteCpp/Database.h>

export module sqlitecpp.database;

import sqlitecpp.sqlite3forward;

/**
 * @namespace sqlite
 * @brief The SQLite SQLite:: namespace
 */
export namespace SQLite {
    using SQLite::OPEN_READONLY;
    using SQLite::OPEN_READWRITE;
    using SQLite::OPEN_CREATE;
    using SQLite::OPEN_URI;
    using SQLite::OPEN_MEMORY;
    using SQLite::OPEN_NOMUTEX;
    using SQLite::OPEN_FULLMUTEX;
    using SQLite::OPEN_SHAREDCACHE;
    using SQLite::OPEN_PRIVATECACHE;
    using SQLite::OPEN_NOFOLLOW;
    using SQLite::OK;
    using SQLite::VERSION;
    using SQLite::VERSION_NUMBER;

    using SQLite::getLibVersion;
    using SQLite::getLibVersionNumber;

    using SQLite::Header;
    using SQLite::Database;
}
