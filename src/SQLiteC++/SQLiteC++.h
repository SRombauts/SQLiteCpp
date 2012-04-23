/**
 * @file  SQLiteC++.h
 * @brief SQLiteC++ is a smart and simple C++ SQLite3 wrapper. This file is only "easy include" for other files.
 *
 * Copyright (c) 2012 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once


// SQLiteC++.h requires sqlite3, and the corresponding library header
#include "sqlite3.h"

// Include useful headers of SQLiteC++
#include "Database.h"
#include "Statement.h"
#include "Column.h"
#include "Transaction.h"


/**
 * @brief Version numbers for SQLiteC++ are provided in the same way as sqlite3.h
 *
 * The [SQLITECPP_VERSION] C preprocessor macro in the SQLiteC++.h header
 * evaluates to a string literal that is the SQLite version in the
 * format "X.Y.Z" where X is the major version number
 * and Y is the minor version number and Z is the release number.
 *
 * The [SQLITECPP_VERSION_NUMBER] C preprocessor macro resolves to an integer
 * with the value (X*1000000 + Y*1000 + Z) where X, Y, and Z are the same
 * numbers used in [SQLITECPP_VERSION].
 */
#define SQLITECPP_VERSION           "0.4.0"
#define SQLITECPP_VERSION_NUMBER    0004000
