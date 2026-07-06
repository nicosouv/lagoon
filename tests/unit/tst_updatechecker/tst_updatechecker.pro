TARGET = tst_updatechecker

CONFIG += testcase c++11
CONFIG -= app_bundle
QT += core network testlib
QT -= gui

INCLUDEPATH += ../../../src

SOURCES += \
    tst_updatechecker.cpp \
    ../../../src/updatechecker.cpp

HEADERS += \
    ../../../src/updatechecker.h
