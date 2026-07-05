TARGET = tst_websocketclient

CONFIG += testcase c++11
CONFIG -= app_bundle
QT += core network websockets testlib
QT -= gui

INCLUDEPATH += ../../../src

SOURCES += \
    tst_websocketclient.cpp \
    ../../../src/websocketclient.cpp

HEADERS += \
    ../../../src/websocketclient.h
