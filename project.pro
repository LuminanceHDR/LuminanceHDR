TEMPLATE = app
CONFIG += release qt thread
#DEFINES += QT_NO_DEBUG_OUTPUT

# Assume openmp-capable g++ (>=4.2)
QMAKE_CXXFLAGS += -funroll-loops -fstrength-reduce -fschedule-insns2 -felide-constructors -frerun-loop-opt -fexceptions -fno-strict-aliasing -fexpensive-optimizations -ffast-math -pipe -fopenmp -msse2
QMAKE_LFLAGS += -fopenmp

TARGET = qtpfsgui

MOC_DIR = generated_moc
OBJECTS_DIR = generated_obj
UI_DIR = generated_uic
RCC_DIR = generated_moc


FORMS = forms/maingui.ui \
        forms/dndoption.ui \
        forms/hdrwizardform.ui \
        forms/tonemappingdialog.ui \
        forms/tonemappingoptions.ui \
        forms/documentation.ui \
        forms/about.ui \
        forms/options.ui \
        forms/transplantexifdialog.ui \
        forms/resizedialog.ui \
        forms/gamma_and_levels.ui \
        forms/projectionsDialog.ui \
        forms/editing_tools.ui \
        forms/batch_dialog.ui \ 
	forms/imageQualityDialog.ui

HEADERS += src/Libpfs/array2d.h \
           src/Libpfs/pfs.h \
           src/Common/global.h \
           src/Common/options.h \
           src/Common/smart_scroll_area.h \
           src/Common/panIconWidget.h \
           src/Common/gamma_and_levels.h \
           src/Common/gang.h \
           src/Common/commandline.h \
           src/Common/imageQualityDialog.h \
	   src/Common/selectionTool.h \
	   src/Common/genericViewer.h \
           src/MainWindow/mainWindow.h \
           src/MainWindow/DnDOption.h \
           src/MainWindow/hdrviewer.h \
           src/MainWindow/luminancerange_widget.h \
           src/MainWindow/histogram.h \
           src/ToneMappingDialog/tonemappingDialog.h \
           src/ToneMappingDialog/tonemapping_widget.h \
           src/ToneMappingDialog/ldrviewer.h \
           src/Threads/tonemapperThread.h \
           src/Threads/loadHdrThread.h \
           src/Threads/hdrInputLoader.h \
           src/Preferences/preferencesDialog.h \
           src/TransplantExif/transplant.h \
           src/Resize/resizeDialog.h \
           src/Projection/projectiveDialog.h \
           src/HdrCreation/HdrCreationManager.h \
           src/HdrCreation/mtb_alignment.h \
           src/HdrCreation/createhdr.h \
           src/HdrCreation/robertson02.h \
           src/HdrCreation/responses.h   \
           src/HdrCreation/debevec.h \
           src/HdrWizard/editingTools.h \
           src/HdrWizard/previewWidget.h \
           src/HdrWizard/newHdrWizard.h \
           src/Filter/pfspanoramic.h \
           src/Filter/pfscut.h \
           src/Fileformat/rgbeio.h \
           src/Fileformat/pfstiff.h \
           src/TM_operators/ashikhmin02/pyramid.h \
           src/TM_operators/ashikhmin02/tmo_ashikhmin02.h \
           src/TM_operators/drago03/tmo_drago03.h \
           src/TM_operators/durand02/tmo_durand02.h \
           src/TM_operators/durand02/bilateral.h \
           src/TM_operators/durand02/fastbilateral.h \
           src/TM_operators/fattal02/tmo_fattal02.h \
           src/TM_operators/fattal02/pde.h \
           src/TM_operators/pattanaik00/tmo_pattanaik00.h \
           src/TM_operators/reinhard02/tmo_reinhard02.h \
           src/TM_operators/mantiuk06/contrast_domain.h \
           src/Batch/batch_dialog.h \
           src/Exif/exif_operations.h

