// driver program to load a SQLite3 extension

#include <sqlite3.h>
#include <iostream>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: example_driver EXTENSION_ABSOLUTE_PATH" << std::endl;
        return -1;
    }
    sqlite3 *db;
    if (sqlite3_open_v2(":memory:", &db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
        std::cerr << "sqlite3_open_v2() failed" << std::endl;
        return -1;
    }
    char *zErrMsg = nullptr;
    if (sqlite3_load_extension(db, argv[1], nullptr, &zErrMsg) != SQLITE_OK) {
        std::cerr << "sqlite3_load_extension() failed";
        if (zErrMsg) {
            std::cerr << ": " << zErrMsg << std::endl;
        }
        std::cerr << std::endl;
        return -1;
    }
    return 0;
}
