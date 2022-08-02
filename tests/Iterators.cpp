/**
 * @file    Iterators.cpp
 * @ingroup tests
 * @brief   Test of Statement iterators
 *
 * Copyright (c) 2022 Kacper Zielinski (KacperZ155@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>

#include <gtest/gtest.h>

#include <vector>
#include <string>
#include <algorithm>

SQLite::Database createDatabase()
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    EXPECT_EQ(SQLite::OK, db.exec("CREATE TABLE test (number INTEGER, number_str TEXT)"));

    SQLite::Statement inserter(db, "INSERT INTO test VALUES(?,?)");
    for (int i = 10; i > 0; --i)
    {
        inserter.bind(1, i);
        inserter.bind(2, std::to_string(i));
        EXPECT_EQ(1, inserter.exec());
        EXPECT_NO_THROW(inserter.reset());
    }
    return db;
}

TEST(Iterators, RowIterator)
{
    auto db = createDatabase();

    SQLite::Statement query(db, "SELECT * FROM test");
    std::vector<int> numbers;
    
    for (const auto& row : query)
    {
        numbers.push_back(row[0]);
    }

    //TODO: EXPECT_TRUE(query.isDone());

    const auto v_size = static_cast<int>(numbers.size());
    for (int i = v_size; i > 0; --i)
    {
        EXPECT_EQ(i, numbers[v_size - i]);
    }

    auto it = query.begin();
    ++it;
    EXPECT_STREQ("number_str", it->getColumn(1).getName());
    EXPECT_EQ(1, it->getColumnIndex("number_str"));

    // getColumn aliases
    EXPECT_EQ(9, it->at(0).getInt());
    EXPECT_EQ(9, it->getColumn(0).getInt());
    EXPECT_EQ(9, it->operator[](0).getInt());

    auto it2 = query.begin();
    ++it2;
    EXPECT_EQ(it, it2);

    // RowInterator is advancing common statement object
    ++it;
    EXPECT_EQ(it->at(0).getInt(), it2->at(0).getInt());
    // But iterators internal state is diffrent.
    EXPECT_NE(it, it2);
}

TEST(Iterators, RowIterator_STL_Algorithms)
{
    auto db = createDatabase();

    SQLite::Statement query(db, "SELECT * FROM test");
    
    for (auto it = query.begin(); it != query.end(); std::advance(it, 3))
    {
        EXPECT_TRUE(it->getRowNumber() % 3 == 0);
    }

    EXPECT_TRUE(std::all_of(query.begin(), query.end(), [](const SQLite::Row& row)
        {
            return row[0].getInt() > 0;
        }));
}

TEST(Iterators, ColumnIterator)
{
    auto db = createDatabase();

    SQLite::Statement query_only1(db, "SELECT * FROM test LIMIT 1");
    std::vector<std::string> numbers_str;

    for(const auto& row : query_only1)
        for (const auto& column : row)
        {
            numbers_str.emplace_back(column.getText());
        }

    for (const auto& column : numbers_str)
    {
        EXPECT_EQ("10", column);
    }
}

TEST(Iterators, ColumnIterator_STL_Algorithms)
{
    auto db = createDatabase();

    SQLite::Statement query_only1(db, "SELECT * FROM test LIMIT 1");
    std::vector<int> numbers;

    for (const auto& row : query_only1)
    {
        EXPECT_EQ(2, std::count_if(row.begin(), row.end(), [i = 10](const SQLite::Column& column)
            {
                return column.getText() == std::to_string(i);
            }));
    }
}