SOURCES += src/Libpfs/pfs.cpp \
           src/Libpfs/colorspace.cpp \
           src/Common/global.cpp \
           src/main.cpp \
           src/Common/options.cpp \
           src/Common/smart_scroll_area.cpp \
           src/Common/panIconWidget.cpp \
           src/Common/gamma_and_levels.cpp \
           src/Common/gang.cpp \
           src/Common/commandline.cpp \
           src/Common/imageQualityDialog.cpp \
	   src/Common/selectionTool.cpp \
	   src/Common/genericViewer.cpp \
           src/MainWindow/mainWindow.cpp \
           src/MainWindow/DnDOption.cpp \
           src/MainWindow/hdrviewer.cpp \
           src/MainWindow/luminancerange_widget.cpp \
           src/MainWindow/histogram.cpp \
           src/ToneMappingDialog/tonemappingDialog.cpp \
           src/ToneMappingDialog/tonemapping_widget.cpp \
           src/ToneMappingDialog/ldrviewer.cpp \
           src/Threads/tonemapperThread.cpp \
           src/Threads/loadHdrThread.cpp \
           src/Threads/hdrInputLoader.cpp \
           src/Preferences/preferencesDialog.cpp \
           src/TransplantExif/transplant.cpp \
           src/Resize/resizeDialog.cpp \
           src/Projection/projectiveDialog.cpp \
           src/HdrCreation/HdrCreationManager.cpp \
           src/HdrCreation/mtb_alignment.cpp \
           src/HdrCreation/createhdr.cpp \
           src/HdrCreation/robertson02.cpp  \
           src/HdrCreation/responses.cpp  \
           src/HdrCreation/debevec.cpp \
           src/HdrWizard/newHdrWizard.cpp \
           src/HdrWizard/editingTools.cpp \
           src/HdrWizard/previewWidget.cpp \
           src/Filter/pfsrotate.cpp \
           src/Filter/pfssize.cpp \
           src/Filter/pfsgamma.cpp \
           src/Filter/pfspanoramic.cpp \
           src/Filter/pfscut.cpp \
           src/Fileformat/pfsinrgbe.cpp \
           src/Fileformat/pfsoutrgbe.cpp \
           src/Fileformat/pfsoutldrimage.cpp \
           src/Fileformat/rgbeio.cpp \
           src/Fileformat/pfstiff.cpp \
           src/Fileformat/pfsinexr.cpp \
           src/Fileformat/pfsoutexr.cpp \
           src/TM_operators/ashikhmin02/pfstmo_ashikhmin02.cpp \
           src/TM_operators/ashikhmin02/tmo_ashikhmin02.cpp \
           src/TM_operators/drago03/pfstmo_drago03.cpp \
           src/TM_operators/drago03/tmo_drago03.cpp \
           src/TM_operators/durand02/pfstmo_durand02.cpp \
           src/TM_operators/durand02/tmo_durand02.cpp \
           src/TM_operators/durand02/bilateral.cpp \
           src/TM_operators/durand02/fastbilateral.cpp \
           src/TM_operators/fattal02/pfstmo_fattal02.cpp \
           src/TM_operators/fattal02/tmo_fattal02.cpp \
           src/TM_operators/fattal02/pde.cpp \
           src/TM_operators/reinhard05/pfstmo_reinhard05.cpp \
           src/TM_operators/pattanaik00/tmo_pattanaik00.cpp \
           src/TM_operators/pattanaik00/pfstmo_pattanaik00.cpp \
           src/TM_operators/reinhard02/pfstmo_reinhard02.cpp \
           src/TM_operators/reinhard02/tmo_reinhard02.cpp \
           src/TM_operators/reinhard02/approx.cpp \
           src/TM_operators/mantiuk06/contrast_domain.cpp \
           src/TM_operators/mantiuk06/pfstmo_mantiuk06.cpp \
           src/Batch/batch_dialog.cpp \
           src/Exif/exif_operations.cpp

RESOURCES = icons.qrc

TRANSLATIONS = i18n/lang_cs.ts \
               i18n/lang_de.ts \
               i18n/lang_es.ts \
               i18n/lang_fr.ts \
               i18n/lang_hu.ts \
               i18n/lang_id.ts \
               i18n/lang_it.ts \
               i18n/lang_pl.ts \
               i18n/lang_ru.ts \
               i18n/lang_tr.ts
               #i18n/lang_xx.ts
               

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

######################################## end of detection ########################################

############################## required by "make install" ########################################
isEmpty(PREFIX) {
        PREFIX = /usr/local
}
isEmpty(I18NDIR) {
	mac {
		#I18NDIR=(QCoreApplication::applicationDirPath()+\"/i18n\")
		#I18NDIR=/Applications/qtpfsgui.app/Contents/Resources/i18n
		#maybe we need parenthesis and/or backslashes somewhere.
		#I18NDIR=QCoreApplication::applicationDirPath()+"/../Resources/i18n"
		I18NDIR=QCoreApplication::applicationDirPath\\\(\\\)+"/../Resources/i18n"
	} else {
		I18NDIR = $${PREFIX}/share/qtpfsgui/i18n
	}
}
isEmpty(DOCDIR) {
	DOCDIR = $${PREFIX}/share/qtpfsgui
}
isEmpty(HTMLDIR) {
	HTMLDIR = $${DOCDIR}
}

