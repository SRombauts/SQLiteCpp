/**
 * @file assertion_handler.cpp
 * @ingroup tests
 * @brief Check the public Meson dependency with an application-defined assertion handler.
 */

#include <SQLiteCpp/SQLiteCpp.h>

#ifndef SQLITECPP_ENABLE_ASSERT_HANDLER
#error "The Meson dependency must expose SQLITECPP_ENABLE_ASSERT_HANDLER"
#endif

namespace SQLite
{
static int gAssertionCalls = 0;

void assertion_failed(const char*, const int, const char*, const char*, const char*)
{
    ++gAssertionCalls;
}
} // namespace SQLite

int main()
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("CREATE TABLE test (value INTEGER)");

    SQLITECPP_ASSERT(false, "custom handler smoke test");
    return (SQLite::gAssertionCalls == 1) ? 0 : 1;
}
