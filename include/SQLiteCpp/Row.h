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

#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/StatementPtr.h>
#include <SQLiteCpp/Column.h>

#include <iterator>
#include <memory>
#include <string>


namespace SQLite
{

/**
* @brief Small class returned by StatementExecutor::RowIterator.
* 
* Use with for-range to iterate on statement columns.
*/
class Row
{
public:
    Row(TStatementWeakPtr apStatement, std::size_t aID);

    /**
    * @return Row ID/steps executed since statement start (starting at 0).
    */
    std::size_t getRowNumber() const noexcept
    {
        return mID;
    }

    ////////////////////////////////////////////////////////////////////////////

    /**
    * @return Column with given index
    *
    * @throws SQLite::Exception when index is out of bounds
    */
    Column at(int_fast16_t aIndex) const;
    /**
    * @return Column with given name
    *
    * @throws SQLite::Exception when there is no column with given name
    */
    Column at(const char* aName) const;

    /**
    * @return Column with given index
    *
    * @note Alias of Column::at()
    * 
    * @throws SQLite::Exception when index is out of bounds
    */
    Column getColumn(int_fast16_t aIndex) const
    {
        return at(aIndex);
    }
    /**
    * @return Column with given name
    *
    * @note Alias of Column::at()
    * 
    * @throws SQLite::Exception when there is no column with given name
    */
    Column getColumn(const char* aName) const
    {
        return at(aName);
    }

    /**
    * @return Column with given index
    *
    * @note Alias of Column::at()
    * 
    * @throws SQLite::Exception when index is out of bounds
    */
    Column operator[](int_fast16_t aIndex) const
    {
        return at(aIndex);
    }
    /**
    * @return Column with given name
    *
    * @note Alias of Column::at()
    * 
    * @throws SQLite::Exception when there is no column with given name
    */
    Column operator[](const char* aName) const
    {
        return at(aName);
    }

    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Return the index of the specified (potentially aliased) column name
     *
     * @param[in] apName Aliased name of the column, that is, the named specified in the query (not the original name)
     *
     * @throws SQLite::Exception if the specified name is not known.
     */
    int_fast16_t getColumnIndex(const char* apName) const;

    ////////////////////////////////////////////////////////////////////////////

    /**
    * @brief RandomAccessIterator for row columns.
    * 
    * Use by using for-range on Row or by calling Row::begin().
    */
    class ColumnIterator
    {
    public:
        // TODO: using iterator_category = std::random_access_iterator_tag;
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Column;
        using reference = const Column&;
        using pointer = const Column*;
        using difference_type = std::ptrdiff_t;

        ColumnIterator() = default;
        ColumnIterator(TStatementPtr apStatement, int_fast16_t aID) :
            mpStatement(apStatement), mRowID(apStatement->mCurrentStep), mID(aID), mColumn(apStatement, aID) {}

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
        TStatementPtr   mpStatement;  //!< Shared pointer to prepared Statement Object
        std::size_t     mRowID;       //!< Current row number
        int_fast16_t    mID;          //!< Current column number

        /// Internal column object storage
        Column mColumn{ mpStatement, mID };
    };

    /**
    * @return RowIterator to first column of this prepared statement.
    */
    ColumnIterator begin() const;

    /**
    * @return RowIterator to after the last column of this prepared statement.
    */
    ColumnIterator end() const;

private:
    /**
    * @brief Checks if weak_ptr contains existing SQLite Statement Object.
    * 
    * @throws SQLite::Exception when weak_ptr is expired.
    */
    void checkStatement() const
    {
        if (mpStatement.expired())
        {
            throw SQLite::Exception("Row is used after destruction of SQLite Statement Object");
        }
    }

    TStatementWeakPtr   mpStatement;    //!< Weak Pointer to the prepared SQLite Statement Object
    std::size_t         mID;            //!< Index of the statement row, starting at 0
    int_fast16_t        mColumnCount{}; //!< Number of columns in row
};

}  // namespace SQLite
