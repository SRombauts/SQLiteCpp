/**
 * @file    StatementPtr.cpp
 * @ingroup SQLiteCpp
 * @brief   Pointer for prepared SQLite Statement Object
 *
 * Copyright (c) 2022 Kacper Zielinski (KacperZ155@gmail.com)
 * Copyright (c) 2022 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include <SQLiteCpp/StatementPtr.h>

#include <SQLiteCpp/Exception.h>

#include <sqlite3.h>

namespace SQLite
{


    StatementPtr::StatementPtr(sqlite3* apSQLite, const std::string& aQuery) :
        mpConnection(apSQLite), mpStatement(prepareStatement(apSQLite, aQuery))
    {}

    sqlite3_stmt* StatementPtr::getStatement() const noexcept
    {
        return mpStatement.get();
    }

    int StatementPtr::reset() noexcept
    {
        mCurrentStep = 0;
        return sqlite3_reset(mpStatement.get());
    }

    int StatementPtr::step() noexcept
    {
        ++mCurrentStep;
        return sqlite3_step(mpStatement.get());
    }

    StatementPtr::TRawStatementPtr StatementPtr::prepareStatement(sqlite3* apConnection,
        const std::string& aQuery) const
    {
        if (!apConnection)
            throw SQLite::Exception("Can't create statement without valid database connection");

        sqlite3_stmt* statement;
        const int ret = sqlite3_prepare_v2(apConnection, aQuery.c_str(),
            static_cast<int>(aQuery.size()), &statement, nullptr);

        if (SQLITE_OK != ret)
        {
            throw SQLite::Exception(apConnection, ret);
        }

        return TRawStatementPtr(statement, [](sqlite3_stmt* stmt)
            {
                sqlite3_finalize(stmt);
            });
    }


}  // namespace SQLite
