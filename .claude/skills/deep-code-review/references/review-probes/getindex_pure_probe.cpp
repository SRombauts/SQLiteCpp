#include <SQLiteCpp/SQLiteCpp.h>
#include <iostream>
int main()
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE);
    SQLite::Statement empty(db, "");
    bool caught = false;
    try
    {
        (void)empty.getIndex(":name");
    }
    catch (const SQLite::Exception &e)
    {
        caught = true;
        std::cout << "discard exception=" << e.what() << '\n';
    }
    std::cout << "discard caught=" << caught << '\n';
    try
    {
        std::cout << "used index=" << empty.getIndex(":name") << '\n';
    }
    catch (const SQLite::Exception &e)
    {
        std::cout << "used exception=" << e.what() << '\n';
    }
    SQLite::Statement valid(db, "SELECT :first, :second");
    char name[8] = ":first";
    const int first = valid.getIndex(name);
    name[1] = 's';
    name[2] = 'e';
    name[3] = 'c';
    name[4] = 'o';
    name[5] = 'n';
    name[6] = 'd';
    name[7] = '\0';
    const int second = valid.getIndex(name);
    std::cout << "valid first=" << first << " second=" << second << '\n';
}
