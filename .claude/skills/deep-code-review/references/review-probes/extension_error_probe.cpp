#include <SQLiteCpp/Database.h>
#include <iostream>
#include <sqlite3.h>
int main()
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE);
    try
    {
        db.loadExtension("/tmp/sqlitecpp-review-database/definitely-missing-extension", nullptr);
    }
    catch (const SQLite::Exception &e)
    {
        std::cout << "fresh exception=" << e.what() << " code=" << e.getErrorCode()
                  << " extended=" << e.getExtendedErrorCode() << '\n';
    }
    char *error = nullptr;
    const int rc = sqlite3_load_extension(db.getHandle(), "/tmp/sqlitecpp-review-database/definitely-missing-extension",
                                          nullptr, &error);
    std::cout << "raw code=" << rc << " error=" << (error ? error : "null")
              << " handle_error=" << sqlite3_errmsg(db.getHandle()) << '\n';
    sqlite3_free(error);
}
