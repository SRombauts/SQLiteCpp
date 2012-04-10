http://www.sqlite.org/cvstrac/wiki?p=SqliteWrappers
http://stackoverflow.com/questions/120295/what-is-a-good-oo-c-wrapper-for-sqlite
http://stackoverflow.com/questions/818155/sqlite-alternatives-for-c
http://www.reddit.com/search?q=sqlite


**sqlite3cc**: http://ed.am/dev/sqlite3cc/
- Nov 2009, Jan 2012 (v0.1)
- (++) modern design, use RAII => can be a source of inspiration for me
- (++) very well documented, in code and with a very good informal presentation
- (+) is maintained (recent), initial release is 0.1.0, January 2012 (started in 2010)
- (+/-) uses boost (some more dependancies...)
- (-) uses boost coding style (I tend to prefer CamelCaps or Java coding style)
- (-) a bit complex: offer many way to do the same thing where I would prefer a clean choice
- (-) thus it does not impose RAII, as it is still possible to open or close a database outside constructor/destructor
- (---) LPGPL: for me, this is a stopper as I would like to be able to use it in commercial products
- bazaar: http://bzr.ed.am/sqlite3cc
- bugtracker: personal: trac is still to be installed to http://dev.ed.am/sqlite3cc
- test suite: a simple main with boost filesystem (build librairy) depandancy

**sqdbcpp**: http://code.google.com/p/sqdbcpp/
- Dec 2009 (no more activity)
- (++) new BSD license
- (++) RAII design, with some good ideas, like the "Convertor" class
- (++) modern design, use RAII
- (+) CamelCaps naming convention
- (+) STL is the only depandancy
- (+) very small code
- (-) not RAII transactions
- (-) some unnecessary complexity to manage copyable objects (RefCount)
- (-) UTF-8/UTF-16: the second is not portable
- (--) Not documented (only a short example
- (---) Not maintained/not finished: contact author !?
- SVN: http://sqdbcpp.googlecode.com/svn/trunk/
- bugtracker: GoogleCode: http://code.google.com/p/sqdbcpp/issues/list
- test suite: with gtest

**sqlite3pp**: http://code.google.com/p/sqlite3pp/
- Sep 2007 to Mar 2009
- (++) MIT License
- (++) modern design, use RAII => can be a source of inspiration for me
- (+/-) uses boost (some more dependancies...)
- (-) complex: offer many way to do the same thing, and advance functionalities
- (--) Not documented in code
- (---) Not maintained, many open issues
- SVN: http://sqlite3pp.googlecode.com/svn/trunk/
- bugtracker: GoogleCode: http://code.google.com/p/sqlite3pp/issues/list

**SQLite++**: http://sqlitepp.berlios.de/
- Jan 2006 until now (Apr 2012)
- (++) still maintained
- (++) Boost License 1.0 
- (+) good online introduction
- (+/-) uses boost (some more dependancies...)
- (-) uses boost build system
- (-) come with yet another unicode string library
- (-) too complex and big to dive easily into
- (--) Not documented in code
- SVN: https://svn.berlios.de/svnroot/repos/sqlitepp/trunk/
- bugtracker: no!
- test suite: yes, with bjam

**CppSQLite**: http://www.codeproject.com/Articles/6343/CppSQLite-C-Wrapper-for-SQLite/
- Mar 2004, Jun 2011
- (++??) BSD License? CodeProject License?
- (+) famous CodeProject, good introductory article
- (--) Not documented in code
- (--) old design, no RAII
- (--) unnecessary complex memory allocation for exception
- VCS: no! but cloned twice on GitHub
- bugtracker: no!
- test suite: no, only some demo code.

**easySQLite**: http://code.google.com/p/easysqlite/
- Sep 2010
- (++) new BSD license
- (+/-) Manage table as objects, requiring to define their structure
- (-) not RAII
- (--) complex
- (---) Not maintained, some open issues
- SVN: http://easysqlite.googlecode.com/svn/trunk/
- bugtracker: http://code.google.com/p/easysqlite/source/list
- test suite: yes, a big one, with UnitTest++