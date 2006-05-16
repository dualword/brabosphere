TEMPLATE = lib
DEFINES += USING_DYNAMIC_LIBS USING_OBDLL
DEFINES -= UNICODE
CONFIG += release exceptions rtti thread dll
CONFIG -= qt
INCLUDEPATH += src data windows-vc2005 
LIBS += -Llib -lobconv -lobdll
OBJECTS_DIR = obj\obformats2
DESTDIR = lib
SOURCES += src\formats\alchemyformat.cpp \
           src\formats\amberformat.cpp \
           src\formats\APIInterface.cpp \
           src\formats\balstformat.cpp \
           src\formats\bgfformat.cpp \
           src\formats\boxformat.cpp \
           src\formats\cacaoformat.cpp \
           src\formats\cacheformat.cpp \
           src\formats\carformat.cpp \
           src\formats\cccformat.cpp \
           src\formats\chem3dformat.cpp \
           src\formats\chemdrawformat.cpp \
           src\formats\chemtoolformat.cpp \
           src\formats\crkformat.cpp \
           src\formats\CSRformat.cpp \
           src\formats\cssrformat.cpp \
           src\formats\dmolformat.cpp \
           src\formats\featformat.cpp \
           src\formats\fhformat.cpp \
           src\formats\freefracformat.cpp \
           src\formats\gamessformat.cpp \
           src\formats\gaussformat.cpp \
           src\formats\ghemicalformat.cpp \
           src\formats\gromos96format.cpp \
           src\formats\hinformat.cpp \
           src\formats\jaguarformat.cpp \
           src\formats\mdlformat.cpp \
           src\formats\mmodformat.cpp \
           src\formats\mol2format.cpp \
           src\formats\mopacformat.cpp \
           src\formats\mpqcformat.cpp \
           src\formats\nwchemformat.cpp \
           src\formats\pcmodelformat.cpp \
           src\formats\pdbformat.cpp \
           src\formats\povrayformat.cpp \
           src\formats\PQSformat.cpp \
           src\formats\qchemformat.cpp \
           src\formats\reportformat.cpp \
           src\formats\rxnformat.cpp \
           src\formats\shelxformat.cpp \
           src\formats\smilesformat.cpp \
           src\formats\tinkerformat.cpp \
           src\formats\turbomoleformat.cpp \
           src\formats\unichemformat.cpp \
           src\formats\viewmolformat.cpp \
           src\formats\xedformat.cpp \
           src\formats\xyzformat.cpp \
           src\formats\yasaraformat.cpp \
           src\formats\zindoformat.cpp
