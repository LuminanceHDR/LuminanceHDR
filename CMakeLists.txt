PROJECT(luminance-hdr)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# find and setup Qt4 for this project
FIND_PACKAGE(Qt4 REQUIRED COMPONENTS QtCore QtGui QtXml QtWebkit QtSql REQUIRED)

# add Qt modules here, the include will setup QT_LIBRARIES
SET(QT_USE_QTXML TRUE)
SET(QT_USE_QTWEBKIT TRUE)
SET(QT_USE_QTSQL TRUE)

# Include the qtmain lib if we are on the windows platform
IF(WIN32)
#    LINK_LIBRARIES(${QT_QTMAIN_LIBRARY})
    SET(LIBS ${LIBS} ${QT_QTMAIN_LIBRARY})
ENDIF(WIN32)

INCLUDE(${QT_USE_FILE})

set(LIBS ${LIBS} ${QT_QTCORE_LIBRARIES} ${QT_QTGUI_LIBRARIES} ${QT_QTWEBKIT_LIBRARIES} ${QT_QTXML_LIBRARIES} ${QT_QTSQL_LIBRARIES})

FIND_PACKAGE(Git QUIET)
IF(NOT GIT_FOUND)
    FIND_FILE(GIT_EXECUTABLE git.exe 
        $ENV{CYGWIN_DIR}\\bin
        $ENV{CYGWIN_DIR}
    )
    IF (EXISTS ${GIT_EXECUTABLE})
        SET(GIT_FOUND 1)
    ENDIF()
ENDIF()

IF(GIT_FOUND)
    include(GetGitRevisionDescription)
    get_git_head_revision(GIT_REFSPEC GIT_SHA1)
ENDIF()

IF(WIN32)
# Windows Section
	SET(OPENEXR_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/OpenEXR")
	SET(TIFF_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/libtiff")
	SET(LIBRAW_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/libraw")
	SET(FFTWF_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/fftw3")
	SET(GSL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/gsl")
	SET(GSLCBLAS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/gsl")
	SET(EXIV2_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/exiv2")
		
	IF(MSVC)
		# Enable multi processor compilation
		ADD_DEFINITIONS(/MP)

		# Full optimization
		ADD_DEFINITIONS(/Ox)
		
		# Enable OpenMP
		ADD_DEFINITIONS(/openmp)

		# Enable SSE2
		ADD_DEFINITIONS(/arch:SSE2)
		
		ADD_DEFINITIONS(-DUNICODE )
		ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -DOPENEXR_DLL -D_REENTRANT)
		SET(FFTWF_LIBRARIES "${CMAKE_SOURCE_DIR}/../DEPs/lib/fftw3/libfftw3f-3.lib")
		ADD_DEFINITIONS(-DHAVE_FFTW3F)
		
		SET(OPENEXR_LIBRARIES "${CMAKE_SOURCE_DIR}/../DEPs/lib/OpenEXR/Half.lib")
		SET(OPENEXR_LIBRARIES ${OPENEXR_LIBRARIES} "${CMAKE_SOURCE_DIR}/../DEPs/lib/OpenEXR/Iex.lib")
		SET(OPENEXR_LIBRARIES ${OPENEXR_LIBRARIES} "${CMAKE_SOURCE_DIR}/../DEPs/lib/OpenEXR/IlmImf.lib")
		
		SET(TIFF_LIBRARIES "${CMAKE_SOURCE_DIR}/../DEPs/lib/libtiff/libtiff.lib")
		
		SET(LIBRAW_LIBRARIES "${CMAKE_SOURCE_DIR}/../DEPs/lib/libraw/libraw.lib")
		
		SET(GSL_LIBRARIES "${CMAKE_SOURCE_DIR}/../DEPs/lib/gsl/gsl.lib")
		SET(GSLCBLAS_LIBRARIES "${CMAKE_SOURCE_DIR}/../DEPs/lib/gsl/cblas.lib")
		
		SET(EXIV2_LIBRARIES "${CMAKE_SOURCE_DIR}/../DEPs/lib/exiv2/exiv2.lib")
	ELSE()
		SET(OPENEXR_LIBRARIES "-lIlmImf.dll -lHalf.dll -lIex.dll -L${CMAKE_SOURCE_DIR}/../DEPs/lib/OpenEXR")

		SET(TIFF_LIBRARIES "-L../DEPs/lib/libtiff -ltiff")

		SET(LIBRAW_LIBRARIES "-L../DEPs/lib/libraw -lraw_r -lws2_32 -lm")
		ADD_DEFINITIONS(-DLIBRAW_NODLL -D_REENTRANT)

		SET(FFTWF_LIBRARIES "-L${CMAKE_SOURCE_DIR}/../DEPs/lib/fftw3 -lfftw3f-3 -lm")

		SET(GSL_LIBRARIES "-L${CMAKE_SOURCE_DIR}/../DEPs/lib/gsl -lgsl")
		SET(GSLCBLAS_LIBRARIES "-L${CMAKE_SOURCE_DIR}/../DEPs/lib/gsl -lgslcblas")

		SET(EXIV2_LIBRARIES "-lexiv2 -L${CMAKE_SOURCE_DIR}/../DEPs/lib/exiv2")
	ENDIF()
ELSE()

	# Unix & Apple part
	FIND_PACKAGE(OpenEXR REQUIRED)
	FIND_PACKAGE(TIFF REQUIRED)
	FIND_PACKAGE(LibRAW REQUIRED)
	FIND_PACKAGE(fftwf REQUIRED)
	FIND_PACKAGE(GSL REQUIRED)
	FIND_PACKAGE(exiv2 REQUIRED)

	ADD_DEFINITIONS(-O3)

ENDIF(WIN32)


SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${OPENEXR_INCLUDE_DIR} "${OPENEXR_INCLUDE_DIR}/OpenEXR")
SET(LIBS ${LIBS} ${OPENEXR_LIBRARIES})

SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${TIFF_INCLUDE_DIR})
SET(LIBS ${LIBS} ${TIFF_LIBRARIES})

SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${LIBRAW_INCLUDE_DIR} "${LIBRAW_INCLUDE_DIR}/libraw")
SET(LIBS ${LIBS} ${LIBRAW_LIBRARIES})

SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${FFTWF_INCLUDE_DIR})
SET(LIBS ${LIBS} ${FFTWF_LIBRARIES})
ADD_DEFINITIONS(-DHAVE_FFTW3F)

SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${GSL_INCLUDE_DIR} ${GSLCBLAS_INCLUDE_DIR})
SET(LIBS ${LIBS} ${GSL_LIBRARIES} ${GSLCBLAS_LIBRARIES})

SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${EXIV2_INCLUDE_DIR} "${EXIV2_INCLUDE_DIR}/exiv2")
SET(LIBS ${LIBS} ${EXIV2_LIBRARIES})

