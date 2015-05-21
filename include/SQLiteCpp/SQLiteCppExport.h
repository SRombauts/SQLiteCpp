/**
 * @file    SQLiteCpp.h
 * @ingroup SQLiteCpp
 * @brief   SQLiteC++ is a smart and simple C++ SQLite3 wrapper. This file is only "easy include" for other files.
 *
 * Copyright (c) 2012-2015 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once
//
#ifdef SQLITECPP_DYNAMIC
#	if defined(WIN32)
#		ifdef SQLITECPP_EXPORT
#			define SQLITECPP_DLL __declspec(dllexport)
#		else
#			define SQLITECPP_DLL __declspec(dllimport)
#		endif
#	else
#		define SQLITECPP_DLL	
#	endif
#else
#	define SQLITECPP_DLL	
#endif
// Disable VisualStudio warnings about DLL exports
#if defined(_MSC_VER) && defined(SQLITECPP_DYNAMIC) && defined(SQLITECPP_DISABLE_MSVC_WARNINGS)
    #pragma warning( disable : 4251 )
    #pragma warning( disable : 4275 )
#endif
