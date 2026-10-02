#include <SQLiteCpp/SQLiteCpp.h>
template <typename InputIt, typename Visit> void consume(InputIt first, InputIt last, Visit visit)
{
    while (first != last)
        visit(*first++);
}
int main()
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    SQLite::Statement query(db, "SELECT 1");
    consume(query.begin(), query.end(), [](SQLite::Statement &row) { (void)row.getColumn(0).getInt(); });
}
