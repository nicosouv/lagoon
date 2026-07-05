TARGET = tst_conversationmodel

CONFIG += testcase c++11
CONFIG -= app_bundle
QT += core testlib
QT -= gui

INCLUDEPATH += ../../../src

SOURCES += \
    tst_conversationmodel.cpp \
    ../../../src/models/conversationmodel.cpp \
    ../../../src/models/usermodel.cpp

HEADERS += \
    ../../../src/models/conversationmodel.h \
    ../../../src/models/usermodel.h
