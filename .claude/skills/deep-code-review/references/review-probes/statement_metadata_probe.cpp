#include <SQLiteCpp/SQLiteCpp.h>
#include <iostream>
int main()
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("CREATE TABLE t(a INTEGER, b INTEGER)");
    SQLite::Statement query(db, "SELECT * FROM t");
    query.getColumnIndex("a");
    db.exec("DROP TABLE t");
    db.exec("CREATE TABLE t(b INTEGER, a INTEGER)");
    db.exec("INSERT INTO t VALUES(9,7)");
    query.executeStep();
    std::cout << "actual_name0=" << query.getColumnName(0) << " named_a=" << query.getColumn("a").getInt()
              << " actual_a=" << query.getColumn(1).getInt() << '\n';
    query.reset();
    db.exec("ALTER TABLE t ADD COLUMN c INTEGER DEFAULT 11");
    query.executeStep();
    std::cout << "count_after_add=" << query.getColumnCount() << '\n';
    try
    {
        std::cout << query.getColumn(2).getInt() << '\n';
    }
    catch (const SQLite::Exception &e)
    {
        std::cout << "added_column_error=" << e.what() << '\n';
    }
    query.reset();
    db.exec("DROP TABLE t");
    db.exec("CREATE TABLE t(a INTEGER)");
    db.exec("INSERT INTO t VALUES(42)");
    query.executeStep();
    std::cout << "count_after_remove=" << query.getColumnCount() << " named_b=" << query.getColumn("b").getInt()
              << '\n';
    try
    {
        std::cout << "obsolete_column1=" << query.getColumn(1).getInt() << '\n';
    }
    catch (const SQLite::Exception &e)
    {
        std::cout << "obsolete_error=" << e.what() << '\n';
    }
}
