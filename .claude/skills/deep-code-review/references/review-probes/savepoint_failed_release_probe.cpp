#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Savepoint.h>
#include <SQLiteCpp/Statement.h>
#include <iostream>
int main()
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("PRAGMA foreign_keys=ON");
    db.exec("CREATE TABLE p(id INTEGER PRIMARY KEY)");
    db.exec("CREATE TABLE c(id INTEGER REFERENCES p(id) DEFERRABLE INITIALLY DEFERRED)");
    SQLite::Savepoint p(db, "outer");
    db.exec("INSERT INTO c VALUES(1)");
    try
    {
        p.release();
        std::cout << "unexpected_success\n";
    }
    catch (const SQLite::Exception &e)
    {
        std::cout << "failed_release=" << e.what() << '\n';
    }
    db.exec("INSERT INTO p VALUES(1)");
    p.release();
    std::cout << "retry_rows=" << db.execAndGet("SELECT count(*) FROM c").getInt() << '\n';
}
