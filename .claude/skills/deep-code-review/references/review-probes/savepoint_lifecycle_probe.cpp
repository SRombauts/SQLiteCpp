#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Savepoint.h>
#include <SQLiteCpp/Statement.h>
#include <iostream>
#include <memory>
#include <sqlite3.h>
int rows(SQLite::Database &db)
{
    return db.execAndGet("SELECT count(*) FROM t").getInt();
}
void parentRelease()
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("CREATE TABLE t(value INTEGER)");
    SQLite::Savepoint parent(db, "parent");
    auto oldChild = std::make_unique<SQLite::Savepoint>(db, "child");
    parent.release();
    SQLite::Savepoint replacement(db, "child");
    db.exec("INSERT INTO t VALUES(9)");
    oldChild.reset();
    std::cout << "parent_release successor_rows=" << rows(db)
              << " autocommit=" << sqlite3_get_autocommit(db.getHandle()) << '\n';
    try
    {
        replacement.release();
        std::cout << "unexpected_success\n";
    }
    catch (const SQLite::Exception &e)
    {
        std::cout << "successor_release_error=" << e.what() << '\n';
    }
}
void parentRollback()
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("CREATE TABLE t(value INTEGER)");
    SQLite::Savepoint parent(db, "parent");
    auto oldChild = std::make_unique<SQLite::Savepoint>(db, "child");
    parent.rollbackTo();
    SQLite::Savepoint replacement(db, "child");
    db.exec("INSERT INTO t VALUES(11)");
    oldChild.reset();
    parent.release();
    std::cout << "parent_rollback successor_rows=" << rows(db) << '\n';
}
void oldRelease()
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("CREATE TABLE t(value INTEGER)");
    SQLite::Savepoint parent(db, "parent");
    SQLite::Savepoint oldChild(db, "child");
    parent.release();
    auto replacement = std::make_unique<SQLite::Savepoint>(db, "child");
    db.exec("INSERT INTO t VALUES(13)");
    oldChild.release();
    replacement.reset();
    std::cout << "old_release successor_rows=" << rows(db) << " autocommit=" << sqlite3_get_autocommit(db.getHandle())
              << '\n';
}
void quoteBoundary()
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("CREATE TABLE t(value INTEGER)");
    {
        SQLite::Savepoint p(db, "x'; DROP TABLE t; --");
        p.release();
    }
    std::cout << "quoted_name_table_exists=" << db.tableExists("t") << '\n';
    {
        SQLite::Savepoint p(db, std::string("prefix\0suffix", 13));
        db.exec("RELEASE 'prefix'");
    }
    std::cout << "nul_name_aliases_prefix=1\n";
}
int main()
{
    parentRelease();
    parentRollback();
    oldRelease();
    quoteBoundary();
}
