/**
 * @file    StatementPtr.h
 * @ingroup SQLiteCpp
 * @brief   Pointer for prepared SQLite Statement Object
 *
 * Copyright (c) 2022 Kacper Zielinski (KacperZ155@gmail.com)
 * Copyright (c) 2022 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include <memory>
#include <string>
#include <map>

// Forward declaration to avoid inclusion of <sqlite3.h> in a header
struct sqlite3;
struct sqlite3_stmt;

namespace SQLite
{


/**
* @brief Container for SQLite Statement pointer.
*
* You should never create this object unless you are expanding SQLiteCPP API.
*/
struct StatementPtr
{
    /**
    * @brief Don't create this object unless you are expanding SQLiteCPP API.
    * 
    * @param[in] apSQLite  the SQLite Database Connection
    * @param[in] aQuery    an UTF-8 encoded query string
    *
    * @throws Exception is thrown in case of error, then the StatementPtr object is NOT constructed.
    */
    StatementPtr(sqlite3* apSQLite, const std::string& aQuery);

    /// Shared pointer to SQLite prepared Statement Object
    using TRawStatementPtr = std::shared_ptr<sqlite3_stmt>;

    /// Type to store columns names and indexes
    using TColumnsMap = std::map<std::string, int_fast16_t, std::less<>>;

    sqlite3* const          mpConnection;       //!< Pointer to SQLite Database Connection Handle
    TRawStatementPtr const  mpStatement;        //!< Shared Pointer to the prepared SQLite Statement Object
    std::size_t             mCurrentStep = 0;   //!< Current step of prepared Statement Object
    int_fast16_t            mColumnCount;       //!< Number of columns in the result of the prepared statement

    /// Map of columns index by name (mutable so getColumnIndex can be const)
    mutable TColumnsMap mColumnNames{};

    /// Resets SQLite Statement Object
    int reset() noexcept;

    /// Execute next step of SQLite Statement Object
    int step() noexcept;

    /**
    * @brief Returns pointer to prepared SQLite Statement Object.
    * Use this ONLY on sqlite3 function!
    * 
    * @return Pointer to SQLite Statement Object
    */
    sqlite3_stmt* getStatement() const noexcept;

private:
    /// Create prepared SQLite Statement Object
    TRawStatementPtr prepareStatement(sqlite3* apConnection, const std::string& aQuery) const;
};


/// Shared pointer to SQLite StatementPtr
using TStatementPtr = std::shared_ptr<StatementPtr>;

/// Weak pointer to SQLite StatementPtr
using TStatementWeakPtr = std::weak_ptr<StatementPtr>;


}  // namespace SQLite
