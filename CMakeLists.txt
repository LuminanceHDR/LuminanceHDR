PROJECT(luminance-hdr)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# find and setup Qt4 for this project
FIND_PACKAGE(Qt4 REQUIRED)

# add Qt modules here, the include will setup QT_LIBRARIES
SET(QT_USE_QTXML TRUE)
SET(QT_USE_QTWEBKIT TRUE)

INCLUDE(${QT_USE_FILE})

set(LIBS ${LIBS} ${QT_QTCORE_LIBRARIES} ${QT_QTGUI_LIBRARIES} ${QT_QTWEBKIT_LIBRARIES} ${QT_QTXML_LIBRARIES})

IF(WIN32)
  set(LIBS ${LIBS} ${QT_JPEG_PLUGIN})
ENDIF()

FIND_PACKAGE(PkgConfig REQUIRED)

FIND_PACKAGE(OpenEXR REQUIRED)
SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${OPENEXR_INCLUDE_DIR} "${OPENEXR_INCLUDE_DIR}/OpenEXR")
set(LIBS ${LIBS} ${OPENEXR_LIBRARIES})

FIND_PACKAGE(TIFF REQUIRED)
SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${TIFF_INCLUDE_DIR})
set(LIBS ${LIBS} ${TIFF_LIBRARIES})

FIND_PACKAGE(LibRAW REQUIRED)
SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${LIBRAW_INCLUDE_DIR} "${LIBRAW_INCLUDE_DIR}/libraw")
set(LIBS ${LIBS} ${LIBRAW_LIBRARIES})

FIND_PACKAGE(fftwf REQUIRED)
SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${FFTWF_INCLUDE_DIR})
set(LIBS ${LIBS} ${FFTWF_LIBRARIES})
add_definitions(-DHAVE_FFTW3F)

FIND_PACKAGE(GSL REQUIRED)
SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${GSL_INCLUDE_DIR} ${GSLCBLAS_INCLUDE_DIR})
set(LIBS ${LIBS} ${GSL_LIBRARIES} ${GSLCBLAS_LIBRARIES})

FIND_PACKAGE(exiv2 REQUIRED)
SET(INCLUDE_DIRS ${INCLUDE_DIRS} ${EXIV2_INCLUDE_DIR} "${EXIV2_INCLUDE_DIR}/exiv2")
set(LIBS ${LIBS} ${EXIV2_LIBRARIES})

#OpenMP support under Linux (on Windows & MacOS X soon)
IF(UNIX)
IF(NOT APPLE)
FIND_PACKAGE(OpenMP REQUIRED)
SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS})
SET(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS})
ENDIF()
ENDIF()


INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/src/" ${INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

IF(WIN32)
#  FIND_LIBRARY(QT_GIF_PLUGIN qgif PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats)
  FIND_LIBRARY(QT_JPEG_PLUGIN qjpeg PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats)
ENDIF()

