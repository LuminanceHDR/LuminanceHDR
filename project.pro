TEMPLATE = app
CONFIG += release qt thread

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
       PREFIX = ""
}
target.path    = $${PREFIX}/usr/local/bin
menu.files      = qtpfsgui.desktop
menu.path       = $${PREFIX}/usr/share/applications
icon.files      = images/qtpfsgui.png
icon.path       = $${prefix}/usr/share/pixmaps
INSTALLS	+= target menu icon
#################################################################################################
#CONFIG += debug
}

macx {
#TODO we have to complete this.
LIBS+=-lIlmThread

# Enable universal (require a universal Qt)
CONFIG += x86 ppc
# Add some extra PATHS in LIBS:
LIBS += -L$$(LOCALSOFT)/lib
}

win32 {
# this is just how my MinGW installation is. You gotta change it, if you want to compile it in windows.
CONFIG += windows
#CONFIG += debug
#CONFIG += console

#OpenEXR not available yet in win32
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

#tiff
INCLUDEPATH += C:\Programmi\GnuWin32\include
LIBS += -LC:\Programmi\GnuWin32\lib -llibtiff

RC_FILE = src/qtpfsgui_ico.rc
}

#INCLUDEPATH += src/libpfs src/fileformat src/hdrcreate src/ashikhmin02 src/drago03 src/durand02 src/fattal02 src/pattanaik00 src/reinhard02

FORMS = forms/maingui.ui \
        forms/hdrwizardform.ui \
        forms/tonemappingdialog.ui \
        forms/help_about.ui \
        forms/options.ui \
        forms/transplantexifdialog.ui \
        forms/resizedialog.ui

HEADERS += src/libpfs/array2d.h \
           src/libpfs/pfs.h \
           src/maingui_impl.h \
           src/hdrwizardform_impl.h \
           src/tonemappingdialog_impl.h \
           src/options_impl.h \
           src/transplant_impl.h \
           src/resizedialog_impl.h \
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
           src/fileformat/pfstiff.h \
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
           src/transplant_impl.cpp \
           src/resizedialog_impl.cpp \
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
           src/fileformat/pfstiff.cpp \
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

RESOURCES = icons.qrc

# Old durand, we use the fftw version now.
#src/durand02/bilateral.h \
#src/durand02/bilateral.cpp \

# Manual align dialog. we should call Pablo's application now.
#           src/align_impl.cpp \
#           src/show_image.cpp \
#           src/align_impl.h \
#           src/show_image.h \
#        forms/aligndialog.ui \

unix {
SOURCES += src/fileformat/pfsinexr.cpp \
           src/fileformat/pfsoutexr.cpp
}

TARGET = qtpfsgui

MOC_DIR = generated_moc
OBJECTS_DIR = generated_obj
UI_DIR = generated_uic
RCC_DIR = generated_moc
