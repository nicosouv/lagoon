TARGET = tst_workspacemanager

CONFIG += testcase c++11
CONFIG -= app_bundle
QT += core testlib
QT -= gui

INCLUDEPATH += ../../../src

SOURCES += \
    tst_workspacemanager.cpp \
    ../../../src/workspacemanager.cpp

HEADERS += \
    ../../../src/workspacemanager.h
