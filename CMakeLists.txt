PROJECT(luminance-hdr)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

# set include path for FindXXX.cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Global variable for CMake
INCLUDE(CMakeGlobal.txt)

IF(NOT CMAKE_BUILD_TYPE)
    SET( CMAKE_BUILD_TYPE "RelWithDebInfo" )
ENDIF()

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
ADD_DEFINITIONS(${QT_DEFINITIONS})

set(LIBS ${LIBS} ${QT_QTCORE_LIBRARIES} ${QT_QTGUI_LIBRARIES} ${QT_QTWEBKIT_LIBRARIES} ${QT_QTXML_LIBRARIES} ${QT_QTSQL_LIBRARIES})

FIND_PACKAGE(Git)
#FIND_PACKAGE(Git QUIET)
IF(NOT GIT_FOUND)
	message(STATUS "Git not found")
    FIND_FILE(GIT_EXECUTABLE git.exe 
        $ENV{CYGWIN_DIR}\\bin
        $ENV{CYGWIN_DIR}
    )
    IF (EXISTS ${GIT_EXECUTABLE})
        SET(GIT_FOUND 1)
    ENDIF()
    FIND_FILE(GIT_EXE git
        $ENV{/usr/bin}
    )
    IF (EXISTS ${GIT_EXE})
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

        #ADD_DEFINITIONS(-O3)

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

        EXECUTE_PROCESS(COMMAND ${CMAKE_C_COMPILER} -dumpversion
					OUTPUT_VARIABLE GCC_VERSION)
	#MESSAGE(STATUS ${GCC_VERSION})
	IF(GCC_VERSION VERSION_GREATER 4.3 OR GCC_VERSION VERSION_EQUAL 4.3)
                MESSAGE(STATUS "GCC >= 4.3")
		FIND_PACKAGE(OpenMP REQUIRED)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	endif()

ENDIF()
# END OpenMP Support

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/src/" ${INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SET(LUMINANCE_HDR_H )
SET(LUMINANCE_HDR_SRC )

IF(MSVC)
SET(LUMINANCE_HDR_H ${LUMINANCE_HDR_H} src/arch/msvc/getopt_win32.h)
SET(LUMINANCE_HDR_SRC ${LUMINANCE_HDR_CPP} src/arch/msvc/getopt_win32.c)
ENDIF()

SET(LUMINANCE_HDR_H ${LUMINANCE_HDR_H}
        src/arch/math.h
        src/arch/malloc.h
        src/arch/minmax.h
        src/arch/string.h
        src/arch/freebsd/math.h
        src/arch/msvc/math.h
)

SET(LUMINANCE_HDR_RCCS icons.qrc)
QT4_ADD_RESOURCES(LUMINANCE_HDR_RCCS_SRCS ${LUMINANCE_HDR_RCCS})

ADD_SUBDIRECTORY(src)

# translations
FILE(GLOB LUMINANCE_TS i18n/*.ts)
SET(FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${LUMINANCE_TS}s)

IF(UPDATE_TRANSLATIONS)
    IF(LUPDATE_NOOBSOLETE)
        QT4_CREATE_TRANSLATION(LUMINANCE_QM ${FILES_TO_TRANSLATE} OPTIONS -noobsolete)
    ELSE()
        QT4_CREATE_TRANSLATION(LUMINANCE_QM ${FILES_TO_TRANSLATE})
    ENDIF()
ELSE(UPDATE_TRANSLATIONS)
    QT4_ADD_TRANSLATION(LUMINANCE_QM ${LUMINANCE_TS})

    SET(LUMINANCE_FILES
        ${LUMINANCE_HDR_H}
        ${LUMINANCE_HDR_CPP}
        ${LUMINANCE_HDR_RCCS_SRCS}
        ${LUMINANCE_QM})

IF(APPLE)
    # icon files to copy in the bundle
    SET( OSX_ICON_FILES "${CMAKE_SOURCE_DIR}/images/luminance.icns")
    # set where in the bundle to put the icns files
    SET_SOURCE_FILES_PROPERTIES(${OSX_ICON_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    # set where in the bundle to put the translation files
    SET_SOURCE_FILES_PROPERTIES(${LUMINANCE_QM} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/i18n)
    # include the icns files in the target
    SET(LUMINANCE_FILES ${LUMINANCE_FILES} ${OSX_ICON_FILES})

    SET( PROGNAME luminance-hdr )
    SET( MACOSX_BUNDLE_ICON_FILE "${CMAKE_SOURCE_DIR}/images/luminance.icns" )
    #SET( MACOSX_BUNDLE_SHORT_VERSION_STRING 0.7-beta2 )
    #SET( MACOSX_BUNDLE_VERSION 0.7-beta2 )
    #SET( MACOSX_BUNDLE_LONG_VERSION_STRING Version 0.7-beta2 )
    #SET( CMAKE_OSX_ARCHITECTURES x86_64;i386 )  # Universal build

    ADD_EXECUTABLE(luminance-hdr MACOSX_BUNDLE ${LUMINANCE_FILES})
  
    SET_TARGET_PROPERTIES(luminance-hdr PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/build/macosx/Info.plist")

ELSEIF(UNIX)

    ADD_EXECUTABLE(luminance-hdr ${LUMINANCE_FILES})

    # make install directivies
    INSTALL(TARGETS luminance-hdr RUNTIME DESTINATION bin) # main exec
    INSTALL(FILES ${LUMINANCE_QM} DESTINATION share/luminance-hdr/i18n) # i18n
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/images/luminance-hdr.png DESTINATION share/icons/hicolor/48x48/apps) # icon
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/luminance-hdr.desktop DESTINATION share/applications) #desktop entry
    INSTALL(FILES
    ${CMAKE_SOURCE_DIR}/AUTHORS
    ${CMAKE_SOURCE_DIR}/README
    ${CMAKE_SOURCE_DIR}/LICENSE
    ${CMAKE_SOURCE_DIR}/Changelog
    DESTINATION share/luminance-hdr) #info files
    INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/help DESTINATION share/luminance-hdr) # help directory

ELSEIF(WIN32)

    # icon files to copy in the bundle
    SET(LUMINANCE_FILES ${LUMINANCE_FILES} "${CMAKE_SOURCE_DIR}/images/luminance_ico.rc")

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

    FILE(GLOB QtI18nAll "${QT_TRANSLATIONS_DIR}/qt_*.qm")
    FILE(GLOB QtI18nHelp "${QT_TRANSLATIONS_DIR}/qt_help_*.qm")
    LIST(LENGTH QtI18nHelp QtI18nHelp_length)
    IF(${QtI18nHelp_length} GREATER 0)
        LIST(REMOVE_ITEM QtI18nAll ${QtI18nHelp})
    ENDIF()
    
    FILE(COPY ${QtI18nAll} DESTINATION "${CMAKE_BINARY_DIR}/QtDlls/i18n/")

ENDIF()

# Link sub modules
TARGET_LINK_LIBRARIES(luminance-hdr ${LUMINANCE_MODULES_GUI})
# Link shared library
TARGET_LINK_LIBRARIES(luminance-hdr ${LIBS})

ENDIF(UPDATE_TRANSLATIONS)

