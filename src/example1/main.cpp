#include <iostream>
#include "../SQLiteC++/SQLiteC++.h"

int main (void)
{
    std::cout << "Hello SQLite.hpp\n";
    try
    {
        SQLite::Database    db("example.db3");
        std::cout << db.getFilename().c_str() << " onpened\n";

        SQLite::Statement   stmt(db, "SELECT * FROM test");
        std::cout << "statement created\n";

        while (stmt.executeStep())
        {
            std::cout << "executeStep\n";
        }
    }
    catch (std::exception& e)
    {
        std::cout << "SQLite exception: " <<  e.what() << std::endl;
    }
    std::cout << "Bye SQLite.hpp\n";

    return 0;
}
