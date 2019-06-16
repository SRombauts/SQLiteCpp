/**
 * @file    ExecuteMany.h
 * @ingroup SQLiteCpp
 * @brief   Convenience function to execute a Statement with multiple Parameter sets
 *
 * Copyright (c) 2019 Maximilian Bachmann (github maxbachmann)
 * Copyright (c) 2019 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#if (__cplusplus >= 201402L) || ( defined(_MSC_VER) && (_MSC_VER >= 1900) ) // c++14: Visual Studio 2015

#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/VariadicBind.h>

/// @cond
#include <tuple>
#include <utility>
#include <initializer_list>

namespace SQLite
{
/// @endcond

/**
 * \brief Convenience function to execute a Statement with multiple Parameter sets once for each parameter set given.
 *
 *
 * This feature requires a c++14 capable compiler.
 *
 * \code{.cpp}
 * execute_many(db, "INSERT INTO test VALUES (?, ?)",
 *   std::make_tuple(1, "one"),
 *   std::make_tuple(2, "two"),
 *   std::make_tuple(3, "three")
 * );
 * \endcode
 * @param aDatabase Database to use
 * @param apQuery   Query to use with all parameter sets
 * @param aArg      first tuple with parameters
 * @param aParams   the following tuples with parameters
 */
template <typename Arg, typename... Types>
void execute_many(Database& aDatabase, const char* apQuery, Arg&& aArg, Types&&... aParams)
{
    SQLite::Statement query(aDatabase, apQuery);
    bind_exec(query, std::forward<decltype(aArg)>(aArg));
    (void)std::initializer_list<int>
    {
        ((void)reset_bind_exec(query, std::forward<decltype(aParams)>(aParams)), 0)...
    };
}

/**
 * \brief Convenience function to reset a statement and call bind_exec to 
 * bind new values to the statement and execute it
 *
 * This feature requires a c++14 capable compiler.
 *
 * @param apQuery   Query to use
 * @param aTuple    Tuple to bind
 */
template <typename ... Types>
void reset_bind_exec(SQLite::Statement& apQuery, std::tuple<Types...>&& aTuple)
{
    apQuery.reset();
    bind_exec(apQuery, std::forward<decltype(aTuple)>(aTuple));
}

/**
 * \brief Convenience function to bind values a the statement and execute it
 *
 * This feature requires a c++14 capable compiler.
 *
 * @param apQuery   Query to use
 * @param aTuple    Tuple to bind
 */
template <typename ... Types>
void bind_exec(SQLite::Statement& apQuery, std::tuple<Types...>&& aTuple)
{
    bind(apQuery, std::forward<decltype(aTuple)>(aTuple));
    while (apQuery.executeStep()) {}
}

}  // namespace SQLite

#endif // c++14
