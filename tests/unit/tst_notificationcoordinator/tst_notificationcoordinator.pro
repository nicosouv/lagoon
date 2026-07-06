TARGET = tst_notificationcoordinator

CONFIG += testcase c++11
CONFIG -= app_bundle
QT += core sql testlib
QT -= gui

INCLUDEPATH += ../../../src

SOURCES += \
    tst_notificationcoordinator.cpp \
    ../../../src/notificationcoordinator.cpp \
    ../../../src/models/conversationmodel.cpp \
    ../../../src/models/usermodel.cpp \
    ../../../src/cache/userdb.cpp

HEADERS += \
    ../../../src/notificationcoordinator.h \
    ../../../src/models/conversationmodel.h \
    ../../../src/models/usermodel.h \
    ../../../src/cache/userdb.h
