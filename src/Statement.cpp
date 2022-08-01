/**
 * @file    Statement.cpp
 * @ingroup SQLiteCpp
 * @brief   A prepared SQLite Statement Object binder and Column getter.
 *
 * Copyright (c) 2012-2021 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include <SQLiteCpp/Statement.h>

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Column.h>
#include <SQLiteCpp/Assertion.h>
#include <SQLiteCpp/Exception.h>

#include <sqlite3.h>

namespace SQLite
{


Statement::Statement(const Database& aDatabase, const std::string& aQuery) :
    StatementExecutor(aDatabase.getHandle(), aQuery), mQuery(aQuery)
{}

// Clears away all the bindings of a prepared statement (can be associated with #reset() above).
void Statement::clearBindings()
{
    const int ret = sqlite3_clear_bindings(getStatement());
    check(ret);
}

// Get bind parameter index
int Statement::getIndex(const char * const apName) const
{
    return sqlite3_bind_parameter_index(getStatement(), apName);
}

// Bind an 32bits int value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const int32_t aValue)
{
    const int ret = sqlite3_bind_int(getStatement(), aIndex, aValue);
    check(ret);
}

// Bind a 32bits unsigned int value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const uint32_t aValue)
{
    const int ret = sqlite3_bind_int64(getStatement(), aIndex, aValue);
    check(ret);
}

// Bind a 64bits int value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const int64_t aValue)
{
    const int ret = sqlite3_bind_int64(getStatement(), aIndex, aValue);
    check(ret);
}

// Bind a double (64bits float) value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const double aValue)
{
    const int ret = sqlite3_bind_double(getStatement(), aIndex, aValue);
    check(ret);
}

// Bind a string value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const std::string& aValue)
{
    const int ret = sqlite3_bind_text(getStatement(), aIndex, aValue.c_str(),
                                      static_cast<int>(aValue.size()), SQLITE_TRANSIENT);
    check(ret);
}

// Bind a text value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const char* apValue)
{
    const int ret = sqlite3_bind_text(getStatement(), aIndex, apValue, -1, SQLITE_TRANSIENT);
    check(ret);
}

// Bind a binary blob value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex, const void* apValue, const int aSize)
{
    const int ret = sqlite3_bind_blob(getStatement(), aIndex, apValue, aSize, SQLITE_TRANSIENT);
    check(ret);
}

// Bind a string value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bindNoCopy(const int aIndex, const std::string& aValue)
{
    const int ret = sqlite3_bind_text(getStatement(), aIndex, aValue.c_str(),
                                      static_cast<int>(aValue.size()), SQLITE_STATIC);
    check(ret);
}

// Bind a text value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bindNoCopy(const int aIndex, const char* apValue)
{
    const int ret = sqlite3_bind_text(getStatement(), aIndex, apValue, -1, SQLITE_STATIC);
    check(ret);
}

// Bind a binary blob value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bindNoCopy(const int aIndex, const void* apValue, const int aSize)
{
    const int ret = sqlite3_bind_blob(getStatement(), aIndex, apValue, aSize, SQLITE_STATIC);
    check(ret);
}

// Bind a NULL value to a parameter "?", "?NNN", ":VVV", "@VVV" or "$VVV" in the SQL prepared statement
void Statement::bind(const int aIndex)
{
    const int ret = sqlite3_bind_null(getStatement(), aIndex);
    check(ret);
}

// Return a copy of the column data specified by its index starting at 0
// (use the Column copy-constructor)
Column Statement::getColumn(const int aIndex) const
{
    checkRow();
    checkIndex(aIndex);

    // Share the Statement Object handle with the new Column created
    return Column(getStatementPtr(), aIndex);
}

// Return a copy of the column data specified by its column name starting at 0
// (use the Column copy-constructor)
Column Statement::getColumn(const char* apName) const
{
    checkRow();
    const int index = getColumnIndex(apName);

    // Share the Statement Object handle with the new Column created
    return Column(getStatementPtr(), index);
}

// Test if the column is NULL
bool Statement::isColumnNull(const int aIndex) const
{
    checkRow();
    checkIndex(aIndex);
    return (SQLITE_NULL == sqlite3_column_type(getStatement(), aIndex));
}

bool Statement::isColumnNull(const char* apName) const
{
    checkRow();
    const int index = getColumnIndex(apName);
    return (SQLITE_NULL == sqlite3_column_type(getStatement(), index));
}

// Return the named assigned to the specified result column (potentially aliased)
const char* Statement::getColumnName(const int aIndex) const
{
    checkIndex(aIndex);
    return sqlite3_column_name(getStatement(), aIndex);
}

#ifdef SQLITE_ENABLE_COLUMN_METADATA
// Return the named assigned to the specified result column (potentially aliased)
const char* Statement::getColumnOriginName(const int aIndex) const
{
    checkIndex(aIndex);
    return sqlite3_column_origin_name(getStatement(), aIndex);
}
#endif

// Return the index of the specified (potentially aliased) column name
int Statement::getColumnIndex(const char* apName) const
{
    auto& columns = getColumnsNames();

    const auto iIndex = columns.find(apName);
    if (iIndex == columns.end())
    {
        throw SQLite::Exception("Unknown column name.");
    }

    return iIndex->second;
}

const char * Statement::getColumnDeclaredType(const int aIndex) const
{
    checkIndex(aIndex);
    const char * result = sqlite3_column_decltype(getStatement(), aIndex);
    if (!result)
    {
        throw SQLite::Exception("Could not determine declared column type.");
    }
    else
    {
        return result;
    }
}

int Statement::getBindParameterCount() const noexcept
{
    return sqlite3_bind_parameter_count(getStatement());
}

// Return a UTF-8 string containing the SQL text of prepared statement with bound parameters expanded.
std::string Statement::getExpandedSQL() const {
    char* expanded = sqlite3_expanded_sql(getStatement());
    std::string expandedString(expanded);
    sqlite3_free(expanded);
    return expandedString;
}


}  // namespace SQLite
