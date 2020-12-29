#-------------------------------------------------
#
# Sensapex uMp SDK library (c) 2016 Sensapex oy
#
#-------------------------------------------------

QT    -= core gui

TARGET = um
TEMPLATE = lib
#CONFIG += staticlib

DEFINES += LIBUM_LIBRARY

SOURCES += libum.c 

HEADERS += ../../libum.h \
           smcp1.h

INCLUDEPATH += ../..

windows: {
    LIBS += -lws2_32
    LIBS += -shared
    addFiles.path = .
    addFiles.sources = libum.dll
    DEPLOYMENT += addFiles


    dlltarget.path = $$PWD/bin
    target.path = $$PWD/bin
    INSTALLS += target dlltarget
}

unix: {
    target.path = /usr/local/lib
    INSTALLS += target
}

mac: {

}

