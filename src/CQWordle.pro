TEMPLATE = app

QT += widgets

TARGET = CQWordle

DEPENDPATH += .

QMAKE_CXXFLAGS += -std=c++17

#CONFIG += debug

# Input
SOURCES += \
CQWordle.cpp \

HEADERS += \
CQWordle.h \
CWordleUseWords.h \
CWordleValidWords.h \

DESTDIR     = ../bin
OBJECTS_DIR = ../obj
LIB_DIR     = ../lib

INCLUDEPATH += \
../../CConfig/include \
.

unix:LIBS += \
-L../lib \
-L../../CConfig/lib \
-L../../CFile/lib \
-L../../CUtil/lib \
-L../../CStrUtil/lib \
-L../../COS/lib \
-lCConfig -lCFile -lCUtil -lCStrUtil -lCOS \
