http://www.sqlite.org/cvstrac/wiki?p=SqliteWrappers
http://stackoverflow.com/questions/120295/what-is-a-good-oo-c-wrapper-for-sqlite
http://stackoverflow.com/questions/818155/sqlite-alternatives-for-c


 - **SQLite++**: uses boost build system, Boost License 1.0 (http://sqlitepp.berlios.de/)
 - **CppSQLite**: famous Code Project but old design, BSD License (http://www.codeproject.com/Articles/6343/CppSQLite-C-Wrapper-for-SQLite/)
 
 
**sqlite3cc**: http://ed.am/dev/sqlite3cc/
- v0.1
- Nov 2009, Jan 2012
- (++) modern design, use RAII => can be a source of inspiration for me
- (++) very well documented, in code and with a very good informal presentation
- (+) is maintained (recent), initial release is 0.1.0, January 2012 (started in 2010)
- (+/-) uses boost (some more dependancies...)
- (-) uses boost coding style (I tend to prefer CamelCaps or Java coding style)
- (-) a bit complicated : offer many way to do the same thing where I would prefer a clean choice
- (-) thus it does not impose RAII, as it is still possible to open or close a database outside constructor/destructor
- (---) LPGPL : for me, this is a stopper as I would like to be able to use it in commercial products
- bazaar: http://bzr.ed.am/sqlite3cc
- bugtracker: personal: trac is still to be installed to http://dev.ed.am/sqlite3cc
 
=> inspiration :
- bind named parameters,
- support for different transaction mode
- comment on returning error code instead of exception that shall not be thrown when exepected (!?)
- explain the noncopyable property for RAII design

**sqdbcpp**: http://code.google.com/p/sqdbcpp/
- Dec 2009 (no more activity)
- (++) new BSD license
- (+) CamelCaps
- (+) STL is the only depandancy
- (+) RAII design, with some good ideas,
- (-) but with some unnecessary complexity, and some unused code (RefCount...)
- (-) UTF-8/UTF-16 : the second is not portable
- (--) Not documented
- (---) Not maintained/not finished : contact author !?
- SVN: http://sqdbcpp.googlecode.com/svn/trunk/
- bugtracker: GoogleCode: http://code.google.com/p/sqdbcpp/issues/list

**sqlite3pp**: http://code.google.com/p/sqlite3pp/
- Sep 2007 to Mar 2009
- (++) MIT License
- (+/-) uses boost (some more dependancies...)
- 

