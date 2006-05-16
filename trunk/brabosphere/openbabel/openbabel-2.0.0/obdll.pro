TEMPLATE = lib
DEFINES += USING_DYNAMIC_LIBS OBDLL_EXPORTS 
DEFINES -= UNICODE
CONFIG += release exceptions rtti thread dll
CONFIG -= qt
INCLUDEPATH += src data
OBJECTS_DIR = obj\obdll
DESTDIR = lib

SOURCES += src\atom.cpp \
           src\base.cpp \
           src\bitvec.cpp \
           src\bond.cpp \
           src\bondtyper.cpp \
           src\chains.cpp \
           src\chiral.cpp \
           src\data.cpp \
           src\fingerprint.cpp \
           src\generic.cpp \
           src\grid.cpp \
           src\kekulize.cpp \
           src\matrix.cpp \
           src\math\matrix3x3.cpp \
           src\mol.cpp \
           src\molchrg.cpp \
           src\oberror.cpp \
           src\obiter.cpp \
           src\obutil.cpp \
           src\parsmart.cpp \
           src\patty.cpp \
           src\phmodel.cpp \
           src\rand.cpp \
           src\residue.cpp \
           src\ring.cpp \
           src\rotamer.cpp \
           src\rotor.cpp \
           src\tokenst.cpp \
           src\transform.cpp \
           src\typer.cpp \
           src\math\vector3.cpp
