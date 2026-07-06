TARGET = tst_messagemodel

CONFIG += testcase c++11
CONFIG -= app_bundle
QT += core testlib
QT -= gui

INCLUDEPATH += ../../../src

SOURCES += \
    tst_messagemodel.cpp \
    ../../../src/models/messagemodel.cpp \
    ../../../src/models/usermodel.cpp \
    ../../../src/slacktextformatter.cpp

HEADERS += \
    ../../../src/models/messagemodel.h \
    ../../../src/models/usermodel.h \
    ../../../src/slacktextformatter.h
