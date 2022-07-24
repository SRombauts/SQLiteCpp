/**
 * @file    Row.cpp
 * @ingroup SQLiteCpp
 * @brief   TODO:
 *
 * Copyright (c) 2015 Shibao HONG (shibaohong@outlook.com)
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


    Row::Row(RowExecutor::TStatementWeakPtr apRow, std::size_t aID) :
        mpRow(apRow), ID(aID)
    {
    }

    bool Row::isColumnNull(const int aIndex) const
    {
        return false;
    }

    const char* Row::getText(uint32_t aColumnID) const noexcept
    {
        auto statement = mpRow.lock();


        auto pText = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), aColumnID));
        return (pText ? pText : "");
    }

}  // namespace SQLite
