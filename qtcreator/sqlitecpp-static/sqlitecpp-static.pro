
TEMPLATE = lib

CONFIG += console
CONFIG += staticlib

CONFIG -= app_bundle
CONFIG -= qt

DESTDIR = \
	../../libs

QMAKE_CFLAGS += \
	-Wno-sign-compare

INCLUDEPATH +=\
	../../sqlite3

SOURCES += \
	 ../../sqlite3/sqlite3.c \
	 ../../src/Transaction.cpp \
	 ../../src/Statement.cpp \
	 ../../src/Database.cpp \
	 ../../src/Column.cpp

HEADERS += \
	 ../../sqlite3/sqlite3.h \
	 ../../src/Transaction.h \
	 ../../src/Statement.h \
	 ../../src/SQLiteC++.h \
	 ../../src/Exception.h \
	 ../../src/Database.h \
	 ../../src/Column.h \
	 ../../src/Assertion.h
