#include <SQLiteCpp/Assertion.h>
#include <iostream>

namespace SQLite
{
void assertion_failed(const char*, const int, const char*, const char*, const char*)
{
}
}

int main()
{
    bool bElseReached = false;
    if (false)
        SQLITECPP_ASSERT(true, "probe");
    else
        bElseReached = true;
    std::cout << "assertion outer_else_reached=" << bElseReached << '\n';
    return bElseReached ? 1 : 0;
}
