#base settings
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app

#enable O3 optimization
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3

#include cppCORE library
INCLUDEPATH += $$PWD/../cppCORE
LIBS += -L$$PWD/../../bin -lcppCORE

#include zlib library
LIBS += -lz

HEADERS += \
        TestFramework.h \
        TFW_Test.h

SOURCES += \
        main.cpp
