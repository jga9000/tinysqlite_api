TEMPLATE = lib

# dll name is determined from filename
TARGET = 
QT += network
DEPENDPATH += src ../inc inc
MOC_DIR = moc

INCLUDEPATH += ../inc inc

# Sources
SOURCES += sqliteapi.cpp \
    sqliteapiclient.cpp \
    sqliteapiclientnotifier.cpp

DEFINES += SQLITEAPI_NO_EXPORT
# following define is for export, creates lib
DEFINES += BUILD_SQLITEAPI

HEADERS += sqliteapiglobal.h \
    tinysqliteapidefs.h \
    sqliteapiserverdefs.h \    
    tinysqliteapiclient.h \
    tinysqliteapiclientnotifier.h \
    tinysqliteapi.h

win32: {
#for simulations
    DESTDIR += $$(QTDIR)/bin
}
