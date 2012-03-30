#include <sqlite3.h>
#include <stdexcept>

namespace SQLite
{

class Database
{
public:
    explicit Database(const char* apFilenameUt8, const bool abReadOnly = false) :
        mpSQLite(NULL)
    {
        int err = sqlite3_open_v2(apFilenameUt8, &mpSQLite, abReadOnly?SQLITE_OPEN_READONLY:SQLITE_OPEN_READWRITE, NULL);
        if (SQLITE_OK != err)
        {
            std::string strerr = sqlite3_errmsg(mpSQLite);
            sqlite3_close(mpSQLite);
            throw std::runtime_error(strerr);
        }
    }
    virtual ~Database(void)
    {
        sqlite3_close(mpSQLite);
    }
    
private:
    sqlite3* mpSQLite;
};

};
