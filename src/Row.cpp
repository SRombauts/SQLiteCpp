/**
 * @file    Row.cpp
 * @ingroup SQLiteCpp
 * @brief   Container for SQLite Statement Object step
 *
 * Copyright (c) 2022 Kacper Zielinski (KacperZ155@gmail.com)
 * Copyright (c) 2015-2021 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include <SQLiteCpp/Row.h>

#include <SQLiteCpp/Exception.h>

#include <sqlite3.h>

namespace SQLite
{


    Row::Row(TStatementWeakPtr apStatement, std::size_t aID) :
        mpStatement(apStatement), ID(aID)
    {
    }

    bool Row::isColumnNull(const int aIndex) const
    {
        auto statement = mpStatement.lock();

        return (SQLITE_NULL == sqlite3_column_type(statement->getStatement(), aIndex));
    }

    const char* Row::getText(uint32_t aColumnID) const noexcept
    {
        auto statement = mpStatement.lock();


        auto pText = reinterpret_cast<const char*>(sqlite3_column_text(statement->getStatement(), aColumnID));
        return (pText ? pText : "");
    }

}  // namespace SQLite
