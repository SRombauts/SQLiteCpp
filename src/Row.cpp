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
        mpStatement(apStatement), mID(aID)
    {
        checkStatement();
        auto statement = mpStatement.lock();
        mColumnCount = statement->mColumnCount;
    }

    Column Row::at(int_fast16_t aIndex) const
    {
        checkStatement();
        if (mColumnCount <= aIndex)
        {
            throw SQLite::Exception("Column index out of range.");
        }
        return Column(mpStatement.lock(), aIndex);
    }

    Column Row::at(const char* aName) const
    {
        return Column(mpStatement.lock(), getColumnIndex(aName));
    }

    int_fast16_t Row::getColumnIndex(const char* apName) const
    {
        checkStatement();
        auto statement = mpStatement.lock();

        const auto& columns = statement->mColumnNames;

        const auto iIndex = columns.find(apName);
        if (iIndex == columns.end())
        {
            throw SQLite::Exception("Unknown column name.");
        }

        return iIndex->second;
    }

    ////////////////////////////////////////////////////////////////////////////

    Row::ColumnIterator Row::begin() const
    {
        checkStatement();
        auto statement = mpStatement.lock();
        return ColumnIterator(statement, 0);
    }

    Row::ColumnIterator Row::end() const
    {
        checkStatement();
        auto statement = mpStatement.lock();
        return ColumnIterator(statement, statement->mColumnCount);
    }

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
