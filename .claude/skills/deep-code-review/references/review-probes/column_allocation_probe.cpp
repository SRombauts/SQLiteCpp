#include <SQLiteCpp/SQLiteCpp.h>
#include <cstdlib>
#include <iostream>
#include <sqlite3.h>
#include <sstream>
#include <string>

static sqlite3_mem_methods original;
static int failures = 0;
static int failedCalls = 0;
static void *failMalloc(int size)
{
    if (failures != 0)
    {
        if (failures > 0)
            --failures;
        ++failedCalls;
        return nullptr;
    }
    return original.xMalloc(size);
}
static void *failRealloc(void *pointer, int size)
{
    if (failures != 0)
    {
        if (failures > 0)
            --failures;
        ++failedCalls;
        return nullptr;
    }
    return original.xRealloc(pointer, size);
}
int main(int argc, char **argv)
{
    sqlite3_shutdown();
    sqlite3_config(SQLITE_CONFIG_GETMALLOC, &original);
    sqlite3_mem_methods modified = original;
    modified.xMalloc = failMalloc;
    modified.xRealloc = failRealloc;
    if (sqlite3_config(SQLITE_CONFIG_MALLOC, &modified) != SQLITE_OK)
        return 2;
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    sqlite3_db_config(db.getHandle(), SQLITE_DBCONFIG_LOOKASIDE, nullptr, 0, 0);
    db.exec("PRAGMA encoding='UTF-16'; CREATE TABLE t(value TEXT)");
    const std::string expected(4000, 'X');
    SQLite::Statement insert(db, "INSERT INTO t VALUES(?)");
    insert.bind(1, expected);
    insert.exec();
    const bool blob = argc > 2;
    SQLite::Statement query(db, blob ? "SELECT zeroblob(4000)" : "SELECT value FROM t");
    query.executeStep();
    failures = argc > 1 ? std::atoi(argv[1]) : -1;
    bool threw = false;
    std::string result;
    try
    {
        result = query.getColumn(0).getString();
    }
    catch (const std::exception &)
    {
        threw = true;
    }
    failures = 0;
    const int error = db.getErrorCode();
    std::cout << "mode=" << (blob ? "zeroblob" : "utf16") << " failedCalls=" << failedCalls << " threw=" << threw
              << " resultSize=" << result.size() << " expectedSize=" << expected.size()
              << " equalsExpected=" << (blob ? result == std::string(4000, '\0') : result == expected)
              << " sqliteError=" << error << '\n';
    return 0;
}
