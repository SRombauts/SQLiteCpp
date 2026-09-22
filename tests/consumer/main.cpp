#include <SQLiteCpp/SQLiteCpp.h>

#if SQLITECPP_CONSUMER_CXX_STANDARD >= 17 && !defined(SQLITECPP_HAVE_STD_FILESYSTEM)
#error "C++17-or-newer consumer should expose std::filesystem support"
#endif

int main()
{
#ifdef SQLITECPP_HAVE_STD_FILESYSTEM
    SQLite::Database db(std::filesystem::path(":memory:"), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
#else
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
#endif

    db.exec("CREATE TABLE test (value INTEGER)");
    db.exec("INSERT INTO test VALUES (42)");

    SQLite::Statement query(db, "SELECT value FROM test");
    if (!query.executeStep())
    {
        return 1;
    }

    return query.getColumn(0).getInt() == 42 ? 0 : 1;
}
