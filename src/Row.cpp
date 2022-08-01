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


    bool Row::ColumnIterator::operator==(const ColumnIterator& aIt) const noexcept
    {
        const auto& left = mpStatement;
        const auto& right = aIt.mpStatement;

        if (mID != aIt.mID)
            return false;

        if (!left && !right)
            return true;

        if (left != right)
            return false;

        return mRowID == aIt.mRowID;
    }


}  // namespace SQLite