#OpenMP Support under Linux, Windows with MSVC & MacOS X with GCC >= 4.3
IF(MSVC)

	FIND_PACKAGE(OpenMP REQUIRED)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")

ELSEIF(CMAKE_COMPILER_IS_GNUCC AND UNIX)

	execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
					OUTPUT_VARIABLE GCC_VERSION)
	#MESSAGE(STATUS ${GCC_VERSION})
	IF(GCC_VERSION VERSION_GREATER 4.3 OR GCC_VERSION VERSION_EQUAL 4.3)
		message(STATUS "GCC >= 4.3")
		FIND_PACKAGE(OpenMP REQUIRED)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	endif()

ENDIF()
# END OpenMP Support

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/src/" ${INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SET(LIB_HDR_INCLUDE
	src/Common/msec_timer.h
	src/Libpfs/vex.h
	src/Libpfs/array2d.h
	src/Libpfs/channel.h
	src/Libpfs/colorspace.h
	src/Libpfs/domio.h
	src/Libpfs/frame.h
	src/Libpfs/pfs.h
	src/Libpfs/tag.h
# PFS FILTER	
	src/Filter/pfscut.h
	src/Filter/pfsgamma.h
	src/Filter/pfspanoramic.h
	src/Filter/pfsrotate.h
        src/Filter/pfssize.h
        src/Filter/pfsgammaandlevels.h
# PFS I/O
	src/Fileformat/pfs_file_format.h
	src/Fileformat/rgbeio.h
	src/Fileformat/pfsinraw.h
	src/Fileformat/pfsoutldrimage.h
	src/Fileformat/pfsout16bitspixmap.h
