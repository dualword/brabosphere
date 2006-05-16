TEMPLATE = lib
DEFINES += INCHI_LINK_AS_DLL
DEFINES -= UNICODE
CONFIG += release exceptions rtti thread
CONFIG -= qt
INCLUDEPATH += src data windows-vc2005 src\formats\xml 
INCLUDEPATH += D:\Xbrabo\libxml2-2.6.20.win32\include
INCLUDEPATH += D:\Xbrabo\iconv-1.9.1.win32\include
OBJECTS_DIR = obj\static
LIBS += -Lwindows-vc2005 -lzdll
LIBS += -Lwindows -llibinchi
LIBS += -LD:\Xbrabo\libxml2-2.6.20.win32\lib -llibxml2 -llibxml2_a
LIBS += -LD:\Xbrabo\iconv-1.9.1.win32\lib -liconv -liconv_a
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
           src\mol.cpp \
           src\molchrg.cpp \
           src\obconversion.cpp \
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
           src\fingerprints\finger2.cpp \
           src\fingerprints\finger3.cpp \
           src\formats\alchemyformat.cpp \
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
           src\formats\copyformat.cpp \
           src\formats\crkformat.cpp \
           src\formats\CSRformat.cpp \
           src\formats\cssrformat.cpp \
           src\formats\dmolformat.cpp \
           src\formats\exampleformat.cpp \
           src\formats\fastsearchformat.cpp \
           src\formats\featformat.cpp \
           src\formats\fhformat.cpp \
           src\formats\fingerprintformat.cpp \
           src\formats\freefracformat.cpp \
           src\formats\gamessformat.cpp \
           src\formats\gaussformat.cpp \
           src\formats\ghemicalformat.cpp \
           src\formats\gromos96format.cpp \
           src\formats\hinformat.cpp \
           src\formats\inchiformat.cpp \
           src\formats\jaguarformat.cpp \
           src\formats\mdlformat.cpp \
           src\formats\mmodformat.cpp \
           src\formats\mol2format.cpp \
           src\formats\mopacformat.cpp \
           src\formats\mpdformat.cpp \
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
           src\formats\zindoformat.cpp \
           src\math\matrix3x3.cpp \
           src\math\vector3.cpp \
           src\formats\xml\cmlreactlformat.cpp \
           src\formats\xml\pubchem.cpp \
           src\formats\xml\xcmlformat.cpp \
           src\formats\xml\xml.cpp \
           src\formats\xml\xmlformat.cpp
