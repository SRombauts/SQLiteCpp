/**
 * @file    Column.h
 * @ingroup SQLiteCpp
 * @brief   Encapsulation of a Column in a row of the result pointed by the prepared SQLite::Statement.
 *
 * Copyright (c) 2012-2022 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Exception.h>

#include <string>
#include <memory>

// Forward declarations to avoid inclusion of <sqlite3.h> in a header
struct sqlite3_stmt;

namespace SQLite
{

extern const int INTEGER;   ///< SQLITE_INTEGER
extern const int FLOAT;     ///< SQLITE_FLOAT
extern const int TEXT;      ///< SQLITE_TEXT
extern const int BLOB;      ///< SQLITE_BLOB
extern const int Null;      ///< SQLITE_NULL

/**
 * @brief Encapsulation of a Column in a row of the result pointed by the prepared Statement.
 *
 * A Column is a particular field of SQLite data in the current row of result
 * of the Statement : it points to a single cell.
 *
 * Its value can be expressed as a text, and, when applicable, as a numeric
 * (integer or floating point) or a binary blob.
 *
 * Thread-safety: a Column object shall not be shared by multiple threads, because :
 * 1) in the SQLite "Thread Safe" mode, "SQLite can be safely used by multiple threads
 *    provided that no single database connection is used simultaneously in two or more threads."
 * 2) the SQLite "Serialized" mode is not supported by SQLiteC++,
 *    because of the way it shares the underling SQLite precompiled statement
 *    in a custom shared pointer (See the inner class "Statement::Ptr").
 */
class Column
{
public:
    /**
     * @brief Encapsulation of a Column in a Row of the result.
     *
     * @param[in] aStmtPtr  Shared pointer to the prepared SQLite Statement Object.
     * @param[in] aIndex    Index of the column in the row of result, starting at 0
     */
    explicit Column(const Statement::TStatementPtr& aStmtPtr, int aIndex);

    /**
     * @brief Return a pointer to the named assigned to this result column (potentially aliased)
     *
     * @see getOriginName() to get original column name (not aliased)
     */
    const char* getName() const noexcept;

#ifdef SQLITE_ENABLE_COLUMN_METADATA
    /**
     * @brief Return a pointer to the table column name that is the origin of this result column
     *
     *  Require definition of the SQLITE_ENABLE_COLUMN_METADATA preprocessor macro :
     * - when building the SQLite library itself (which is the case for the Debian libsqlite3 binary for instance),
     * - and also when compiling this wrapper.
     */
    const char* getOriginName() const noexcept;
#endif

