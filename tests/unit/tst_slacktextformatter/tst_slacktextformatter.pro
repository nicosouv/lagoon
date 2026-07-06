TARGET = tst_slacktextformatter

CONFIG += testcase c++11
CONFIG -= app_bundle
QT += core testlib
QT -= gui

INCLUDEPATH += ../../../src

SOURCES += \
    tst_slacktextformatter.cpp \
    ../../../src/slacktextformatter.cpp \
    ../../../src/models/usermodel.cpp

HEADERS += \
    ../../../src/slacktextformatter.h \
    ../../../src/models/usermodel.h
