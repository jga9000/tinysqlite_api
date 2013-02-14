TEMPLATE = app

# exe name is determined from filename
TARGET = 
QT += core \
    network \
    sql
DEPENDPATH += src ../inc inc
MOC_DIR = moc
CONFIG += qt \
    no_icon

INCLUDEPATH += ./inc ../inc

SOURCES += servermain.cpp \
    sqliteapiserver.cpp \
    sqliteapirequesthandler.cpp \
    sqliteapiresponsehandler.cpp \
    sqliteapirequestmsg.cpp \
    sqliteapisql.cpp \
    sqliteapistorage.cpp \
    sqliteapiresponsemsg.cpp

# Sources
HEADERS += sqliteapiglobal.h \
    sqliteapidefs.h \
    sqliteapiserverdefs.h \
    sqliteapiserver.h \
    sqliteapirequesthandler.h \
    sqliteapirequestmsg.h \
    sqliteapiresponsehandler.h \
    sqliteapisql.h \
    sqliteapistorage.h \
    sqliteapiresponsemsg.h \
    serverlauncher.h

win32: {
#for simulations    
    CONFIG += console\
              debug
    DESTDIR += $$(QTDIR)/bin
}
