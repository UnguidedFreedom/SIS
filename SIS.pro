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
    messageedit.cpp \
    messagesbrowser.cpp \
    conversation.cpp \
    contact.cpp

HEADERS  += sis.h \
    messageedit.h \
    messagesbrowser.h \
    conversation.h \
    contact.h
