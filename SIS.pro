#-------------------------------------------------
#
# Project created by QtCreator 2012-12-30T21:25:42
#
#-------------------------------------------------

QT       += core gui network phonon

LIBS += -lssl
LIBS += -lcrypto

TARGET = SIS
TEMPLATE = app


SOURCES += main.cpp\
        sis.cpp \
    qmessageedit.cpp \
    qmessagesbrowser.cpp \
    qwindow.cpp \
    friend.cpp

HEADERS  += sis.h \
    qmessageedit.h \
    qmessagesbrowser.h \
    qwindow.h \
    friend.h
