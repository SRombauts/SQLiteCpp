#include <SQLiteCpp/SQLiteCpp.h>
void visit(SQLite::Statement &query)
{
    auto it = query.begin();
    (void)it->getColumn(0).getInt();
}