    /// Return the integer value of the column.
    int32_t     getInt() const noexcept;
    /// Return the 32bits unsigned integer value of the column (note that SQLite3 does not support unsigned 64bits).
    uint32_t    getUInt() const noexcept;
    /// Return the 64bits integer value of the column (note that SQLite3 does not support unsigned 64bits).
    int64_t     getInt64() const noexcept;
    /// Return the double (64bits float) value of the column
    double      getDouble() const noexcept;
    /**
     * @brief Return a pointer to the text value (NULL terminated string) of the column.
     *
     * @warning The value pointed at is only valid while the statement is valid (ie. not finalized),
     *          thus you must copy it before using it beyond its scope (to a std::string for instance).
     */
    const char* getText(const char* apDefaultValue = "") const noexcept;
    /**
     * @brief Return a pointer to the binary blob value of the column.
     *
     * @warning The value pointed at is only valid while the statement is valid (ie. not finalized),
     *          thus you must copy it before using it beyond its scope (to a std::string for instance).
     */
    const void* getBlob() const noexcept;
    /**
     * @brief Return a std::string for a TEXT or BLOB column.
     *
     * Note this correctly handles strings that contain null bytes.
     */
    std::string getString() const;

#ifdef __cpp_unicode_characters
    /**
     * @brief Return a pointer to the text value (NULL terminated UTF-16 string) of the column.
     *
     * @warning The value pointed at is only valid while the statement is valid (ie. not finalized),
     *          thus you must copy it before using it beyond its scope (to a std::u16string for instance).
     */
    const char16_t* getU16Text(const char16_t* apDefaultValue = u"") const noexcept;
    /**
     * @brief Return a std::u16string for a TEXT column.
     *
     * Note this correctly handles strings that contain null bytes.
     */
    std::u16string getU16String() const;
#if WCHAR_MAX == 0xffff
    /**
     * @brief Return a pointer to the text value (NULL terminated UTF-16 string) of the column.
     *
     * @warning The value pointed at is only valid while the statement is valid (ie. not finalized),
     *          thus you must copy it before using it beyond its scope (to a std::wstring for instance).
     */
    const wchar_t* getWText(const wchar_t* apDefaultValue = L"") const noexcept;
    /**
     * @brief Return a std::wstring for a TEXT column.
     */
    std::wstring getWString() const;
#endif  // WCHAR_MAX == 0xffff
#endif  // __cpp_unicode_characters
#ifdef __cpp_char8_t
    /**
     * @brief Return a pointer to the text value (NULL terminated UTF-8 string) of the column.
     *
     * @warning The value pointed at is only valid while the statement is valid (ie. not finalized),
     *          thus you must copy it before using it beyond its scope (to a std::u8string for instance).
     */
    const char8_t* getU8Text(const char8_t* apDefaultValue = u8"") const noexcept;
    /**
     * @brief Return a std::u8string for a TEXT or BLOB column.
     *
     * Note this correctly handles strings that contain null bytes.
     */
    std::u8string getU8String() const;
#endif  // __cpp_char8_t
    /**
     * @brief Return the type of the value of the column using sqlite3_column_type()
     *
     * Return either SQLite::INTEGER, SQLite::FLOAT, SQLite::TEXT, SQLite::BLOB, or SQLite::Null.
     * This type may change from one row to the next, since
     * SQLite stores data types dynamically for each value and not per column.
     * Use Statement::getColumnDeclaredType() to retrieve the declared column type from a SELECT statement.
     *
     * @warning After a type conversion (by a call to a getXxx on a Column of a Yyy type),
     *          the value returned by sqlite3_column_type() is undefined.
     */
    int getType() const noexcept;

    /// Test if the column is an integer type value (meaningful only before any conversion)
    bool isInteger() const noexcept
    {
        return (SQLite::INTEGER == getType());
    }
    /// Test if the column is a floating point type value (meaningful only before any conversion)
    bool isFloat() const noexcept
    {
        return (SQLite::FLOAT == getType());
    }
    /// Test if the column is a text type value (meaningful only before any conversion)
    bool isText() const noexcept
    {
        return (SQLite::TEXT == getType());
    }
    /// Test if the column is a binary blob type value (meaningful only before any conversion)
    bool isBlob() const noexcept
    {
        return (SQLite::BLOB == getType());
    }
    /// Test if the column is NULL (meaningful only before any conversion)
    bool isNull() const noexcept
    {
        return (SQLite::Null == getType());
    }

    /**
     * @brief Return the number of bytes used by the UTF-8 text (or blob) value of the column
     *
     *  Can cause conversion to text and between UTF-8/UTF-16 encodings
     *  Be careful when using with getBytes16() and UTF-16 functions
     *
     * Return either :
     * - size in bytes (not in characters) of the string returned by getText() without the '\0' terminator
     * - size in bytes of the string representation of the numerical value (integer or double)
     * - size in bytes of the binary blob returned by getBlob()
     * - 0 for a NULL value
     */
    int getBytes() const noexcept;

    /**
     * @brief Return the number of bytes used by the UTF-16 text value of the column
     *
     *  Can cause conversion to text and between UTF-8/UTF-16 encodings
     *  Be careful when using with getBytes() and UTF-8 functions
     *
     * Return either :
     * - size in bytes (not in characters) of the string returned by getText() without the '\0' terminator
     * - size in bytes of the string representation of the numerical value (integer or double)
     * - 0 for a NULL value
     */
    int getBytes16() const noexcept;