SET(LIB_HDR_INCLUDE
	src/Common/msec_timer.h
	src/Common/vex.h
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
# PFS I/O
	src/Fileformat/pfs_file_format.h
	src/Fileformat/rgbeio.h
	src/Fileformat/pfsinraw.h
	src/Fileformat/pfsoutldrimage.h
# PFS TMO
	src/TonemappingOperators/pfstmo.h
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

SET(LIB_HDR_SRC
	src/Common/msec_timer.cpp
	src/Common/vex.cpp
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
# PFS I/O
	src/Fileformat/pfstiff.cpp
	src/Fileformat/rgbeio.cpp
	src/Fileformat/pfsinrgbe.cpp
	src/Fileformat/pfsoutrgbe.cpp
	src/Fileformat/pfsinexr.cpp
	src/Fileformat/pfsoutexr.cpp
	src/Fileformat/pfsinraw.cpp
	src/Fileformat/pfsoutldrimage.cpp
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

SET(LIB_HDR_MOC
# PFS I/O
	src/Fileformat/pfstiff.h	
)

SET(LUMINANCE_HDR_INCLUDE
	src/arch/freebsd/math.h
	src/Common/archs.h
	src/Exif/ExifOperations.h
	src/HelpBrowser/LuminancePaths.h	
	src/HelpBrowser/schelptreemodel.h	
	src/HelpBrowser/treeitem.h
	src/HelpBrowser/treemodel.h
	src/Threads/TMOFactory.h
	src/UI/UMessageBox.h
	src/Viewers/Histogram.h
	src/Common/global.h
        src/Common/options.h
        src/Common/config.h
)

SET(LUMINANCE_HDR_SRC
	src/Batch/BatchTMDialog.cpp
	src/Batch/BatchTMJob.cpp
	src/Core/IOWorker.cpp
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
	src/Projection/ProjectionsDialog.cpp
	src/Resize/ResizeDialog.cpp
	src/Threads/Ashikhmin02Thread.cpp
	src/Threads/Drago03Thread.cpp
	src/Threads/Durand02Thread.cpp
	src/Threads/Fattal02Thread.cpp
	src/Threads/HdrInputLoader.cpp
	src/Threads/LoadHdrThread.cpp
	src/Threads/Mantiuk06Thread.cpp	
	src/Threads/Mantiuk08Thread.cpp
	src/Threads/Pattanaik00Thread.cpp
	src/Threads/Reinhard02Thread.cpp
	src/Threads/Reinhard05Thread.cpp
	src/Threads/TMOThread.cpp
	src/Threads/TMOFactory.cpp
	src/TonemappingWindow/TMOProgressIndicator.cpp
	src/TonemappingWindow/TonemappingPanel.cpp
	src/TonemappingWindow/TonemappingWarnDialog.cpp
	src/TonemappingWindow/TonemappingWindow.cpp
	src/TransplantExif/TransplantExifDialog.cpp
	src/UI/UMessageBox.cpp
	src/Viewers/GenericViewer.cpp
	src/Viewers/HdrViewer.cpp
	src/Viewers/LdrViewer.cpp
	src/Viewers/Histogram.cpp
	src/Viewers/LuminanceRangeWidget.cpp
	src/Viewers/SmartScrollArea.cpp
	src/Common/GammaAndLevels.cpp
	src/Common/Gang.cpp	
	src/Common/ImageQualityDialog.cpp
	src/Common/PanIconWidget.cpp
	src/Common/ProgressHelper.cpp	
	src/Common/SelectionTool.cpp
	src/Common/commandline.cpp	
	src/Common/global.cpp
	src/Common/options.cpp
	
	src/main.cpp
)

SET(LUMINANCE_HDR_MOC
	src/Batch/BatchTMDialog.h
	src/Batch/BatchTMJob.h
	src/Core/IOWorker.h
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
	src/Projection/ProjectionsDialog.h
	src/Resize/ResizeDialog.h
	src/Threads/Ashikhmin02Thread.h
	src/Threads/Drago03Thread.h
	src/Threads/Durand02Thread.h
	src/Threads/Fattal02Thread.h
	src/Threads/HdrInputLoader.h
	src/Threads/LoadHdrThread.h
	src/Threads/Mantiuk06Thread.h	
	src/Threads/Mantiuk08Thread.h
	src/Threads/Pattanaik00Thread.h
	src/Threads/Reinhard02Thread.h
	src/Threads/Reinhard05Thread.h
	src/Threads/TMOThread.h
	src/TonemappingWindow/TMOProgressIndicator.h
	src/TonemappingWindow/TonemappingPanel.h
	src/TonemappingWindow/TonemappingWarnDialog.h
	src/TonemappingWindow/TonemappingWindow.h
	src/TransplantExif/TransplantExifDialog.h
	src/Viewers/GenericViewer.h
	src/Viewers/HdrViewer.h
	src/Viewers/LdrViewer.h
	src/Viewers/LuminanceRangeWidget.h
	src/Viewers/SmartScrollArea.h
	src/Common/GammaAndLevels.h
	src/Common/Gang.h
	src/Common/ImageQualityDialog.h
	src/Common/PanIconWidget.h
	src/Common/ProgressHelper.h	
	src/Common/SelectionTool.h	
	src/Common/commandline.h
)

SET(LUMINANCE_HDR_UI
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
	forms/ProjectionsDialog.ui
	forms/ResizeDialog.ui
	forms/Splash.ui
	forms/TonemappingPanel.ui
	forms/TonemappingWarnDialog.ui
	forms/TonemappingWindow.ui
	forms/TransplantExifDialog.ui
	forms/about.ui
)

SET(LUMINANCE_HDR_RCCS icons.qrc)

QT4_WRAP_CPP(LIB_HDR_MOC_SRC ${LIB_HDR_MOC})
QT4_WRAP_CPP(LUMINANCE_HDR_MOC_SRC ${LUMINANCE_HDR_MOC})

QT4_ADD_RESOURCES(LUMINANCE_HDR_RCCS_SRCS ${LUMINANCE_HDR_RCCS})

QT4_WRAP_UI(LUMINANCE_HDR_UI_HEADERS ${LUMINANCE_HDR_UI})

SET(LUMINANCE_FILES
	${LIB_HDR_INCLUDE}
	${LIB_HDR_SRC}
	${LIB_HDR_MOC_SRC}
	${LUMINANCE_HDR_UI_HEADERS}
	${LUMINANCE_HDR_INCLUDE}
	${LUMINANCE_HDR_SRC}
	${LUMINANCE_HDR_MOC_SRC}
	${LUMINANCE_HDR_RCCS_SRCS}
)


IF(APPLE)
  # icon files to copy in the bundle
  SET( OSX_ICON_FILES "${CMAKE_SOURCE_DIR}/images/luminance.icns")
  # set where in the bundle to put the icns files
  SET_SOURCE_FILES_PROPERTIES(${OSX_ICON_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  # include the icns files in the target
  SET( LUMINANCE_FILES ${LUMINANCE_FILES} ${OSX_ICON_FILES} )

  ADD_DEFINITIONS(-DI18NDIR=QCoreApplication::applicationDirPath\(\)+"/../Resources/i18n")
  ADD_EXECUTABLE(luminance-hdr MACOSX_BUNDLE ${LUMINANCE_FILES})
  
  SET_TARGET_PROPERTIES(luminance-hdr PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist")
ELSEIF(UNIX)
  ADD_DEFINITIONS(-DI18NDIR="${CMAKE_INSTALL_PREFIX}/share/luminance-hdr/i18n")
  ADD_EXECUTABLE(luminance-hdr ${LUMINANCE_FILES})  
ELSEIF(WIN32)
  ADD_EXECUTABLE(luminance-hdr WIN32 ${LUMINANCE_FILES})
  
  # Set icon!
ENDIF()

TARGET_LINK_LIBRARIES(luminance-hdr ${LIBS})