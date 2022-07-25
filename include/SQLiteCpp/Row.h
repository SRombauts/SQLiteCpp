/**
 * @file    Row.h
 * @ingroup SQLiteCpp
 * @brief   Container for SQLite Statement Object step
 *
 * Copyright (c) 2015 Shibao HONG (shibaohong@outlook.com)
 * Copyright (c) 2015-2021 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include <SQLiteCpp/StatementPtr.h>

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
    Row(TRowWeakPtr apStatement, std::size_t aID);

    std::size_t getRowNumber() const
    {
        return ID;
    }

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

    /**
     * @brief Return a pointer to the text value (NULL terminated string) of the column.
     *
     * @warning The value pointed at is only valid while the statement is valid (ie. not finalized),
     *          thus you must copy it before using it beyond its scope (to a std::string for instance).
     */
    const char* getText(uint32_t aColumnID) const noexcept;

private:
    TRowWeakPtr mpStatement;
    std::size_t ID;
};

}  // namespace SQLite
