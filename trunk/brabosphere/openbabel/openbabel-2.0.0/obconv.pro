TEMPLATE = lib
DEFINES += USING_DYNAMIC_LIBS OBCONV_EXPORTS HAVE_CONFIG_H
DEFINES -= UNICODE
CONFIG += release exceptions rtti thread dll
CONFIG -= qt
INCLUDEPATH += src data windows-vc2005
LIBS += -Lwindows-vc2005 -lzdll
OBJECTS_DIR = obj\obconv
DESTDIR = lib

SOURCES += src\dlhandler_win32.cpp \
           src\obconversion.cpp