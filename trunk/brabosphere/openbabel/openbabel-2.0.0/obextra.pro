TEMPLATE = lib
DEFINES += USING_DYNAMIC_LIBS USING_OBDLL INCHI_LINK_AS_DLL
DEFINES -= UNICODE
CONFIG += release exceptions rtti thread dll
CONFIG -= qt
INCLUDEPATH += src data windows-vc2005 
LIBS += -Llib -lobconv -lobdll
LIBS += -Lwindows -llibinchi
OBJECTS_DIR = obj\obextra
DESTDIR = lib
SOURCES += src\formats\fastsearchformat.cpp \
           src\fingerprints\finger2.cpp \
           src\fingerprints\finger3.cpp \
           src\formats\fingerprintformat.cpp \
           src\formats\inchiformat.cpp \
           src\formats\mpdformat.cpp