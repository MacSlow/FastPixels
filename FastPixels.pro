#-------------------------------------------------
#
# Project created by QtCreator 2016-05-24T11:22:09
#
#-------------------------------------------------

QT       += core gui
QMAKE_CXXFLAGS += -std=c++14

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FastPixels
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h \
    asm.h

FORMS    += mainwindow.ui
