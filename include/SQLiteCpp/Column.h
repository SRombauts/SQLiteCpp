/**
 * @file    Column.h
 * @ingroup SQLiteCpp
 * @brief   Encapsulation of a Column in a row of the result pointed by the prepared SQLite::Statement.
 *
 * Copyright (c) 2012-2026 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include <SQLiteCpp/SQLiteCppExport.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Exception.h>

#include <string>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>

// Forward declarations to avoid inclusion of <sqlite3.h> in a header
struct sqlite3_stmt;

namespace SQLite
{

/// SQLite result type codes, usable in constant expressions without including sqlite3.h.
inline constexpr int INTEGER = 1;   ///< SQLITE_INTEGER
inline constexpr int FLOAT   = 2;   ///< SQLITE_FLOAT
inline constexpr int TEXT    = 3;   ///< SQLITE_TEXT
inline constexpr int BLOB    = 4;   ///< SQLITE_BLOB
inline constexpr int Null    = 5;   ///< SQLITE_NULL

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
class SQLITECPP_API Column
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

    /**
     * @brief Return the original table name for this result column.
     *
     * @return UTF-8 table name, or nullptr for an expression or an allocation failure.
     * @note Requires SQLITE_ENABLE_COLUMN_METADATA in both SQLite and this wrapper.
     * @warning The pointer belongs to the prepared statement. Copy it before the statement is finalized,
     *          automatically reprepared, or the same metadata is requested in another encoding.
     */
    const char* getTableName() const noexcept;
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
     * @brief Return a pointer to the UTF-8 text value (NULL-terminated string) of the column.
     *
     * @param[in] apDefaultValue Value returned if SQLite returns nullptr (empty string by default).
     * @return Text pointer, or apDefaultValue if SQLite provides no text pointer.
     * @note SQLite can return nullptr for SQL NULL or an out-of-memory conversion error.
     * @warning Type conversions or stepping, resetting, or finalizing the statement may invalidate the
     *          returned pointer. Copy the text if it must be retained.
     */
    const char* getText(const char* apDefaultValue = "") const noexcept;
    /**
     * @brief Return a pointer to the binary blob value of the column.
     *
     * @warning Type conversions or stepping, resetting, or finalizing the statement may invalidate the
     *          returned pointer. Copy the data if it must be retained.
     */
    const void* getBlob() const noexcept;
    /**
     * @brief Return a non-owning byte view of a TEXT or BLOB column.
     *
     * TEXT is returned as UTF-8; BLOB bytes are unchanged.
     * Preserves embedded NUL bytes. SQL NULL and empty values both return an empty view;
     * use isNull() to distinguish them.
     *
     * @return View of the column bytes, without copying them.
     * @warning A type conversion or stepping, resetting, or finalizing the statement
     *          may invalidate the view. Copy it to retain the data.
     */
    std::string_view getStringView() const;
    /**
     * @brief Return a std::string for a TEXT or BLOB column.
     *
     * Note this correctly handles strings that contain null bytes.
     */
    std::string getString() const;

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
     * @brief Return the number of bytes used by the text (or blob) value of the column
     *
     * Return either :
     * - size in bytes (not in characters) of the string returned by getText() without the '\0' terminator
     * - size in bytes of the string representation of the numerical value (integer or double)
     * - size in bytes of the binary blob returned by getBlob()
     * - 0 for a NULL value
     */
    int getBytes() const noexcept;

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
     * @brief Convert to a text pointer, returning nullptr for SQL NULL.
     *
     * Unlike getText() without arguments, this does not replace a null pointer with an empty string.
     * @warning Type conversions or stepping, resetting, or finalizing the statement may invalidate the pointer.
     * @see getText
     */
    operator const char*() const
    {
        return getText(nullptr);
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

    /**
     * @brief Inline cast operator to std::string_view
     *
     * Handles BLOB or TEXT, which may contain null bytes within
     *
     * @see getStringView
     */
    operator std::string_view() const
    {
        return getStringView();
    }

    /**
     * @brief Return the column value as an optional, preserving SQL NULL.
     *
     * Uses the string accessors for std::string and std::string_view, and the existing Column
     * conversion operators for other types. NULL is distinct from an empty string or zero.
     *
     * @tparam T Destination value type, such as int, double, std::string, or std::string_view.
     * @return std::nullopt for SQL NULL; otherwise the converted value.
     * @note std::optional indicates whether the value is NULL; it does not imply ownership.
     * @note Check for NULL before any other type conversions of the same column.
     * @warning If T is a pointer or view, the result borrows SQLite-owned memory and may be
     *          invalidated by column conversions or stepping, resetting, or finalizing the statement.
     *          Use an owning T (e.g. std::string) to retain the value independently.
     * @note A zero-length BLOB may produce an engaged optional<const void*> containing nullptr.
     */
    template<typename T>
    std::optional<T> getOptional() const
    {
        if (isNull())
        {
            return std::nullopt;
        }
        if constexpr (std::is_same_v<T, std::string>)
        {
            return getString();
        }
        else if constexpr (std::is_same_v<T, std::string_view>)
        {
            return getStringView();
        }
        else
        {
            return static_cast<T>(*this);
        }
    }

private:
    Statement::TStatementPtr    mStmtPtr;   ///< Shared Pointer to the prepared SQLite Statement Object
    int                         mIndex;     ///< Index of the column in the row of result, starting at 0
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
SQLITECPP_API std::ostream& operator<<(std::ostream& aStream, const Column& aColumn);

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

}  // namespace SQLite
