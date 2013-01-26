#-------------------------------------------------
#
# Project created by QtCreator 2012-12-30T21:25:42
#
#-------------------------------------------------

QT       += core gui network

win32 {
    LIBS += -lqca2
    LIBS += -lqca-ossl2
}
unix {
    LIBS += -lqca
    LIBS += -lssl
}

CONFIG += crypto

TARGET = SIS
TEMPLATE = app


SOURCES += main.cpp\
        sis.cpp

HEADERS  += sis.h

QMAKE_CXXFLAGS += -std=c++11
