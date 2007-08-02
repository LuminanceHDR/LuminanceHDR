TEMPLATE = app
CONFIG += release qt thread
DEFINES += QT_NO_DEBUG_OUTPUT
QMAKE_CXXFLAGS += -O3 -funroll-loops -fstrength-reduce -fschedule-insns2 -felide-constructors -frerun-loop-opt -fexceptions -fno-strict-aliasing -fexpensive-optimizations -ffast-math -pipe
QMAKE_CXXFLAGS_RELEASE-=-O2

unix {
########################################### QT ###########################################
message( "Detecting Qt version:" )
checkqt4 = $$[QT_VERSION]
isEmpty(checkqt4) {
message( "error, Qt3 found!")
message( "It seems like we are using Qt3, which is wrong!" )
message( "to install qt4 in ubuntu run:" )
message( "sudo apt-get install libqt4-dev qt4-dev-tools libqt4-core" )
message( "to install qt4 in fedora core 6 run (as root):" )
message( "yum install qt4-devel" )
message( "Make sure also that you are using the Qt4 versions of the executables qmake, uic and moc." )
message( "To do so in Ubuntu you can install galternatives to switch those executable from Qt3 to Qt4, by running:" )
message( "sudo apt-get install galternatives" )
message( "and then run galternatives with:" )
message( "sudo galternatives" )
message( "In fedora you can simply invoke qmake with its full qt4 path:" )
message( "/usr/lib/qt4/bin/qmake" )
message( "In Ubuntu you can invoke Qt4's qmake with:" )
message( "qmake-qt4" )
message( "If you, on the other had, think that this message is wrong and indeed you HAVE Qt4, send an email to grota@users.sourceforge.net saying so." )
error( "fatal error, bailing out." )
} else {
message("Qt4, OK")
}

########################################### EXIV2 ###########################################
message ( "" )
message ( "Detecting exiv2:" )
#I think these are the only paths where we have to search for.
#If your system is more exotic let me know.
EXIV2IMAGEHPP = /usr/include/exiv2/image.hpp /usr/local/include/exiv2/image.hpp $$(LOCALSOFT)/include/exiv2/image.hpp 
for(path, EXIV2IMAGEHPP) {
	exists($$path) {
		EXIV2PATH = $$dirname(path)
		message ( headers found in $$EXIV2PATH)
	}
}
isEmpty(EXIV2PATH) {
	message("exiv2 devel package not found")
	message("In ubuntu you can run:")
	message("sudo apt-get install libexiv2-dev")
	message("in fedora core 6 run (as root):")
	message("yum install exiv2-devel")
	message("Or, if you have to compile the sources, go to www.exiv2.org")
	message( "If you, on the other had, think that this message is wrong and indeed you HAVE exiv2-devel installed, send an email to grota@users.sourceforge.net saying so." )
	error( "fatal error, bailing out." )
}
INCLUDEPATH	*= $$EXIV2PATH
LIBS		+= -lexiv2

########################################### OPENEXR ###########################################
#openEXR dependencies
message ( "" )
message ( "Detecting OpenEXR:" )
#I think these are the only paths where we have to search for.
#If your system is more exotic let me know.
OPENEXRHEADER = /usr/include/OpenEXR/ImfHeader.h /usr/local/include/OpenEXR/ImfHeader.h /usr/local/include/ilmbase/ImfHeader.h /usr/include/ilmbase/ImfHeader.h $$(LOCALSOFT)/include/OpenEXR/ImfHeader.h 
for(path, OPENEXRHEADER) {
	exists($$path) {
		OPENEXRDIR = $$dirname(path)
		message ( headers found in $$OPENEXRDIR)
	}
}
isEmpty(OPENEXRDIR) {
	message("OpenEXR devel package not found")
	message("In ubuntu you can run:")
	message("sudo apt-get install libopenexr-dev")
	message("in fedora core 6 run (as root):")
	message("yum install OpenEXR-devel")
	message("Or, if you have to compile the sources go to http://www.openexr.com")
	message("If you, on the other had, think that this message is wrong and indeed you HAVE OpenEXR-devel installed, send an email to grota@users.sourceforge.net saying so.")
	error("fatal error, bailing out.")
}
INCLUDEPATH *= $$OPENEXRDIR
LIBS += -lIlmImf -lImath -lHalf -lIex

########################################### FFTW3 ###########################################
#durand02 requires fftw3
message ( "" )
message ( "Detecting fftw3:" )
#I think these are the only paths where we have to search for.
#If your system is more exotic let me know.
FFTW3HEADER = /usr/include/fftw3.h /usr/local/include/fftw3.h $$(LOCALSOFT)/include/fftw3.h
for(path, FFTW3HEADER) {
	exists($$path) {
		FFTW3DIR = $$dirname(path)
		message ( headers found in $$FFTW3DIR)
	}
}
isEmpty(FFTW3DIR) {
	message("fftw3 devel package not found")
	message("In ubuntu you can run:")
	message("sudo apt-get install fftw3-dev")
	message("in fedora core 6 run (as root):")
	message("yum install fftw-devel")
	message("Or, if you have to compile the sources go to http://www.fftw.org")
	message("If you, on the other had, think that this message is wrong and indeed you HAVE fftw3-devel installed, send an email to grota@users.sourceforge.net saying so.")
	error( "fatal error, bailing out." )	
}
INCLUDEPATH *= $$FFTW3DIR
LIBS += -lfftw3f -lm
DEFINES += HAVE_FFTW

########################################### LIBTIFF ###########################################
#required, since we want to read hdr/ldr tiff files.
message ( "" )
message ( "Detecting libtiff:" )
#I think these are the only paths where we have to search for.
#If your system is more exotic let me know.
LIBTIFFHEADER = /usr/include/tiffio.h /usr/local/include/tiffio.h $$(LOCALSOFT)/include/tiffio.h
for(path, LIBTIFFHEADER) {
	exists($$path) {
		LIBTIFFDIR = $$dirname(path)
		message ( headers found in $$LIBTIFFDIR)
	}
}
isEmpty(LIBTIFFDIR) {
	message("libtiff devel package not found")
	message("In ubuntu you can run:")
	message("sudo apt-get install libtiff4-dev")
	message("in fedora core 6 run (as root):")
	message("yum install libtiff-devel")
	message("Or, if you have to compile the sources go to http://www.remotesensing.org/libtiff/")
	message("If you, on the other had, think that this message is wrong and indeed you HAVE libtiff-devel installed, send an email to grota@users.sourceforge.net saying so.")
	error( "fatal error, bailing out." )	
}
INCLUDEPATH *= $$LIBTIFFDIR
LIBS += -ltiff

########################################### LIBJPEG ###########################################
#dcraw requires libjpeg
message ( "" )
message ( "Detecting libjpeg:" )
#I think these are the only paths where we have to search for.
#If your system is more exotic let me know.
LIBJPEGHEADER = /usr/include/jpeglib.h /usr/local/include/jpeglib.h $$(LOCALSOFT)/include/jpeglib.h 
for(path, LIBJPEGHEADER) {
	exists($$path) {
		LIBJPEGDIR = $$dirname(path)
		message ( headers found in $$LIBJPEGDIR)
	}
}
isEmpty(LIBJPEGDIR) {
	message("libjpeg devel package not found")
	message("In ubuntu you can run:")
	message("sudo apt-get install libjpeg62-dev")
	message("in fedora core 6 run (as root):")
	message("yum install libjpeg-devel")
	message("Or, if you have to compile the sources go to http://www.ijg.org")
	message("If you, on the other had, think that this message is wrong and indeed you HAVE libjpeg-devel installed, send an email to grota@users.sourceforge.net saying so.")
	error( "fatal error, bailing out." )	
}
INCLUDEPATH *= $$LIBJPEGDIR
LIBS += -ljpeg
######################################## end of detection ########################################

############################## required by "make install" ########################################
isEmpty(PREFIX) {
        PREFIX = /usr/local
}
isEmpty(I18NDIR) {
	I18NDIR = $${PREFIX}/share/qtpfsgui/i18n
}

target.path      = $${PREFIX}/bin
menu.files       = qtpfsgui.desktop
menu.path        = $${PREFIX}/share/applications
icon.files       = images/qtpfsgui.png
icon.path        = $${PREFIX}/share/icons/hicolor/48x48/apps
htmls.files      = html
htmls.path       = $${PREFIX}/share/qtpfsgui
i18n.files       = i18n/lang_de.qm i18n/lang_es.qm i18n/lang_it.qm i18n/lang_fr.qm i18n/lang_pl.qm i18n/lang_tr.qm
i18n.path        = $$I18NDIR

INSTALLS        += target menu icon htmls i18n
message ( "" )
message ("********************************************************************")
message ("Installation PREFIX=$$PREFIX")
isEmpty(ENABLE_DEBUG) | contains(ENABLE_DEBUG, "no") {
message ("Debug statements DISABLED")
} else {
DEFINES -= QT_NO_DEBUG_OUTPUT
message ("Debug statements ENABLED")
}
message ("Here's what will be installed:")
message ("qtpfsgui ==> $$target.path")
message ("qtpfsgui.desktop ==> $$menu.path")
message ("qtpfsgui.png ==> $$icon.path")
message ("html directory ==> $$htmls.path")
message ("i18n messages ==> $$i18n.path")
message ("********************************************************************")

MAJOR_MINOR_QT_VERSION = $$[QT_VERSION]
MAJOR_MINOR_QT_VERSION ~= s/(4\..)\../\1
contains(MAJOR_MINOR_QT_VERSION,4.1) {
DEFINES += I18NDIR=\"$$I18NDIR\"
#message("Detected Qt4.1")
} else {
DEFINES += I18NDIR=\\\"$$I18NDIR\\\"
#message("Detected Qt4.2")
}
#################################################################################################
#CONFIG += debug
}

