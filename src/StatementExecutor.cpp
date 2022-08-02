/**
 * @file    StatementExecutor.cpp
 * @ingroup SQLiteCpp
 * @brief   Step executor for SQLite prepared Statement Object
 *
 * Copyright (c) 2022 Kacper Zielinski (KacperZ155@gmail.com)
 * Copyright (c) 2012-2021 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include <iterator>
#include <SQLiteCpp/StatementExecutor.h>

#include <SQLiteCpp/Exception.h>

#include <sqlite3.h>

namespace SQLite
{


    // Reset the statement to make it ready for a new execution. This doesn't clear bindings.
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
            if (ret == getErrorCode())
            {
                throw SQLite::Exception(mpStatement->mpConnection, ret);
            }
            else
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
            else if (ret == getErrorCode())
            {
                throw SQLite::Exception(mpStatement->mpConnection, ret);
            }
            else
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

    ////////////////////////////////////////////////////////////////////////////

    StatementExecutor::RowIterator StatementExecutor::begin()
    {
        reset();
        tryExecuteStep();
        return StatementExecutor::RowIterator(mpStatement);
    }

    StatementExecutor::RowIterator StatementExecutor::end() noexcept
    {
        return StatementExecutor::RowIterator();
    }

    void StatementExecutor::RowIterator::advance() noexcept
    {
        mRow = Row(mpStatement, ++mID);

        if (mpStatement.expired())
            return;

        auto statement = mpStatement.lock();
        auto ret = statement->step();

        if (SQLITE_ROW != ret)
        {
            mpStatement = TStatementWeakPtr{};
            return;
        }
    }

    bool StatementExecutor::RowIterator::operator==(const RowIterator& aIt) const noexcept
    {
        const auto left = mpStatement.lock();
        const auto right = aIt.mpStatement.lock();

        if (!left && !right)
            return true;

        if (left != right)
            return false;

        return mID == aIt.mID;
    }


}  // namespace SQLite
