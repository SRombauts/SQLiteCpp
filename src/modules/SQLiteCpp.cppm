/**
 * @file    SQLiteCpp.cppm
 * @ingroup SQLiteCpp
 * @brief   The module file for SQLiteCpp.
 *
 * Copyright (c) 2012-2026 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

module;

#include <sqlite3.h>
#include <SQLiteCpp/Column.h>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>

export module sqlite;

/**
 * @brief The C sqlite3 symbols, re-exported.
 */
export {
    using ::sqlite3;
    using ::sqlite3_context;
    using ::sqlite3_stmt;
    using ::sqlite3_value;
    
    using ::sqlite3_open;
    using ::sqlite3_close;
    using ::sqlite3_prepare_v2;
    using ::sqlite3_step;
    using ::sqlite3_finalize;
    using ::sqlite3_bind_text;
    using ::sqlite3_bind_int;
    using ::sqlite3_bind_double;
    using ::sqlite3_column_text;
    using ::sqlite3_column_int;
    using ::sqlite3_column_double;
    using ::sqlite3_errmsg;
    using ::sqlite3_errcode;
}

/**
 * @namespace SQLite
 * @brief The SQLite namespace
 */
export namespace SQLite {
    using SQLite::INTEGER;
    using SQLite::FLOAT;
    using SQLite::TEXT;
    using SQLite::BLOB;
    using SQLite::Null;

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

    using SQLite::Column;
    using SQLite::Database;
    using SQLite::Exception;
    using SQLite::Header;
    using SQLite::Statement;
    using SQLite::Transaction;
    using SQLite::TransactionBehavior;

    using SQLite::getLibVersion;
    using SQLite::getLibVersionNumber;

    using SQLite::operator<<;
}

export namespace sqlite = SQLite;