    /// Alias returning the number of bytes used by the text (or blob) value of the column
    int size() const noexcept
    {
        return getBytes ();
    }

    /// Inline cast operators to basic types
    operator char() const
    {
        return static_cast<char>(getInt());
    }
    operator int8_t() const
    {
        return static_cast<int8_t>(getInt());
    }
    operator uint8_t() const
    {
        return static_cast<uint8_t>(getInt());
    }
    operator int16_t() const
    {
        return static_cast<int16_t>(getInt());
    }
    operator uint16_t() const
    {
        return static_cast<uint16_t>(getInt());
    }
    operator int32_t() const
    {
        return getInt();
    }
    operator uint32_t() const
    {
        return getUInt();
    }
    operator int64_t() const
    {
        return getInt64();
    }
    operator double() const
    {
        return getDouble();
    }
    /**
     * @brief Inline cast operator to char*
     *
     * @see getText
     */
    operator const char*() const
    {
        return getText();
    }
    /**
     * @brief Inline cast operator to void*
     *
     * @see getBlob
     */
    operator const void*() const
    {
        return getBlob();
    }

    /**
     * @brief Inline cast operator to std::string
     *
     * Handles BLOB or TEXT, which may contain null bytes within
     *
     * @see getString
     */
    operator std::string() const
    {
        return getString();
    }

#ifdef __cpp_unicode_characters
    /**
     * @brief Inline cast operator to char16_t*
     *
     * @see getU16String
     */
    operator const char16_t* () const
    {
        return getU16Text();
    }
    /**
     * @brief Inline cast operator to std::u16string
     *
     * Handles UTF-16 TEXT
     *
     * @see getU16String
     */
    operator std::u16string() const
    {
        return getU16String();
    }
#if WCHAR_MAX == 0xffff
    /**
     * @brief Inline cast operator to wchar_t*
     *
     * @see getWText
     */
    operator const wchar_t* () const
    {
        return getWText();
    }
    /**
     * @brief Inline cast operator to std::wstring
     *
     * Handles UTF-16 TEXT
     *
     * @see getWString
     */
    operator std::wstring() const
    {
        return getWString();
    }
#endif  // WCHAR_MAX == 0xffff
#endif  // __cpp_unicode_characters
#ifdef __cpp_char8_t
    /**
     * @brief Inline cast operator to char8_t*
     *
     * @see getU8Text
     */
    operator const char8_t* () const
    {
        return getU8Text();
    }
    /**
     * @brief Inline cast operator to std::u8string
     *
     * Handles BLOB or TEXT, which may contain null bytes within
     *
     * @see getU8String
     */
    operator std::u8string() const
    {
        return getU8String();
    }
#endif  // __cpp_char8_t

private:
    Statement::TStatementPtr    mStmtPtr;  ///< Shared Pointer to the prepared SQLite Statement Object
    int                         mIndex;    ///< Index of the column in the row of result, starting at 0
};

/**
 * @brief Standard std::ostream text inserter
 *
 * Insert the text value of the Column object, using getText(), into the provided stream.
 *
 * @param[in] aStream   Stream to use
 * @param[in] aColumn   Column object to insert into the provided stream
 *
 * @return  Reference to the stream used
 */
std::ostream& operator<<(std::ostream& aStream, const Column& aColumn);

#if __cplusplus >= 201402L || (defined(_MSC_VER) && _MSC_VER >= 1900) // c++14: Visual Studio 2015

// Create an instance of T from the first N columns, see declaration in Statement.h for full details
template<typename T, int N>
T Statement::getColumns()
{
    checkRow();
    checkIndex(N - 1);
    return getColumns<T>(std::make_integer_sequence<int, N>{});
}

// Helper function called by getColums<typename T, int N>
template<typename T, const int... Is>
T Statement::getColumns(const std::integer_sequence<int, Is...>)
{
    return T{Column(mpPreparedStatement, Is)...};
}

#endif

}  // namespace SQLite
