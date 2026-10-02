#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/Backup.h>
#include <sqlite3.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    const std::filesystem::path workDir(argv[1]);
    std::filesystem::create_directories(workDir);
    const int flags = SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE;
    {
        const auto missingPath = (workDir / "missing.db").string();
        std::filesystem::remove(missingPath);
        SQLite::Database db(":memory:", flags);
        db.exec("CREATE TABLE valuable (x); INSERT INTO valuable VALUES (42)");
        db.backup(missingPath.c_str(), SQLite::Database::Load);
        std::cout << "missing_load created=" << std::filesystem::exists(missingPath)
                  << " tables=" << db.execAndGet("SELECT count(*) FROM sqlite_master WHERE type='table'").getInt()
                  << '\n';
    }
    {
        const auto missingPath = (workDir / "missing-file-source.db").string();
        const auto destinationPath = (workDir / "valuable.db").string();
        std::filesystem::remove(missingPath);
        std::filesystem::remove(destinationPath);
        {
            SQLite::Database db(destinationPath.c_str(), flags);
            db.exec("CREATE TABLE valuable (x); INSERT INTO valuable VALUES (42)");
            db.backup(missingPath.c_str(), SQLite::Database::Load);
        }
        SQLite::Database reopened(destinationPath.c_str(), SQLite::OPEN_READONLY);
        std::cout << "missing_load persisted_tables="
                  << reopened.execAndGet("SELECT count(*) FROM sqlite_master WHERE type='table'").getInt()
                  << '\n';
    }
    {
        const auto backupPath = (workDir / "backup.db").string();
        std::filesystem::remove(backupPath);
        SQLite::Database source(":memory:", flags);
        source.exec("CREATE TABLE valuable (x); INSERT INTO valuable VALUES (42)");
        source.exec("BEGIN; INSERT INTO valuable VALUES (43)");
        {
            SQLite::Database destination(":memory:", flags);
            SQLite::Backup backup(destination, source);
            std::cout << "backup raw_status=" << backup.executeStep() << '\n';
        }
        source.backup(backupPath.c_str(), SQLite::Database::Save);
        SQLite::Database destination(backupPath.c_str(), SQLite::OPEN_READONLY);
        std::cout << "backup returned tables="
                  << destination.execAndGet("SELECT count(*) FROM sqlite_master WHERE type='table'").getInt()
                  << '\n';
        const auto existingPath = (workDir / "existing-backup.db").string();
        std::filesystem::remove(existingPath);
        {
            SQLite::Database existing(existingPath.c_str(), flags);
            existing.exec("CREATE TABLE old_data (x); INSERT INTO old_data VALUES (123)");
        }
        source.backup(existingPath.c_str(), SQLite::Database::Save);
        SQLite::Database existing(existingPath.c_str(), SQLite::OPEN_READONLY);
        std::cout << "backup existing_old_row=" << existing.execAndGet("SELECT x FROM old_data").getInt()
                  << " new_table=" << existing.tableExists("valuable") << '\n';
        source.exec("ROLLBACK");
    }
    {
        const auto sourcePath = (workDir / "locked-source.db").string();
        std::filesystem::remove(sourcePath);
        SQLite::Database source(sourcePath.c_str(), flags);
        source.exec("CREATE TABLE new_data (x); INSERT INTO new_data VALUES (77); BEGIN EXCLUSIVE");
        SQLite::Database db(":memory:", flags);
        db.exec("CREATE TABLE old_data (x); INSERT INTO old_data VALUES (123)");
        db.backup(sourcePath.c_str(), SQLite::Database::Load);
        std::cout << "locked_load old_row=" << db.execAndGet("SELECT x FROM old_data").getInt()
                  << " new_table=" << db.tableExists("new_data") << '\n';
        source.exec("ROLLBACK");
    }
    {
        SQLite::Database db(":memory:", flags);
        db.exec("CREATE TABLE valuable (x)");
        auto first = std::make_unique<SQLite::Transaction>(db);
        first->rollback();
        SQLite::Transaction second(db);
        db.exec("INSERT INTO valuable VALUES (42)");
        first.reset();
        std::cout << "replacement_transaction rows="
                  << db.execAndGet("SELECT count(*) FROM valuable").getInt() << '\n';
        try
        {
            second.commit();
            std::cout << "replacement_transaction commit=success\n";
        }
        catch (const SQLite::Exception& error)
        {
            std::cout << "replacement_transaction commit=" << error.what() << '\n';
        }
    }
    {
        SQLite::Database db(":memory:", flags);
        db.exec("CREATE TABLE valuable (x)");
        SQLite::Transaction first(db);
        first.rollback();
        SQLite::Transaction second(db);
        db.exec("INSERT INTO valuable VALUES (42)");
        first.commit();
        std::cout << "old_wrapper_commit autocommit=" << sqlite3_get_autocommit(db.getHandle())
                  << " rows=" << db.execAndGet("SELECT count(*) FROM valuable").getInt() << '\n';
    }
    {
        SQLite::Database db(":memory:", flags);
        db.exec("CREATE TABLE t (id INTEGER PRIMARY KEY); INSERT INTO t VALUES (1)");
        auto first = std::make_unique<SQLite::Transaction>(db);
        try
        {
            db.exec("INSERT OR ROLLBACK INTO t VALUES (1)");
        }
        catch (const SQLite::Exception&)
        {
        }
        SQLite::Transaction second(db);
        db.exec("INSERT INTO t VALUES (2)");
        first.reset();
        std::cout << "automatic_rollback successor_rows="
                  << db.execAndGet("SELECT count(*) FROM t").getInt() << '\n';
    }
    {
        SQLite::Database db(":memory:", flags);
        db.exec("PRAGMA foreign_keys=ON; CREATE TABLE parent (id INTEGER PRIMARY KEY);"
                "CREATE TABLE child (id INTEGER REFERENCES parent(id) DEFERRABLE INITIALLY DEFERRED)");
        SQLite::Transaction transaction(db);
        db.exec("INSERT INTO child VALUES (1)");
        try
        {
            transaction.commit();
        }
        catch (const SQLite::Exception&)
        {
        }
        std::cout << "failed_commit still_active=" << !sqlite3_get_autocommit(db.getHandle()) << '\n';
        db.exec("INSERT INTO parent VALUES (1)");
        transaction.commit();
        std::cout << "failed_commit retry_rows=" << db.execAndGet("SELECT count(*) FROM child").getInt() << '\n';
    }
    {
        SQLite::Database db(":memory:", flags);
        db.exec("CREATE TABLE t (a); INSERT INTO t VALUES (7)");
        SQLite::Statement query(db, "SELECT * FROM t");
        query.executeStep();
        query.getColumn("a");
        query.reset();
        db.exec("ALTER TABLE t ADD COLUMN b INTEGER DEFAULT 9");
        query.executeStep();
        std::cout << "metadata cached=" << query.getColumnCount()
                  << " actual=" << sqlite3_column_count(sqlite3_next_stmt(db.getHandle(), nullptr)) << '\n';
        try
        {
            query.getColumn(1);
        }
        catch (const SQLite::Exception& error)
        {
            std::cout << "metadata added_column=" << error.what() << '\n';
        }
    }
    {
        SQLite::Database db(":memory:", flags);
        db.exec("CREATE TABLE t (a, b); INSERT INTO t VALUES (7, 9)");
        SQLite::Statement query(db, "SELECT * FROM t");
        query.executeStep();
        query.getColumn("a");
        query.reset();
        db.exec("DROP TABLE t; CREATE TABLE t (b, a); INSERT INTO t VALUES (9, 7)");
        query.executeStep();
        std::cout << "metadata name_a_value=" << query.getColumn("a").getInt()
                  << " actual_first_name=" << query.getColumnName(0) << '\n';
    }
    {
        const auto headerPath = (workDir / "header.db").string();
        std::filesystem::remove(headerPath);
        {
            SQLite::Database db(headerPath.c_str(), flags);
            db.exec("PRAGMA page_size=65536; CREATE TABLE t (x)");
        }
        std::cout << "header page_size=" << SQLite::Database::getHeaderInfo(headerPath).pageSizeBytes << '\n';
        {
            std::fstream stream(headerPath, std::ios::in | std::ios::out | std::ios::binary);
            stream.seekp(15);
            stream.put('X');
        }
        SQLite::Database::getHeaderInfo(headerPath);
        std::cout << "header bad_magic_accepted=1\n";
    }
}
