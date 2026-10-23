/**
 * @file column_metadata.cpp
 * @ingroup tests
 * @brief Verify column metadata through the public Meson dependency.
 */

#include <SQLiteCpp/SQLiteCpp.h>

#include <cstdlib>
#include <cstring>

#ifdef SQLITECPP_ENABLE_ASSERT_HANDLER
namespace SQLite
{
void assertion_failed(const char*, const int, const char*, const char*, const char*)
{
    std::abort();
}
} // namespace SQLite
#endif

#if SQLITECPP_EXPECT_COLUMN_METADATA
#ifndef SQLITE_ENABLE_COLUMN_METADATA
#error "The Meson dependency must expose enabled column metadata APIs"
#endif
#else
#ifdef SQLITE_ENABLE_COLUMN_METADATA
#error "The Meson dependency must not expose disabled column metadata APIs"
#endif
#endif

int main()
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("CREATE TABLE test (value INTEGER)");
    db.exec("INSERT INTO test VALUES (42)");

    SQLite::Statement query(db, "SELECT value AS alias FROM test");

#ifdef SQLITE_ENABLE_COLUMN_METADATA
    const char* pStatementOrigin = query.getColumnOriginName(0);
    if (!pStatementOrigin || std::strcmp(pStatementOrigin, "value") != 0)
    {
        return 1;
    }
    const char* pStatementTable = query.getColumnTableName(0);
    if (!pStatementTable || std::strcmp(pStatementTable, "test") != 0)
    {
        return 1;
    }
#endif

    if (!query.executeStep() || query.getColumn(0).getInt() != 42)
    {
        return 1;
    }

#ifdef SQLITE_ENABLE_COLUMN_METADATA
    const char* pColumnOrigin = query.getColumn(0).getOriginName();
    if (!pColumnOrigin || std::strcmp(pColumnOrigin, "value") != 0)
    {
        return 1;
    }
    const char* pColumnTable = query.getColumn(0).getTableName();
    if (!pColumnTable || std::strcmp(pColumnTable, "test") != 0)
    {
        return 1;
    }
#endif

    return 0;
}