# PFS TMO
	src/TonemappingOperators/pfstmo.h
	src/TonemappingOperators/pfstmdefaultparams.h
	src/TonemappingOperators/ashikhmin02/pyramid.h
	src/TonemappingOperators/ashikhmin02/tmo_ashikhmin02.h
	src/TonemappingOperators/drago03/tmo_drago03.h
	src/TonemappingOperators/durand02/bilateral.h
	src/TonemappingOperators/durand02/fastbilateral.h
	src/TonemappingOperators/durand02/tmo_durand02.h
	src/TonemappingOperators/fattal02/pde.h
	src/TonemappingOperators/fattal02/tmo_fattal02.h
	src/TonemappingOperators/mantiuk06/contrast_domain.h
	src/TonemappingOperators/mantiuk08/display_adaptive_tmo.h
	src/TonemappingOperators/mantiuk08/display_function.h
	src/TonemappingOperators/mantiuk08/display_size.h
	src/TonemappingOperators/mantiuk08/cqp/gsl_cqp.h
	src/TonemappingOperators/mantiuk08/cqp/initial_point.h
	src/TonemappingOperators/pattanaik00/tmo_pattanaik00.h	
	src/TonemappingOperators/reinhard02/tmo_reinhard02.h
	src/TonemappingOperators/reinhard05/tmo_reinhard05.h
# CREATE HDR
	src/HdrCreation/createhdr.h
	src/HdrCreation/debevec.h
	src/HdrCreation/generic_applyResponse.h
#	src/HdrCreation/icip06.h
	src/HdrCreation/responses.h
	src/HdrCreation/robertson02.h
	src/HdrCreation/mtb_alignment.h	
)

IF(MSVC)
	SET(LIB_HDR_INCLUDE ${LIB_HDR_INCLUDE}
		src/arch/msvc/getopt_win32.h
	)
ENDIF()

SET(LIB_HDR_SRC
	src/Common/msec_timer.cpp
        src/Libpfs/vex.cpp
	src/Libpfs/array2d.cpp
	src/Libpfs/channel.cpp
	src/Libpfs/colorspace.cpp
	src/Libpfs/domio.cpp
	src/Libpfs/frame.cpp
#	src/Libpfs/pfs.cpp
	src/Libpfs/tag.cpp
# PFS FILTER	
#	src/Filter/pfsclamp.cpp
	src/Filter/pfscut.cpp
#	src/Filter/pfsflip.cpp
	src/Filter/pfsgamma.cpp
	src/Filter/pfspanoramic.cpp
	src/Filter/pfsrotate.cpp
	src/Filter/pfssize.cpp
        src/Filter/pfsgammaandlevels.cpp
# PFS I/O
	src/Fileformat/pfstiff.cpp
	src/Fileformat/rgbeio.cpp
	src/Fileformat/pfsinrgbe.cpp
	src/Fileformat/pfsoutrgbe.cpp
	src/Fileformat/pfsinexr.cpp
	src/Fileformat/pfsoutexr.cpp
	src/Fileformat/pfsinraw.cpp
	src/Fileformat/pfsoutldrimage.cpp
	src/Fileformat/pfsout16bitspixmap.cpp
# PFS TMO
	src/TonemappingOperators/ashikhmin02/tmo_ashikhmin02.cpp
	src/TonemappingOperators/ashikhmin02/pfstmo_ashikhmin02.cpp
	src/TonemappingOperators/drago03/tmo_drago03.cpp
	src/TonemappingOperators/drago03/pfstmo_drago03.cpp
	src/TonemappingOperators/durand02/bilateral.cpp
	src/TonemappingOperators/durand02/fastbilateral.cpp
	src/TonemappingOperators/durand02/pfstmo_durand02.cpp
	src/TonemappingOperators/durand02/tmo_durand02.cpp
	src/TonemappingOperators/fattal02/pde.cpp
	src/TonemappingOperators/fattal02/tmo_fattal02.cpp
	src/TonemappingOperators/fattal02/pfstmo_fattal02.cpp
	src/TonemappingOperators/mantiuk06/contrast_domain.cpp
	src/TonemappingOperators/mantiuk06/pfstmo_mantiuk06.cpp
	src/TonemappingOperators/mantiuk08/display_adaptive_tmo.cpp
	src/TonemappingOperators/mantiuk08/display_function.cpp
	src/TonemappingOperators/mantiuk08/display_size.cpp
	src/TonemappingOperators/mantiuk08/cqp/initial_point.c
	src/TonemappingOperators/mantiuk08/cqp/cqpminimizer.c
	src/TonemappingOperators/mantiuk08/cqp/mg_pdip.c
	src/TonemappingOperators/mantiuk08/pfstmo_mantiuk08.cpp	
	src/TonemappingOperators/pattanaik00/tmo_pattanaik00.cpp
	src/TonemappingOperators/pattanaik00/pfstmo_pattanaik00.cpp
	src/TonemappingOperators/reinhard02/tmo_reinhard02.cpp
	src/TonemappingOperators/reinhard02/pfstmo_reinhard02.cpp
	src/TonemappingOperators/reinhard02/approx.cpp
	src/TonemappingOperators/reinhard05/tmo_reinhard05.cpp
	src/TonemappingOperators/reinhard05/pfstmo_reinhard05.cpp
# CREATE HDR
	src/HdrCreation/createhdr.cpp
	src/HdrCreation/debevec.cpp
	src/HdrCreation/generic_applyResponse.cpp
#	src/HdrCreation/icip06.cpp
	src/HdrCreation/responses.cpp
	src/HdrCreation/robertson02.cpp	
	src/HdrCreation/mtb_alignment.cpp
)
IF(MSVC)
	SET(LIB_HDR_INCLUDE ${LIB_HDR_INCLUDE}
		src/arch/msvc/getopt_win32.c
	)
