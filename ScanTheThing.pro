#-------------------------------------------------
#
# Project created by QtCreator 2016-10-04T17:30:49
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ScanTheThing
TEMPLATE = app

SOURCES += \
        main.cpp\
        scanthething.cpp \
        depthscanner.cpp \
        depthreconstruct.cpp \
        depthorient.cpp

HEADERS  += \
        scanthething.h \
        depthscanner.h \
        depthreconstruct.h \
        depthorient.h

FORMS    += scanthething.ui
