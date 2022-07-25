/**
 * @file    StatementExecutor.cpp
 * @ingroup SQLiteCpp
 * @brief   Step executor for SQLite prepared Statement Object
 *
 * Copyright (c) 2015 Shibao HONG (shibaohong@outlook.com)
 * Copyright (c) 2015-2021 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include <SQLiteCpp/StatementExecutor.h>

#include <SQLiteCpp/Exception.h>

#include <sqlite3.h>

namespace SQLite
{


    StatementExecutor::StatementExecutor(sqlite3* apSQLite, const std::string& aQuery)
        : mpSQLite(apSQLite)
    {
        prepareStatement(aQuery);
        createColumnInfo();

        mpRowExecutor.swap(TRowPtr(this, [](const StatementExecutor* const) {
            // empty destructor to make shared_ptr without ownership
            }));
    }

    void SQLite::StatementExecutor::prepareStatement(const std::string& aQuery)
    {
        if (!mpSQLite)
            throw SQLite::Exception("Can't create statement without valid database connection");

        sqlite3_stmt* statement;
        const int ret = sqlite3_prepare_v2(mpSQLite, aQuery.c_str(),
            static_cast<int>(aQuery.size()), &statement, nullptr);

        if (SQLITE_OK != ret)
        {
            throw SQLite::Exception(mpSQLite, ret);
        }
        mpStatement = TStatementPtr(statement, [](sqlite3_stmt* stmt)
            {
                sqlite3_finalize(stmt);
            });
    }

    void SQLite::StatementExecutor::createColumnInfo()
    {
        mColumnCount = sqlite3_column_count(mpStatement.get());


        if (!mColumnNames.empty())
            mColumnNames.clear();

        // Build the map of column name and index
        for (int i = 0; i < mColumnCount; ++i)
        {
            const char* pName = sqlite3_column_name(getPreparedStatement(), i);
            mColumnNames.emplace(pName, i);
        }
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
        return sqlite3_reset(mpStatement.get());
    }

    // Execute a step of the query to fetch one row of results
    bool StatementExecutor::executeStep()
    {
        const int ret = tryExecuteStep();
        if ((SQLITE_ROW != ret) && (SQLITE_DONE != ret)) // on row or no (more) row ready, else it's a problem
        {
            if (ret == sqlite3_errcode(mpSQLite))
            {
                throw SQLite::Exception(mpSQLite, ret);
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
            else if (ret == sqlite3_errcode(mpSQLite))
            {
                throw SQLite::Exception(mpSQLite, ret);
            }
            else
            {
                throw SQLite::Exception("Statement needs to be reseted", ret);
            }
        }

        // Return the number of rows modified by those SQL statements (INSERT, UPDATE or DELETE)
        return sqlite3_changes(mpSQLite);
    }

    int StatementExecutor::tryExecuteStep() noexcept
    {
        if (mbDone)
        {
            return SQLITE_MISUSE; // Statement needs to be reseted !
        }

        const int ret = sqlite3_step(mpStatement.get());
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
        return sqlite3_changes(mpSQLite);
    }

    // Return the numeric result code for the most recent failed API call (if any).
    int StatementExecutor::getErrorCode() const noexcept
    {
        return sqlite3_errcode(mpSQLite);
    }

    // Return the extended numeric result code for the most recent failed API call (if any).
    int StatementExecutor::getExtendedErrorCode() const noexcept
    {
        return sqlite3_extended_errcode(mpSQLite);
    }

    // Return UTF-8 encoded English language explanation of the most recent failed API call (if any).
    const char* StatementExecutor::getErrorMsg() const noexcept
    {
        return sqlite3_errmsg(mpSQLite);
    }

    // Return prepered SQLite statement object or throw
    sqlite3_stmt* StatementExecutor::getPreparedStatement() const
    {
        sqlite3_stmt* ret = mpStatement.get();
        if (ret)
        {
            return ret;
        }
        throw SQLite::Exception("Statement was not prepared.");
    }

    StatementExecutor::RowIterator StatementExecutor::begin()
    {
        reset();
        tryExecuteStep();
        return StatementExecutor::RowIterator(getStatement(), getExecutorWeakPtr(), 0);
    }

    StatementExecutor::RowIterator StatementExecutor::end()
    {
        return StatementExecutor::RowIterator();
    }

    void StatementExecutor::RowIterator::advance() noexcept
    {
        if (mpRow.expired())
            return;

        auto statement = mpRow.lock();
        statement->tryExecuteStep();

        if (statement->isDone())
        {
            mpRow.reset();
            return;
        }
    }

    bool StatementExecutor::RowIterator::operator==(const RowIterator& aIt) const
    {
        auto left = mpRow.lock();
        auto right = aIt.mpRow.lock();

        if (!left && !right)
            return true;

        if (left != right)
            return false;

        return mID == aIt.mID;
    }

}  // namespace SQLite