macx {
ICON = images/qtpfsgui.icns

#TODO we have to complete this.
LIBS+=-lIlmThread

# Enable universal (require a universal Qt)
CONFIG += x86 ppc
# Add some extra PATHS in LIBS:
LIBS += -L$$(LOCALSOFT)/lib

#I18NDIR=(QCoreApplication::applicationDirPath()+\"/i18n\")
#I18NDIR=/Applications/qtpfsgui.app/Contents/Resources/i18n
#maybe we need parenthesis and/or backslashes somewhere.
I18NDIR=QCoreApplication::applicationDirPath()+"/../Resources/i18n"
DEFINES += I18NDIR=\"$$I18NDIR\"
}

win32 {
# this is just how my MinGW installation is. You gotta change it if you want to compile it in windows.
CONFIG += windows
#CONFIG += debug
#CONFIG += console

#OpenEXR available in win32
LIBS += -lIlmImf -lImath -lHalf -lIex
LIBS += -LC:\msys\1.0\local\lib
INCLUDEPATH += C:\msys\1.0\local\include\OpenEXR

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

#tiff
INCLUDEPATH += "C:\Program Files\GnuWin32\include"
LIBS += "-LC:\Program Files\GnuWin32\lib" -llibtiff

DEFINES += I18NDIR=(QCoreApplication::applicationDirPath()+\\\"/i18n\\\")
RC_FILE = images/qtpfsgui_ico.rc

}

#INCLUDEPATH += src/libpfs src/fileformat src/hdrcreate src/ashikhmin02 src/drago03 src/durand02 src/fattal02 src/pattanaik00 src/reinhard02

FORMS = forms/maingui.ui \
        forms/hdrwizardform.ui \
        forms/tonemappingdialog.ui \
        forms/tonemappingoptions.ui \
        forms/help_about.ui \
        forms/options.ui \
        forms/transplantexifdialog.ui \
        forms/resizedialog.ui \
        forms/gamma_and_levels.ui \
        forms/batch_dialog.ui

HEADERS += src/Libpfs/array2d.h \
           src/Libpfs/pfs.h \
           src/options.h \
           src/MainWindow/maingui_impl.h \
           src/MainWindow/hdrviewer.h \
           src/MainWindow/luminancerange_widget.h \
           src/MainWindow/histogram.h \
           src/HdrWizard/hdrwizardform_impl.h \
           src/ToneMappingDialog/gamma_and_levels.h \
           src/ToneMappingDialog/tonemappingdialog_impl.h \
           src/ToneMappingDialog/tonemapping_widget.h \
           src/Threads/tonemapper_thread.h \
           src/Threads/io_threads.h \
           src/Options/options_impl.h \
           src/TransplantExif/transplant_impl.h \
           src/Resize/resizedialog_impl.h \
           src/HdrWizard/hdrcreation/createhdr.h \
           src/HdrWizard/hdrcreation/robertson02.h \
           src/HdrWizard/hdrcreation/responses.h   \
           src/HdrWizard/hdrcreation/icip06.h \
           src/HdrWizard/hdrcreation/debevec.h \
           src/ToneMappingDialog/ldrviewer.h \
           src/smart_scroll_area.h \
           src/ToneMappingDialog/gang.h \
           src/Fileformat/rgbeio.h \
           src/Fileformat/pfstiff.h \
           src/TM_operators/ashikhmin02/pyramid.h \
           src/TM_operators/ashikhmin02/tmo_ashikhmin02.h \
           src/TM_operators/drago03/tmo_drago03.h \
           src/TM_operators/durand02/tmo_durand02.h \
           src/TM_operators/durand02/fastbilateral.h \
           src/TM_operators/fattal02/tmo_fattal02.h \
           src/TM_operators/fattal02/pde.h \
           src/TM_operators/pattanaik00/tmo_pattanaik00.h \
           src/TM_operators/reinhard02/tmo_reinhard02.h \
           src/TM_operators/mantiuk06/contrast_domain.h \
           src/Batch/batch_dialog_impl.h \
           src/Exif/exif_operations.h

SOURCES += src/Libpfs/pfs.cpp \
           src/Libpfs/colorspace.cpp \
           src/main.cpp \
           src/options.cpp \
           src/MainWindow/maingui_impl.cpp \
           src/MainWindow/hdrviewer.cpp \
           src/MainWindow/luminancerange_widget.cpp \
           src/MainWindow/histogram.cpp \
           src/HdrWizard/hdrwizardform_impl.cpp \
           src/ToneMappingDialog/gamma_and_levels.cpp \
           src/ToneMappingDialog/tonemappingdialog_impl.cpp \
           src/ToneMappingDialog/tonemapping_widget.cpp \
           src/Threads/tonemapper_thread.cpp \
           src/Threads/io_threads.cpp \
           src/Options/options_impl.cpp \
           src/TransplantExif/transplant_impl.cpp \
           src/Resize/resizedialog_impl.cpp \
           src/HdrWizard/hdrcreation/createhdr.cpp \
           src/HdrWizard/hdrcreation/robertson02.cpp  \
           src/HdrWizard/hdrcreation/responses.cpp  \
           src/HdrWizard/hdrcreation/icip06.cpp \
           src/HdrWizard/hdrcreation/debevec.cpp \
           src/ToneMappingDialog/ldrviewer.cpp \
           src/smart_scroll_area.cpp \
           src/ToneMappingDialog/gang.cpp \
           src/Filter/pfsrotate.cpp \
           src/Filter/pfssize.cpp \
           src/Filter/pfsgamma.cpp \
           src/Fileformat/pfsinrgbe.cpp \
           src/Fileformat/pfsoutrgbe.cpp \
           src/Fileformat/rgbeio.cpp \
           src/Fileformat/pfsoutldrimage.cpp \
           src/Fileformat/pfsindcraw.cpp \
           src/Fileformat/pfstiff.cpp \
           src/TM_operators/ashikhmin02/pfstmo_ashikhmin02.cpp \
           src/TM_operators/ashikhmin02/tmo_ashikhmin02.cpp \
           src/TM_operators/drago03/pfstmo_drago03.cpp \
           src/TM_operators/drago03/tmo_drago03.cpp \
           src/TM_operators/durand02/pfstmo_durand02.cpp \
           src/TM_operators/durand02/tmo_durand02.cpp \
           src/TM_operators/durand02/fastbilateral.cpp \
           src/TM_operators/fattal02/pfstmo_fattal02.cpp \
           src/TM_operators/fattal02/tmo_fattal02.cpp \
           src/TM_operators/fattal02/pde.cpp \
           src/TM_operators/reinhard04/pfstmo_reinhard04.cpp \
           src/TM_operators/pattanaik00/tmo_pattanaik00.cpp \
           src/TM_operators/pattanaik00/pfstmo_pattanaik00.cpp \
           src/TM_operators/reinhard02/pfstmo_reinhard02.cpp \
           src/TM_operators/reinhard02/tmo_reinhard02.cpp \
           src/TM_operators/reinhard02/approx.cpp \
           src/TM_operators/mantiuk06/contrast_domain.cpp \
           src/TM_operators/mantiuk06/pfstmo_mantiuk06.cpp \
           src/Batch/batch_dialog_impl.cpp \
           src/Exif/exif_operations.cpp \
           src/Fileformat/pfsinexr.cpp \
           src/Fileformat/pfsoutexr.cpp

RESOURCES = icons.qrc

TRANSLATIONS = i18n/lang_it.ts \
               i18n/lang_fr.ts \
               i18n/lang_de.ts \
               i18n/lang_es.ts \
               i18n/lang_pl.ts \
               i18n/lang_tr.ts

# Old durand, we use the fftw version now.
#src/durand02/bilateral.h \
#src/durand02/bilateral.cpp \

# Manual align dialog. we should call Pablo's application now.
#           src/align_impl.cpp \
#           src/show_image.cpp \
#           src/align_impl.h \
#           src/show_image.h \
#        forms/aligndialog.ui

TARGET = qtpfsgui

MOC_DIR = generated_moc
OBJECTS_DIR = generated_obj
UI_DIR = generated_uic
RCC_DIR = generated_moc
