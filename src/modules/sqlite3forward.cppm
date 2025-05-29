/**
 * @file sqlite3forward.cppm
 * @brief Module file containing forward declarations of sqlite3 structs.
 */

module;

#include <sqlite3.h>

export module sqlitecpp.sqlite3forward;

/**
 * @namespace SQLite
 * @brief The SQLite SQLite:: namespace
 */
export namespace SQLite {
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
