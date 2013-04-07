/**
 * @file    Exception.h
 * @ingroup SQLiteCpp
 * @brief   Encapsulation of the error message from SQLite3 on a std::runtime_error.
 *
 * Copyright (c) 2012-2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include <stdexcept>
#include <cassert>


// assert() is used in destructors, where exceptions are not allowed
// here you can chose if you want to use them or not
#ifndef NDEBUG
    // in debug mode :
    #define SQLITE_CPP_ASSERT(expression) assert(expression)
#else
    // in release mode :
    #define SQLITE_CPP_ASSERT(expression) (expression)
#endif


#ifdef _WIN32
#pragma warning(disable:4290) // Disable warning C4290: C++ exception specification ignored except to indicate a function is not __declspec(nothrow)
#endif


namespace SQLite
{


/**
 * @brief Encapsulation of the error message from SQLite3, based on std::runtime_error.
 */
class Exception : public std::runtime_error
{
public:
    /**
     * @brief Encapsulation of the error message from SQLite3, based on std::runtime_error.
     *
     * @param[in] aErrorMessage The string message describing the SQLite error
     */
    Exception(const std::string& aErrorMessage) :
        std::runtime_error(aErrorMessage)
    {
    }
};


}  // namespace SQLite
