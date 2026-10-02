#include <SQLiteCpp/SQLiteCpp.h>
#include <sqlite3.h>

#include <cstdlib>
#include <iostream>
#include <new>

static bool bFailNextAllocation = false;

void* operator new(std::size_t aSize)
{
    if (bFailNextAllocation)
    {
        bFailNextAllocation = false;
        throw std::bad_alloc();
    }
    if (void* pValue = std::malloc(aSize))
    {
        return pValue;
    }
    throw std::bad_alloc();
}

void operator delete(void* pValue) noexcept
{
    std::free(pValue);
}

void operator delete(void* pValue, std::size_t) noexcept
{
    std::free(pValue);
}

int main()
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    SQLite::Statement query(db, "SELECT 'a deliberately long literal exceeding the C++ small string buffer'");
    const sqlite3_int64 before = sqlite3_memory_used();
    try
    {
        bFailNextAllocation = true;
        query.getExpandedSQL();
        std::cout << "allocation unexpectedly succeeded" << std::endl;
        return 1;
    }
    catch (const std::bad_alloc&)
    {
        const sqlite3_int64 after = sqlite3_memory_used();
        std::cout << "caught bad_alloc; retained SQLite bytes=" << (after - before) << std::endl;
        return after > before ? 0 : 2;
    }
}
