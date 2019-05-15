/**
 * @file    VariadicBind_test.cpp
 * @ingroup tests
 * @brief   Test of variadic bind
 *
 * Copyright (c) 2016 Paul Dreik (github@pauldreik.se)
 * Copyright (c) 2016-2019 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 * Copyright (c) 2019 Maximilian Bachmann (github@maxbachmann)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/VariadicBind.h>

#include <gtest/gtest.h>

#include <cstdio>

#if (__cplusplus >= 201103L) || ( defined(_MSC_VER) && (_MSC_VER >= 1800) ) // c++11: Visual Studio 2013
TEST(VariadicBind, invalid) {
    // Create a new database
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);

    EXPECT_EQ(0, db.exec("DROP TABLE IF EXISTS test"));
    EXPECT_EQ(0,
            db.exec(
                    "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT DEFAULT 'default') "));
    EXPECT_TRUE(db.tableExists("test"));

    {
        SQLite::Statement query(db, "INSERT INTO test VALUES (?, ?)");

        // zero arguments - should give compile time error through a static assert
        // SQLite::bind(query);

        // bind one argument less than expected - should be fine.
        // the unspecified argument should be set to null, not the default.
        SQLite::bind(query, 1);
        EXPECT_EQ(1, query.exec());
        query.reset();

        // bind all arguments - should work just fine
        SQLite::bind(query, 2, "two");
        EXPECT_EQ(1, query.exec());
        query.reset();

        // bind too many arguments - should throw.
        EXPECT_THROW(SQLite::bind(query, 3, "three", 0), SQLite::Exception);
        EXPECT_EQ(1, query.exec());
    }

    // make sure the content is as expected
    {
        SQLite::Statement query(db, std::string{"SELECT id, value FROM test ORDER BY id"});
        std::vector<std::pair<int, std::string> > results;
        while (query.executeStep()) {
            const int id = query.getColumn(0);
            std::string value = query.getColumn(1);
            results.emplace_back( id, std::move(value) );
        }
        EXPECT_EQ(std::size_t(3), results.size());

        EXPECT_EQ(std::make_pair(1,std::string{""}), results.at(0));
        EXPECT_EQ(std::make_pair(2,std::string{"two"}), results.at(1));
        EXPECT_EQ(std::make_pair(3,std::string{"three"}), results.at(2));
    }
}
#endif // c++11
