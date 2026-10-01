#include <SQLiteCpp/SQLiteCpp.h>

#include <string>
#include <string_view>

#if SQLITECPP_CONSUMER_CXX_STANDARD >= 17 && !defined(SQLITECPP_HAVE_STD_FILESYSTEM)
#error "C++17-or-newer consumer should expose std::filesystem support"
#endif

#if !defined(__cpp_lib_filesystem) || __cpp_lib_filesystem < 201703L
#error "Consumer requires standard C++17 filesystem support"
#endif

#if !defined(__cpp_lib_string_view) || __cpp_lib_string_view < 201606L
#error "Consumer requires standard C++17 string_view support"
#endif

#ifdef SQLITECPP_EXPECTED_MSVC_VERSION
static_assert(_MSC_VER == SQLITECPP_EXPECTED_MSVC_VERSION, "Consumer must use the minimum MSVC toolset");
static_assert(_MSVC_LANG == 201703L, "Minimum MSVC toolset must compile in C++17 mode");
#endif

constexpr std::string_view MEMORY_DATABASE = ":memory:";
static_assert(MEMORY_DATABASE.size() == 8, "string_view must support constexpr operations");

int main()
{
#ifdef SQLITECPP_HAVE_STD_FILESYSTEM
    SQLite::Database db(std::filesystem::path(MEMORY_DATABASE), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
#else
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
#endif

    db.exec("CREATE TABLE test (value INTEGER)");
    db.exec("INSERT INTO test VALUES (42)");

    // Exercise string_view slices through the existing string API ahead of direct string_view support.
    const std::string_view sql = "SELECT value FROM test; unused suffix";
    SQLite::Statement slicedQuery(db, std::string(sql.substr(0, sql.find(';'))));
    if (!slicedQuery.executeStep() || slicedQuery.getColumn(0).getInt() != 42)
    {
        return 1;
    }

    SQLite::Statement query(db, "SELECT value FROM test");
    if (!query.executeStep())
    {
        return 1;
    }

    return query.getColumn(0).getInt() == 42 ? 0 : 1;
}
