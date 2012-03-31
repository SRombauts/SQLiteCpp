#include <iostream>
#include "../sqlite.hpp"


int main (void)
{
    std::cout << "Hello SQLite.hpp\n";
    try
    {
        SQLite::Database db("sqlite.db3");
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    std::cout << "Bye SQLite.hpp\n";
    
    return 0;
}
