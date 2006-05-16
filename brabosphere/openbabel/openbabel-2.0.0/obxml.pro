# The locations of the iconv and libxml2 libraries
ICONVDIR = ..\iconv-1.9.1.win32
LIBXML2DIR = ..\libxml2-2.6.23.win32
# 
TEMPLATE = lib
DEFINES += USING_DYNAMIC_LIBS USING_OBDLL INCHI_LINK_AS_DLL
DEFINES -= UNICODE
CONFIG += release exceptions rtti thread dll
CONFIG -= qt
INCLUDEPATH += src data windows-vc2005 src\formats\xml 
INCLUDEPATH += $$LIBXML2DIR\include
INCLUDEPATH += $$ICONVDIR\include
LIBS += -Llib -lobconv -lobdll
LIBS += -Lwindows -llibinchi
LIBS += -L$$LIBXML2DIR\lib -llibxml2 -llibxml2_a
LIBS += -L$$ICONVDIR\lib -liconv -liconv_a
OBJECTS_DIR = obj\obxml
DESTDIR = lib
SOURCES += src\formats\xml\cmlreactlformat.cpp \
           src\formats\xml\pubchem.cpp \
           src\formats\xml\xcmlformat.cpp \
           src\formats\xml\xml.cpp \
           src\formats\xml\xmlformat.cpp