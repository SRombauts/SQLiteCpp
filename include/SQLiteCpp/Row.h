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

private:
    TStatementWeakPtr mpStatement;
    std::size_t mID;
};

}  // namespace SQLite
