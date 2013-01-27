#-------------------------------------------------
#
# Project created by QtCreator 2012-12-30T21:25:42
#
#-------------------------------------------------

QT       += core gui network

win32 {
    LIBS += -lqca2
    LIBS += -L$quote(C:\Users\Vernier\qca-ossl-2.0.0-beta3\lib) -lqca-ossl2
    QMAKE_CXXFLAGS += -std=c++0x
}
unix {
    LIBS += -lqca
    LIBS += -lssl
    QMAKE_CXXFLAGS += -std=c++11
}

CONFIG *= crypto

TARGET = SIS
TEMPLATE = app


SOURCES += main.cpp\
        sis.cpp \
    qmessageedit.cpp \
    qtabswidget.cpp \
    qmessagesbrowser.cpp

HEADERS  += sis.h \
    qmessageedit.h \
    qtabswidget.h \
    qmessagesbrowser.h
