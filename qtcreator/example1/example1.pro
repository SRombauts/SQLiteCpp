
TEMPLATE = app

CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += \
	../../sqlite3

LIBS += \
	-L../../libs \
	-lsqlitecpp-static

SOURCES += \
	 ../../examples/example1/main.cpp

