/**
 * @file    Row.h
 * @ingroup SQLiteCpp
 * @brief   Container for SQLite Statement Object step
 *
 * Copyright (c) 2022 Kacper Zielinski (KacperZ155@gmail.com)
 * Copyright (c) 2015-2021 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include <SQLiteCpp/StatementPtr.h>
#include <SQLiteCpp/Column.h>

#include <memory>
#include <string>


namespace SQLite
{

/**
* @brief CLASS IS WIP!
*/
class Row
{
public:
    Row(TStatementWeakPtr apStatement, std::size_t aID) :
        mpStatement(apStatement), mID(aID) {}

    std::size_t getRowNumber() const
    {
        return mID;
    }

    /**
    * @brief RandomAccessIterator for row columns.
    */
    class ColumnIterator
    {
    public:
        //TODO: using iterator_category = std::random_access_iterator_tag;
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Column;
        using reference = const Column&;
        using pointer = const Column*;
        using difference_type = std::ptrdiff_t;

        ColumnIterator() = default;
        ColumnIterator(TStatementPtr apStatement, uint16_t aID) :
            mpStatement(apStatement), mID(aID), mColumn(apStatement, aID) {}

        reference operator*() const noexcept
        {
            return mColumn;
        }
        pointer operator->() const noexcept
        {
            return &mColumn;
        }

        ColumnIterator& operator++() noexcept
        {
            mColumn = Column(mpStatement, ++mID);
            return *this;
        }
        ColumnIterator operator++(int) noexcept
        {
            ColumnIterator copy{ *this };
            mColumn = Column(mpStatement, ++mID);
            return copy;
        }
        ColumnIterator& operator--() noexcept
        {
            mColumn = Column(mpStatement, --mID);
            return *this;
        }
        ColumnIterator operator--(int) noexcept
        {
            ColumnIterator copy{ *this };
            mColumn = Column(mpStatement, --mID);
            return copy;
        }

        bool operator==(const ColumnIterator& aIt) const noexcept;
        bool operator!=(const ColumnIterator& aIt) const noexcept
        {
            return !(*this == aIt);
        }

    private:
        TStatementPtr   mpStatement{};  //!< Shared pointer to prepared Statement Object
        std::size_t     mRowID{};       //!< Current row number
        uint16_t        mID{};          //!< Current column number

        /// Internal column object storage
        Column mColumn{ mpStatement, mID };
    };

private:
    TStatementWeakPtr mpStatement;
    std::size_t       mID;
};

}  // namespace SQLite
