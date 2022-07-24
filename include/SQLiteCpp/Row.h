/**
 * @file    Row.h
 * @ingroup SQLiteCpp
 * @brief   TODO:
 *
 * Copyright (c) 2015 Shibao HONG (shibaohong@outlook.com)
 * Copyright (c) 2015-2021 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include <SQLiteCpp/RowExecutor.h>

#include <string>

// Forward declaration to avoid inclusion of <sqlite3.h> in a header
struct sqlite3_stmt;
class Row;

namespace SQLite
{


class Row
{
public:
    Row(RowExecutor::TRowPtr apRow, std::size_t aID);

    /**
     * @brief Test if the column value is NULL
     *
     * @param[in] aIndex    Index of the column, starting at 0
     *
     * @return true if the column value is NULL
     *
     *  Throw an exception if the specified index is out of the [0, getColumnCount()) range.
     */
    bool    isColumnNull(const int aIndex) const;

private:
    RowExecutor::TRowWeakPtr mpRow;
    std::size_t ID;
};

}  // namespace SQLite
