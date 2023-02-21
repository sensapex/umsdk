TARGET = cppsample
QT =
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE  = app
SOURCES  += cppsample.cpp
INCLUDEPATH += ../..

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../lib/release/ -lum
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../lib/debug/ -lum
else:unix: LIBS += -L$$PWD/../lib/ -L/usr/local/lib -lum

INCLUDEPATH += $$PWD/../lib
DEPENDPATH += $$PWD/../lib