ENDIF()

SET(LIB_HDR_MOC
# PFS I/O
	src/Fileformat/pfstiff.h	
)

SET(LUMINANCE_HDR_INCLUDE
        src/Core/TonemappingOptions.h
	src/arch/math.h
	src/arch/malloc.h
	src/arch/minmax.h
	src/arch/string.h
	src/arch/freebsd/math.h
	src/arch/msvc/math.h
	src/Common/archs.h
	src/Exif/ExifOperations.h
	src/HelpBrowser/LuminancePaths.h	
	src/HelpBrowser/schelptreemodel.h	
	src/HelpBrowser/treeitem.h
	src/HelpBrowser/treemodel.h
	src/UI/UMessageBox.h
	src/Viewers/Histogram.h
	src/Common/global.h
	src/Common/GitSHA1.h
    src/Common/config.h

# new viewer
        src/Viewers/ISelectionBox.h
        src/Viewers/ISelectionAnchor.h
# end new viewer

        src/TonemappingEngine/TonemapOperatorAshikhmin02.h
        src/TonemappingEngine/TonemapOperatorDrago03.h
        src/TonemappingEngine/TonemapOperatorDurand02.h
        src/TonemappingEngine/TonemapOperatorFattal02.h
        src/TonemappingEngine/TonemapOperatorMantiuk06.h
        src/TonemappingEngine/TonemapOperatorMantiuk08.h
        src/TonemappingEngine/TonemapOperatorPattanaik00.h
        src/TonemappingEngine/TonemapOperatorReinhard02.h
        src/TonemappingEngine/TonemapOperatorReinhard05.h
        src/TonemappingEngine/TonemapOperator.h
)