target.path      = $${PREFIX}/bin
menu.files       = qtpfsgui.desktop
menu.path        = $${PREFIX}/share/applications
icon.files       = images/qtpfsgui.png
icon.path        = $${PREFIX}/share/icons/hicolor/32x32/apps
htmls.files      = html
htmls.path       = $$HTMLDIR
i18n.files       = i18n/lang_de.qm i18n/lang_es.qm i18n/lang_it.qm i18n/lang_fr.qm i18n/lang_pl.qm i18n/lang_tr.qm i18n/lang_ru.qm i18n/lang_cs.qm
i18n.path        = $$I18NDIR
docs.files       = README LICENSE AUTHORS INSTALL Changelog
docs.path        = $$DOCDIR

INSTALLS        += target menu icon htmls i18n docs
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
message ("qtpfsgui         ==> $$target.path")
message ("qtpfsgui.desktop ==> $$menu.path")
message ("qtpfsgui.png     ==> $$icon.path")
message ("docs             ==> $$docs.path")
message ("html             ==> $$htmls.path")
message ("i18n messages    ==> $$i18n.path")
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

}

macx {
ICON = images/qtpfsgui.icns

#TODO we have to complete this.
LIBS+=-lIlmThread

# Enable universal (requires a universal Qt)? Default = non-universal
# If you wish to build a Universal Binary please un-comment the following line
#CONFIG += x86 ppc

# Warn user what type of binary is being built and what the possible implications are
contains(CONFIG, "x86"):contains(CONFIG, "ppc") {
	message ("Building an OS X Universal Binary:")
	message ("Please ensure all dependencies and Qt are also Universal")
	message ("********************************************************************")
} else {
	# Test what architecture we are on (Intel or PPC)
	# 'arch' returns "i386" on Intel-Tiger is this true on Intel-Leopard?
	# What does 'arch' return on PPC machines? Presumably "ppc"?
	MAC_ARCH = $$system(arch)
	contains(MAC_ARCH, i386) {
		message ("This is an Intel Mac - Building an Intel specific OS X binary")
		message ("Please refer to the documentation if you require a Universal Binary")
		message ("********************************************************************")
		# Is this next line strictly necessary? gcc should compile for the correct architecture by default.
		CONFIG += x86
	} else {
		message ("This is a PPC Mac - Building a PPC specific OS X binary")
		message ("Please refer to the documentation if you require a Universal Binary")
		message ("********************************************************************")
		# Is this next line strictly necessary? gcc should compile for the correct architecture by default.
		CONFIG += ppc
	}
}

# We like to search the LOCALSOFT/lib explicitly on MacOSX
LIBS += -L$$(LOCALSOFT)/lib
# Libtiff depends on jpeg, but it is not searched for automatically on MacOSX
LIBS += -ljpeg
# Exiv also depend on libexpat and libiconv, so same as above:
LIBS += -lexpat 
LIBS += -liconv

# for now, we disable openMP on MacOSX - have to wait for support in next
# Xcode!
QMAKE_CXXFLAGS -= -fopenmp
}

win32 {

isEmpty(ENABLE_DEBUG) | contains(ENABLE_DEBUG, "no") {
message ("Debug statements DISABLED")
} else {
DEFINES -= QT_NO_DEBUG_OUTPUT
message ("Debug statements ENABLED")
}

# this is just how my MinGW installation is. You gotta change it if you want to compile it in windows.
CONFIG += windows
#CONFIG += debug
CONFIG += console

#OpenEXR available in win32
LIBS += -lIlmImf -lHalf -lIex -L../DEPs/lib/OpenEXR
INCLUDEPATH += ../DEPs/include/OpenEXR
# -lImath (no need to link against this)

#exiv2
INCLUDEPATH += ../DEPs/include/exiv2
LIBS        += -lexiv2 -L../DEPs/lib/exiv2

#win32-pthread, required by OpenMP (gcc-4.2.1-sjlj-2) (headers not required)
LIBS            += -L../DEPs/lib/pthread  -lpthreadGC2

#fftw3
LIBS += -L../DEPs/lib/fftw3 -lfftw3f-3 -lm
DEFINES += HAVE_FFTW
INCLUDEPATH += ../DEPs/include/fftw3

#tiff
INCLUDEPATH += ../DEPs/include/libtiff
LIBS += -L../DEPs/lib/libtiff -ltiff

DEFINES += I18NDIR=(QCoreApplication::applicationDirPath()+\\\"/i18n\\\")
RC_FILE = images/qtpfsgui_ico.rc

}


