TARGET = tst_slackapi

CONFIG += testcase c++11
CONFIG -= app_bundle
QT += core network websockets testlib
QT -= gui

INCLUDEPATH += ../../../src

SOURCES += \
    tst_slackapi.cpp \
    mockslackserver.cpp \
    ../../../src/slackapi.cpp \
    ../../../src/websocketclient.cpp

HEADERS += \
    mockslackserver.h \
    ../../../src/slackapi.h \
    ../../../src/websocketclient.h