SET(LUMINANCE_HDR_SRC
	src/BatchHDR/BatchHDRDialog.cpp
        src/BatchTM/BatchTMDialog.cpp
        src/BatchTM/BatchTMJob.cpp
	src/Core/IOWorker.cpp
        src/Core/TMWorker.cpp
        src/Core/TonemappingOptions.cpp
	src/Exif/ExifOperations.cpp
	src/HdrCreation/HdrCreationManager.cpp
	src/HdrWizard/EditingTools.cpp
	src/HdrWizard/HdrWizard.cpp
	src/HdrWizard/PreviewWidget.cpp
	src/HelpBrowser/HelpSideBar.cpp
	src/HelpBrowser/LuminancePaths.cpp
	src/HelpBrowser/helpbrowser.cpp
	src/HelpBrowser/schelptreemodel.cpp	
	src/HelpBrowser/sctextbrowser.cpp
	src/HelpBrowser/treeitem.cpp
	src/HelpBrowser/treemodel.cpp
	src/MainWindow/DnDOption.cpp
	src/MainWindow/MainWindow.cpp
	src/Preferences/PreferencesDialog.cpp
	src/PreviewPanel/PreviewLabel.cpp
	src/PreviewPanel/PreviewPanel.cpp
	src/Projection/ProjectionsDialog.cpp
	src/Resize/ResizeDialog.cpp

	src/Threads/HdrInputLoader.cpp

	src/TonemappingPanel/TMOProgressIndicator.cpp
	src/TonemappingPanel/TonemappingPanel.cpp
	src/TonemappingPanel/TonemappingWarnDialog.cpp
	src/TonemappingPanel/SavedParametersDialog.cpp
	src/TonemappingPanel/SavingParametersDialog.cpp
	src/TransplantExif/TransplantExifDialog.cpp
	src/UI/UMessageBox.cpp
	src/Viewers/GenericViewer.cpp
	src/Viewers/HdrViewer.cpp
	src/Viewers/LdrViewer.cpp
	src/Viewers/Histogram.cpp
	src/Viewers/LuminanceRangeWidget.cpp
	src/Viewers/SmartScrollArea.cpp
        src/Viewers/SelectionTool.cpp
# new viewer
        src/Viewers/IGraphicsView.cpp
        src/Viewers/IGraphicsPixmapItem.cpp
        src/Viewers/ISelectionBox.cpp
        src/Viewers/ISelectionAnchor.cpp
# end new viewer
	src/Common/GammaAndLevels.cpp
	src/Common/Gang.cpp	
	src/Common/ImageQualityDialog.cpp
	src/Common/PanIconWidget.cpp
        src/Common/ProgressHelper.cpp
	src/Common/commandline.cpp	
	src/Common/global.cpp
        src/Common/LuminanceOptions.cpp

        src/TonemappingEngine/TonemapOperatorAshikhmin02.cpp
        src/TonemappingEngine/TonemapOperatorDrago03.cpp
        src/TonemappingEngine/TonemapOperatorDurand02.cpp
        src/TonemappingEngine/TonemapOperatorFattal02.cpp
        src/TonemappingEngine/TonemapOperatorMantiuk06.cpp
        src/TonemappingEngine/TonemapOperatorMantiuk08.cpp
        src/TonemappingEngine/TonemapOperatorPattanaik00.cpp
        src/TonemappingEngine/TonemapOperatorReinhard02.cpp
        src/TonemappingEngine/TonemapOperatorReinhard05.cpp
        src/TonemappingEngine/TonemapOperator.cpp

	src/main.cpp
	${CMAKE_CURRENT_BINARY_DIR}/src/Common/GitSHA1.cpp     # for actual git version
)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/src/Common/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/src/Common/GitSHA1.cpp" @ONLY)

SET(LUMINANCE_HDR_MOC
	src/BatchHDR/BatchHDRDialog.h
        src/BatchTM/BatchTMDialog.h
        src/BatchTM/BatchTMJob.h
	src/Core/IOWorker.h
        src/Core/TMWorker.h
	src/HdrCreation/HdrCreationManager.h
	src/HdrWizard/EditingTools.h
	src/HdrWizard/HdrWizard.h
	src/HdrWizard/PreviewWidget.h
	src/HelpBrowser/HelpSideBar.h
	src/HelpBrowser/helpbrowser.h	
	src/HelpBrowser/sctextbrowser.h
	src/MainWindow/DnDOption.h
	src/MainWindow/MainWindow.h
	src/Preferences/PreferencesDialog.h
	src/PreviewPanel/PreviewLabel.h
	src/PreviewPanel/PreviewPanel.h
	src/Projection/ProjectionsDialog.h
	src/Resize/ResizeDialog.h
	src/Threads/HdrInputLoader.h
	src/TonemappingPanel/TMOProgressIndicator.h
	src/TonemappingPanel/TonemappingPanel.h
	src/TonemappingPanel/TonemappingWarnDialog.h
	src/TonemappingPanel/SavedParametersDialog.h
	src/TonemappingPanel/SavingParametersDialog.h
	src/TransplantExif/TransplantExifDialog.h
	src/Viewers/GenericViewer.h
	src/Viewers/HdrViewer.h
	src/Viewers/LdrViewer.h
	src/Viewers/LuminanceRangeWidget.h
	src/Viewers/SmartScrollArea.h
        src/Viewers/SelectionTool.h
# new viewer
        src/Viewers/IGraphicsView.h
        src/Viewers/IGraphicsPixmapItem.h
# end new viewer
        src/Common/LuminanceOptions.h
	src/Common/GammaAndLevels.h
	src/Common/Gang.h
	src/Common/ImageQualityDialog.h
	src/Common/PanIconWidget.h
        src/Common/ProgressHelper.h
	src/Common/commandline.h
)

