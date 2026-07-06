TARGET = tst_messagemodel

CONFIG += testcase c++11
CONFIG -= app_bundle
QT += core sql testlib
QT -= gui

INCLUDEPATH += ../../../src

SOURCES += \
    tst_messagemodel.cpp \
    ../../../src/models/messagemodel.cpp \
    ../../../src/models/usermodel.cpp \
    ../../../src/slacktextformatter.cpp \
    ../../../src/cache/userdb.cpp

HEADERS += \
    ../../../src/models/messagemodel.h \
    ../../../src/models/usermodel.h \
    ../../../src/slacktextformatter.h \
    ../../../src/cache/userdb.h
