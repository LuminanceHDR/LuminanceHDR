TEMPLATE = app

CONFIG += release qt thread

unix {
target.path	= /usr/local/bin
menu.files      = qtpfsgui.desktop
menu.path       = /usr/share/applications
icon.files      = images/qtpfsgui.png
icon.path       = /usr/share/pixmaps
INSTALLS	+= target menu icon

#exiv2
INCLUDEPATH	+= /usr/local/include/exiv2
LIBS		+= -lexiv2

#openEXR dependencies 
INCLUDEPATH += /usr/include/OpenEXR
LIBS += -lIlmImf -lImath -lHalf -lIex

#durand02 with fftw3
LIBS += -lfftw3f -lm
DEFINES += HAVE_FFTW

#dcraw
LIBS += -ljpeg

#INCLUDEPATH += /usr/include
#CONFIG += debug
#CONFIG -= x11
}

win32 {
CONFIG += windows
#CONFIG += debug
#CONFIG += console

#OpenEXR not available yet
#LIBS += -lIlmImf -lImath -lHalf -lIex
#LIBS += -LC:\devcppprojects\temp
#INCLUDEPATH += C:\cygwin\usr\local\include\OpenEXR

#exiv2
INCLUDEPATH += C:\msys\1.0\local\include\exiv2
LIBS            += -lexiv2 -LC:\msys\1.0\local\lib

#fftw3
LIBS += -lfftw3f-3 -lm 
DEFINES += HAVE_FFTW
INCLUDEPATH += C:\comp_prj\fftw3
LIBS += -LC:\comp_prj\fftw3

#dcraw
LIBS += -ljpeg62 -lWs2_32
LIBS += -LC:\comp_prj\libjpeg
INCLUDEPATH += C:\comp_prj\libjpeg\include

RC_FILE = src/qtpfsgui_ico.rc
}

INCLUDEPATH += src/libpfs src/fileformat src/hdrcreate src/ashikhmin02 src/drago03 src/durand02 src/fattal02 src/pattanaik00 src/reinhard02

FORMS = forms/maingui.ui \
        forms/hdrwizardform.ui \
        forms/tonemappingdialog.ui \
        forms/help_about.ui \
        forms/options.ui

RESOURCES = icons.qrc

HEADERS += src/libpfs/array2d.h \
           src/libpfs/pfs.h \
           src/maingui_impl.h \
           src/hdrwizardform_impl.h \
           src/tonemappingdialog_impl.h \
           src/options_impl.h \
           src/hdrcreate/createhdr.h \
           src/hdrcreate/robertson02.h \
           src/hdrcreate/responses.h   \
           src/hdrcreate/icip06.h \
           src/hdrcreate/debevec.h \
           src/imagehdrviewer.h \
           src/luminancerange_widget.h \
           src/histogram.h \
           src/gang.h \
           src/fileformat/rgbeio.h \
           src/ashikhmin02/pyramid.h \
           src/ashikhmin02/tmo_ashikhmin02.h \
           src/drago03/tmo_drago03.h \
           src/durand02/tmo_durand02.h \
           src/durand02/fastbilateral.h \
           src/fattal02/tmo_fattal02.h \
           src/fattal02/pde.h \
           src/pattanaik00/tmo_pattanaik00.h \
           src/reinhard02/tmo_reinhard02.h

SOURCES += src/libpfs/pfs.cpp \
           src/libpfs/colorspace.cpp \
           src/main.cpp \
           src/maingui_impl.cpp \
           src/tonemappingdialog_impl.cpp \
           src/options_impl.cpp \
           src/hdrcreate/createhdr.cpp \
           src/hdrwizardform_impl.cpp \
           src/hdrcreate/robertson02.cpp  \
           src/hdrcreate/responses.cpp  \
           src/hdrcreate/icip06.cpp \
           src/hdrcreate/debevec.cpp \
           src/imagehdrviewer.cpp \
           src/luminancerange_widget.cpp \
           src/histogram.cpp \
           src/gang.cpp \
           src/filter/pfsrotate.cpp \
           src/filter/pfssize.cpp \
           src/filter/pfsgamma.cpp \
           src/fileformat/pfsinrgbe.cpp \
           src/fileformat/pfsoutrgbe.cpp \
           src/fileformat/rgbeio.cpp \
           src/fileformat/pfsoutldrimage.cpp \
           src/fileformat/pfsindcraw.cpp \
           src/ashikhmin02/pfstmo_ashikhmin02.cpp \
           src/ashikhmin02/tmo_ashikhmin02.cpp \
           src/drago03/pfstmo_drago03.cpp \
           src/drago03/tmo_drago03.cpp \
           src/durand02/pfstmo_durand02.cpp \
           src/durand02/tmo_durand02.cpp \
           src/durand02/fastbilateral.cpp \
           src/fattal02/pfstmo_fattal02.cpp \
           src/fattal02/tmo_fattal02.cpp \
           src/fattal02/pde.cpp \
           src/reinhard04/pfstmo_reinhard04.cpp \
           src/pattanaik00/tmo_pattanaik00.cpp \
           src/pattanaik00/pfstmo_pattanaik00.cpp \
           src/reinhard02/pfstmo_reinhard02.cpp \
           src/reinhard02/tmo_reinhard02.cpp \
           src/reinhard02/approx.cpp

#src/durand02/bilateral.h \
#src/durand02/bilateral.cpp \

unix {
SOURCES += src/fileformat/pfsinexr.cpp \
           src/fileformat/pfsoutexr.cpp
}

TARGET = qtpfsgui

MOC_DIR = generated_moc
OBJECTS_DIR = generated_obj
UI_DIR = generated_uic
RCC_DIR = generated_moc
