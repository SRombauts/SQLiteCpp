/**
 * @file    VariadicBind.h
 * @ingroup SQLiteCpp
 * @brief   Convenience function for Statement::bind(...)
 *
 * Copyright (c) 2016 Paul Dreik (github@pauldreik.se)
 * Copyright (c) 2016-2019 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 * Copyright (c) 2019 Maximilian Bachmann (github@maxbachmann)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#if (__cplusplus >= 201103L) || ( defined(_MSC_VER) && (_MSC_VER >= 1800) ) // c++11: Visual Studio 2013

#include <SQLiteCpp/Statement.h>

/// @cond
#include <utility>
#include <initializer_list>

namespace SQLite
{
/// @endcond

/**
 * \brief Convenience function for calling Statement::bind(...) once for each argument given.
 *
 * This takes care of incrementing the index between each calls to bind.
 *
 * This feature requires a c++11 capable compiler.
 *
 * \code{.cpp}
 * SQLite::Statement stm("SELECT * FROM MyTable WHERE colA>? && colB=? && colC<?");
 * bind(stm,a,b,c);
 * //...is equivalent to
 * stm.bind(1,a);
 * stm.bind(2,b);
 * stm.bind(3,c);
 * \endcode
 * @param s statement
 * @param args one or more args to bind.
 */
template<class ...Args>
void bind(SQLite::Statement& s, const Args& ... args)
{
    int pos = 0;
    (void)std::initializer_list<int>{
      ((void)s.bind(++pos, std::forward<decltype(args)>(args)), 0)...
    };
}

}  // namespace SQLite

#endif // c++11