SET(LUMINANCE_HDR_UI
	forms/BatchHDRDialog.ui
	forms/BatchTMDialog.ui
	forms/DnDOption.ui
	forms/EditingTools.ui
	forms/GammaAndLevels.ui
	forms/HdrWizard.ui
	forms/HelpBrowser.ui
	forms/HelpSideBar.ui
	forms/ImageQualityDialog.ui
	forms/MainWindow.ui
	forms/PreferencesDialog.ui
	forms/PreviewPanel.ui
	forms/ProjectionsDialog.ui
	forms/ResizeDialog.ui
	forms/Splash.ui
	forms/TonemappingPanel.ui
	forms/TonemappingWarnDialog.ui
	forms/TransplantExifDialog.ui
	forms/about.ui
	forms/SavedParametersDialog.ui
	forms/SavingParametersDialog.ui
)

SET(LUMINANCE_TS
        i18n/lang_cs.ts
        i18n/lang_de.ts
        i18n/lang_es.ts
        i18n/lang_fr.ts
        i18n/lang_fi.ts
        i18n/lang_hu.ts
        i18n/lang_id.ts
        i18n/lang_it.ts
        i18n/lang_pl.ts
        i18n/lang_ro.ts
        i18n/lang_ru.ts
        i18n/lang_tr.ts
)

SET(LUMINANCE_HDR_RCCS icons.qrc)

QT4_WRAP_CPP(LIB_HDR_MOC_SRC ${LIB_HDR_MOC})
QT4_WRAP_CPP(LUMINANCE_HDR_MOC_SRC ${LUMINANCE_HDR_MOC})

QT4_ADD_RESOURCES(LUMINANCE_HDR_RCCS_SRCS ${LUMINANCE_HDR_RCCS})

QT4_WRAP_UI(LUMINANCE_HDR_UI_HEADERS ${LUMINANCE_HDR_UI})

