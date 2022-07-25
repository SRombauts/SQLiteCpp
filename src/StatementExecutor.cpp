/**
 * @file    StatementExecutor.cpp
 * @ingroup SQLiteCpp
 * @brief   Step executor for SQLite prepared Statement Object
 *
 * Copyright (c) 2012-2021 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include <SQLiteCpp/StatementExecutor.h>

#include <SQLiteCpp/Exception.h>

#include <sqlite3.h>

namespace SQLite
{


    StatementExecutor::StatementExecutor(sqlite3* apSQLite, const std::string& aQuery) :
        mpStatement(std::make_shared<StatementPtr>(apSQLite, aQuery))
    {
        createColumnInfo();
    }

    void StatementExecutor::createColumnInfo()
    {
        mColumnCount = sqlite3_column_count(mpStatement->getPreparedStatement());


        if (!mColumnNames.empty())
            mColumnNames.clear();

        // Build the map of column name and index
        for (int i = 0; i < mColumnCount; ++i)
        {
            const char* pName = sqlite3_column_name(getPreparedStatement(), i);
            mColumnNames.emplace(pName, i);
        }
    }

    bool StatementExecutor::checkReturnCode(int aReturnCode) const
    {
        if (aReturnCode == getErrorCode())
        {
            throw SQLite::Exception(mpStatement->mpConnection, aReturnCode);
        }
        return true;
    }

    bool StatementExecutor::checkReturnCode(int aReturnCode, int aErrorCode) const
    {
        if (aReturnCode == aErrorCode)
        {
            throw SQLite::Exception(mpStatement->mpConnection, aReturnCode);
        }
        return true;
    }

    // Reset the statement to make it ready for a new execution (see also #clearBindings() bellow)
    void StatementExecutor::reset()
    {
        const int ret = tryReset();
        check(ret);
    }

    int StatementExecutor::tryReset() noexcept
    {
        mbHasRow = false;
        mbDone = false;
        return mpStatement->reset();
    }

    // Execute a step of the query to fetch one row of results
    bool StatementExecutor::executeStep()
    {
        const int ret = tryExecuteStep();
        if ((SQLITE_ROW != ret) && (SQLITE_DONE != ret)) // on row or no (more) row ready, else it's a problem
        {
            if (checkReturnCode(ret))
            {
                throw SQLite::Exception("Statement needs to be reseted", ret);
            }
        }

        return mbHasRow; // true only if one row is accessible by getColumn(N)
    }

    // Execute a one-step query with no expected result, and return the number of changes.
    int StatementExecutor::exec()
    {
        const int ret = tryExecuteStep();
        if (SQLITE_DONE != ret) // the statement has finished executing successfully
        {
            if (SQLITE_ROW == ret)
            {
                throw SQLite::Exception("exec() does not expect results. Use executeStep.");
            }
            else if (checkReturnCode(ret))
            {
                throw SQLite::Exception("Statement needs to be reseted", ret);
            }
        }

        // Return the number of rows modified by those SQL statements (INSERT, UPDATE or DELETE)
        return getChanges();
    }

    int StatementExecutor::tryExecuteStep() noexcept
    {
        if (mbDone)
        {
            return SQLITE_MISUSE; // Statement needs to be reseted !
        }

        const auto ret = mpStatement->step();
        if (SQLITE_ROW == ret) // one row is ready : call getColumn(N) to access it
        {
            mbHasRow = true;
        }
        else
        {
            mbHasRow = false;
            mbDone = SQLITE_DONE == ret; // check if the query has finished executing
        }
        return ret;
    }

    // Get number of rows modified by last INSERT, UPDATE or DELETE statement (not DROP table).
    int StatementExecutor::getChanges() const noexcept
    {
        return sqlite3_changes(mpStatement->mpConnection);
    }

    // Return the numeric result code for the most recent failed API call (if any).
    int StatementExecutor::getErrorCode() const noexcept
    {
        return sqlite3_errcode(mpStatement->mpConnection);
    }

    // Return the extended numeric result code for the most recent failed API call (if any).
    int StatementExecutor::getExtendedErrorCode() const noexcept
    {
        return sqlite3_extended_errcode(mpStatement->mpConnection);
    }

    // Return UTF-8 encoded English language explanation of the most recent failed API call (if any).
    const char* StatementExecutor::getErrorMsg() const noexcept
    {
        return sqlite3_errmsg(mpStatement->mpConnection);
    }

    // Return prepered SQLite statement object or throw
    sqlite3_stmt* StatementExecutor::getPreparedStatement() const
    {
        return mpStatement->getPreparedStatement();
    }

    StatementExecutor::RowIterator StatementExecutor::begin()
    {
        reset();
        tryExecuteStep();
        return StatementExecutor::RowIterator(mpStatement, 0);
    }

    StatementExecutor::RowIterator StatementExecutor::end()
    {
        return StatementExecutor::RowIterator();
    }

    void StatementExecutor::RowIterator::advance() noexcept
    {
        if (mpStatement.expired())
            return;

        auto statement = mpStatement.lock();
        auto ret = statement->step();

        if (SQLITE_ROW != ret)
        {
            mpStatement = TRowWeakPtr{};
            return;
        }
    }

    bool StatementExecutor::RowIterator::operator==(const RowIterator& aIt) const
    {
        auto left = mpStatement.lock();
        auto right = aIt.mpStatement.lock();

        if (!left && !right)
            return true;

        if (left != right)
            return false;

        return mID == aIt.mID;
    }

}  // namespace SQLite
