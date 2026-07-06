TARGET = tst_usermodel

CONFIG += testcase c++11
CONFIG -= app_bundle
QT += core sql testlib
QT -= gui

INCLUDEPATH += ../../../src

SOURCES += \
    tst_usermodel.cpp \
    ../../../src/models/usermodel.cpp \
    ../../../src/cache/userdb.cpp

HEADERS += \
    ../../../src/models/usermodel.h \
    ../../../src/cache/userdb.h