OPTION(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files (WARNING: make clean will delete the source .ts files! Danger!)")
IF(UPDATE_TRANSLATIONS)
    SET(LUMINANCE_TO_TRANSLATE
    ${LIB_HDR_INCLUDE}
    ${LIB_HDR_SRC}
    ${LIB_HDR_MOC}
    ${LUMINANCE_HDR_INCLUDE}
    ${LUMINANCE_HDR_MOC}
    ${LUMINANCE_HDR_SRC}
    ${LUMINANCE_HDR_UI})
    QT4_CREATE_TRANSLATION(LUMINANCE_QM ${LUMINANCE_TO_TRANSLATE} ${LUMINANCE_TS})
ELSE(UPDATE_TRANSLATIONS)
    QT4_ADD_TRANSLATION(LUMINANCE_QM ${LUMINANCE_TS})

SET(LUMINANCE_FILES
         ${LIB_HDR_INCLUDE}
         ${LIB_HDR_SRC}
         ${LIB_HDR_MOC_SRC}
         ${LUMINANCE_HDR_UI_HEADERS}
         ${LUMINANCE_HDR_INCLUDE}
         ${LUMINANCE_HDR_SRC}
         ${LUMINANCE_HDR_MOC_SRC}
         ${LUMINANCE_HDR_RCCS_SRCS}
         ${LUMINANCE_QM}
)

IF(APPLE)
    # icon files to copy in the bundle
    #SET( OSX_ICON_FILES "${CMAKE_SOURCE_DIR}/images/luminance.icns")
    # set where in the bundle to put the icns files
    SET_SOURCE_FILES_PROPERTIES(${OSX_ICON_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    # set where in the bundle to put the translation files
    SET_SOURCE_FILES_PROPERTIES(${LUMINANCE_QM} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/i18n)
    # include the icns files in the target
    SET( LUMINANCE_FILES ${LUMINANCE_FILES} ) #${OSX_ICON_FILES} )

    SET( PROGNAME luminance-hdr )
    SET( MACOSX_BUNDLE_ICON_FILE "${CMAKE_SOURCE_DIR}/images/luminance.icns" )
    #SET( MACOSX_BUNDLE_SHORT_VERSION_STRING 0.7-beta2 )
    #SET( MACOSX_BUNDLE_VERSION 0.7-beta2 )
    #SET( MACOSX_BUNDLE_LONG_VERSION_STRING Version 0.7-beta2 )
    #SET( CMAKE_OSX_ARCHITECTURES x86_64;i386 )  # Universal build

    ADD_DEFINITIONS(-DI18NDIR=QCoreApplication::applicationDirPath\(\)+"/../Resources/i18n")
    ADD_EXECUTABLE(luminance-hdr MACOSX_BUNDLE ${LUMINANCE_FILES})
  
    SET_TARGET_PROPERTIES(luminance-hdr PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/build/macosx/Info.plist")

ELSEIF(UNIX)

    #ADD_DEFINITIONS(-DPREFIX=${CMAKE_INSTALL_PREFIX})
    ADD_DEFINITIONS(-DI18NDIR="${CMAKE_INSTALL_PREFIX}/share/luminance-hdr/i18n")
    ADD_EXECUTABLE(luminance-hdr ${LUMINANCE_FILES})

    # make install directivies
    INSTALL(TARGETS luminance-hdr RUNTIME DESTINATION bin) # main exec
    INSTALL(FILES ${LUMINANCE_QM} DESTINATION share/luminance/i18n) # i18n
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/images/luminance-hdr.png DESTINATION share/icons/hicolor/48x48/apps) # icon
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/luminance-hdr.desktop DESTINATION share/applications) #desktop entry
    INSTALL(FILES
    ${CMAKE_SOURCE_DIR}/AUTHORS
    ${CMAKE_SOURCE_DIR}/README
    ${CMAKE_SOURCE_DIR}/LICENSE
    ${CMAKE_SOURCE_DIR}/Changelog
    DESTINATION share/luminance) #info files
    INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/help DESTINATION share/luminance) # help directory

ELSEIF(WIN32)

    # icon files to copy in the bundle
    SET(LUMINANCE_FILES ${LUMINANCE_FILES} "${CMAKE_SOURCE_DIR}/images/luminance_ico.rc")

    ADD_DEFINITIONS(-DI18NDIR=QCoreApplication::applicationDirPath\(\)+"/i18n")

    # Subsystem console:
    ADD_EXECUTABLE(luminance-hdr ${LUMINANCE_FILES})
  
    # copies all necessary Qt dlls from the Qt binary dir to the output folder
    SET(QT_RELEASE_DLLS
        phonon4.dll
        QtCore4.dll
        QtGui4.dll
        QtMultimedia4.dll
        QtNetwork4.dll
        QtSql4.dll
        QtWebKit4.dll
        QtXml4.dll
        QtXmlPatterns4.dll
    )
    FOREACH(dll ${QT_RELEASE_DLLS})
        FILE(COPY "${QT_BINARY_DIR}/${dll}" DESTINATION "${CMAKE_BINARY_DIR}/QtDlls/Release")
        STRING(REPLACE "4" "d4" debugdll ${dll})
        FILE(COPY "${QT_BINARY_DIR}/${debugdll}" DESTINATION "${CMAKE_BINARY_DIR}/QtDlls/Debug")
    ENDFOREACH()

    FILE(COPY "${QT_PLUGINS_DIR}/imageformats/qjpeg4.dll" DESTINATION "${CMAKE_BINARY_DIR}/QtDlls/Release/imageformats")
    FILE(COPY "${QT_PLUGINS_DIR}/imageformats/qjpegd4.dll" DESTINATION "${CMAKE_BINARY_DIR}/QtDlls/Debug/imageformats")
    FILE(COPY "${QT_PLUGINS_DIR}/sqldrivers/qsqlite4.dll" DESTINATION "${CMAKE_BINARY_DIR}/QtDlls/Release/sqldrivers")
    FILE(COPY "${QT_PLUGINS_DIR}/sqldrivers/qsqlited4.dll" DESTINATION "${CMAKE_BINARY_DIR}/QtDlls/Debug/sqldrivers")

ENDIF()

TARGET_LINK_LIBRARIES(luminance-hdr ${LIBS})

ENDIF(UPDATE_TRANSLATIONS)
