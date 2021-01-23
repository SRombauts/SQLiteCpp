// Example using SQLiteCpp within the implementation of a SQLite3 run-time loadable extension
// SEE: https://sqlite.org/loadext.html
#include <sqlite3ext.h>
// When SQLiteCpp is built with option SQLITECPP_IN_EXTENSION=ON, its compiled objects will expect
// to find an extern "C" symbol declared by the following macro in the extension implementation.
extern "C" {
SQLITE_EXTENSION_INIT1
}
#include <SQLiteCpp/SQLiteCpp.h>
#include <iostream>

extern "C" int sqlite3_example_init(sqlite3 *rawDb, char **pzErrMsg,
                                              const sqlite3_api_routines *pApi) {
    SQLITE_EXTENSION_INIT2(pApi);

    try {
        // temporarily wrap rawDb as a SQLite::Database so we can use SQLiteCpp's conveniences
        SQLite::Database db(rawDb);
        SQLite::Statement stmt(db, "SELECT 'it works ' || ?");
        stmt.bind(1, 42);
        if (stmt.executeStep()) {
            std::cout << stmt.getColumn(0).getString() << std::endl;
        }
        // In a real extension we'd now register custom functions, virtual tables, or VFS objects,
        // any of which might also want to use SQLiteCpp wrappers for the raw connection.
        return SQLITE_OK;
    } catch (SQLite::Exception& exn) {
        std::cerr << exn.getErrorStr() << std::endl;
        return exn.getErrorCode();
    }
}
