PROJECT(luminance-hdr)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

# set include path for FindXXX.cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Global variable for CMake
INCLUDE(CMakeGlobal.txt)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "RelWithDebInfo" )
ENDIF()

# MESSAGE(${CMAKE_BUILD_TYPE})

IF(${CMAKE_BUILD_TYPE} STREQUAL "Release" OR ${CMAKE_BUILD_TYPE} STREQUAL "MinSizeRel")
ADD_DEFINITIONS("-DQT_NO_DEBUG_OUTPUT")
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
IF(GIT_FOUND)
    include(GetGitRevisionDescription)
    get_git_head_revision(GIT_REFSPEC GIT_SHA1)
ENDIF()

IF(WIN32)
    # Windows Section
    
    # to not define min/max macros, which cause troubles with STL and boost
    ADD_DEFINITIONS(-DNOMINMAX)
    
    SET(OPENEXR_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/OpenEXR")
    SET(LIBRAW_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/libraw")
    SET(FFTWF_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/fftw3")
    SET(GSL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/gsl")
    SET(GSLCBLAS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/gsl")
    SET(LCMS2_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/lcms2")
    SET(JPEGLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/libjpeg")

    # Enable multi processor compilation
    ADD_DEFINITIONS(/MP)

    # Enable SSE2
    IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
        # SSE2 is on by default, on x64 platforms
    	ADD_DEFINITIONS(/arch:SSE2)
    ENDIF() 

    # Don't "Treat WChar_t As Build in Type", in order to be able to use Qts (from|to)StdWString functions
    ADD_DEFINITIONS(/Zc:wchar_t-)
	
    ADD_DEFINITIONS(-DUNICODE )
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -DOPENEXR_DLL -D_REENTRANT)
    SET(FFTWF_LIBRARIES "${CMAKE_SOURCE_DIR}/../DEPs/lib/fftw3/libfftw3f-3.lib")
	
    SET(OPENEXR_LIBRARIES "${CMAKE_SOURCE_DIR}/../DEPs/lib/OpenEXR/Half.lib")
    SET(OPENEXR_LIBRARIES ${OPENEXR_LIBRARIES} "${CMAKE_SOURCE_DIR}/../DEPs/lib/OpenEXR/Iex.lib")
    SET(OPENEXR_LIBRARIES ${OPENEXR_LIBRARIES} "${CMAKE_SOURCE_DIR}/../DEPs/lib/OpenEXR/IlmImf.lib")
	
    SET(LIBRAW_LIBRARIES "${CMAKE_SOURCE_DIR}/../DEPs/lib/libraw/libraw.lib")
    ADD_DEFINITIONS("-DDEMOSAICING_GPL2")
    ADD_DEFINITIONS("-DDEMOSAICING_GPL3")
	
    SET(GSL_LIBRARIES "${CMAKE_SOURCE_DIR}/../DEPs/lib/gsl/gsl.lib")
    SET(GSLCBLAS_LIBRARIES "${CMAKE_SOURCE_DIR}/../DEPs/lib/gsl/cblas.lib")
	
    SET(LCMS2_LIBRARIES "${CMAKE_SOURCE_DIR}/../DEPs/lib/lcms2/lcms2_dll.lib")
    ADD_DEFINITIONS(-DCMS_DLL)
    ADD_DEFINITIONS(-DUSE_LCMS2)
ELSE()
    IF(APPLE)
    # optimize for 64-bit core 2 architecture on apple
    ADD_DEFINITIONS(-march=core2)
    ELSE()
    ADD_DEFINITIONS(-std=c++0x -ffast-math -msse2 -msse)
    ENDIF()

    # Unix & Apple part
    FIND_PACKAGE(OpenEXR REQUIRED)
    FIND_PACKAGE(LibRAW REQUIRED)
    FIND_PACKAGE(fftwf REQUIRED)
    FIND_PACKAGE(GSL REQUIRED)
    FIND_PACKAGE(libjpeg REQUIRED)
    FIND_PACKAGE(lcms2 REQUIRED)

ENDIF()


FIND_PACKAGE(Boost REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

FIND_PACKAGE(exiv2 REQUIRED)
INCLUDE_DIRECTORIES(${EXIV2_INCLUDE_DIR} "${EXIV2_INCLUDE_DIR}/exiv2")

FIND_PACKAGE(TIFF REQUIRED)
INCLUDE_DIRECTORIES(${TIFF_INCLUDE_DIR})

FIND_PACKAGE(PNG REQUIRED)
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})

INCLUDE_DIRECTORIES(${OPENEXR_INCLUDE_DIR} "${OPENEXR_INCLUDE_DIR}/OpenEXR")
INCLUDE_DIRECTORIES(${LIBRAW_INCLUDE_DIR} "${LIBRAW_INCLUDE_DIR}/libraw")
INCLUDE_DIRECTORIES(${FFTWF_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${GSL_INCLUDE_DIR} ${GSLCBLAS_INCLUDE_DIR})

INCLUDE_DIRECTORIES(${JPEGLIB_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${LCMS2_INCLUDE_DIR})

SET(LIBS ${LIBS} ${OPENEXR_LIBRARIES})
SET(LIBS ${LIBS} ${TIFF_LIBRARIES})
SET(LIBS ${LIBS} ${LIBRAW_LIBRARIES})
SET(LIBS ${LIBS} ${FFTWF_LIBRARIES})
ADD_DEFINITIONS(-DHAVE_FFTW3F)
SET(LIBS ${LIBS} ${GSL_LIBRARIES} ${GSLCBLAS_LIBRARIES})
SET(LIBS ${LIBS} ${EXIV2_LIBRARIES})
SET(LIBS ${LIBS} ${JPEGLIB_LIBRARIES})
SET(LIBS ${LIBS} ${LCMS2_LIBRARIES})
SET(LIBS ${LIBS} ${PNG_LIBRARIES})

#OpenMP Support under Linux, Windows with MSVC & MacOS X with GCC >= 4.3
IF(MSVC)
    FIND_PACKAGE(OpenMP REQUIRED)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
ELSEIF(CMAKE_COMPILER_IS_GNUCC AND UNIX)
    EXECUTE_PROCESS(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    #MESSAGE(STATUS ${GCC_VERSION})
    IF(GCC_VERSION VERSION_GREATER 4.3 OR GCC_VERSION VERSION_EQUAL 4.3)
        MESSAGE(STATUS "GCC >= 4.3")
        FIND_PACKAGE(OpenMP REQUIRED)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    ENDIF()
ENDIF()
# END OpenMP Support

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/src/")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SET(LUMINANCE_HDR_H )
SET(LUMINANCE_HDR_SRC )

IF(MSVC)
    SET(LUMINANCE_HDR_H ${LUMINANCE_HDR_H} src/arch/msvc/getopt_win32.h)
    SET(LUMINANCE_HDR_CPP ${LUMINANCE_HDR_CPP} src/arch/msvc/getopt_win32.c)
ENDIF()

SET(LUMINANCE_HDR_H ${LUMINANCE_HDR_H}
    src/arch/math.h
    src/arch/malloc.h
    src/arch/string.h
    src/arch/freebsd/math.h
    src/arch/msvc/math.h
)

SET(LUMINANCE_HDR_RCCS icons.qrc)
QT4_ADD_RESOURCES(LUMINANCE_HDR_RCCS_SRCS ${LUMINANCE_HDR_RCCS})

ENABLE_TESTING()

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(tools)

# translations
FILE(GLOB LUMINANCE_TS i18n/*.ts)

IF(UPDATE_TRANSLATIONS)
    IF(LUPDATE_NOOBSOLETE)
        QT4_CREATE_TRANSLATION(LUMINANCE_QM ${FILES_TO_TRANSLATE} ${LUMINANCE_TS} OPTIONS -noobsolete)
    ELSE()
        QT4_CREATE_TRANSLATION(LUMINANCE_QM ${FILES_TO_TRANSLATE} ${LUMINANCE_TS})
    ENDIF()
ELSE(UPDATE_TRANSLATIONS)
    QT4_ADD_TRANSLATION(LUMINANCE_QM ${LUMINANCE_TS})
ENDIF(UPDATE_TRANSLATIONS)

ADD_CUSTOM_TARGET(translations_target DEPENDS ${LUMINANCE_QM})

SET(LUMINANCE_FILES
    ${LUMINANCE_HDR_H}
    ${LUMINANCE_HDR_CPP}
    ${LUMINANCE_HDR_RCCS_SRCS}
    ${LUMINANCE_QM})

# Apple Mac OS X
IF(APPLE)
    # TODO
    # 1. copy help under Resources/help
    # 2. copy html under Resources/html
    # 3. deployment align_image_stack

    CONFIGURE_FILE(
    ${CMAKE_SOURCE_DIR}/build/macosx/create-dmg.sh
    ${CMAKE_BINARY_DIR}/create-dmg.sh)

    CONFIGURE_FILE(
    ${CMAKE_SOURCE_DIR}/build/macosx/deploy-cli.sh
    ${CMAKE_BINARY_DIR}/deploy-cli.sh)

    FILE(GLOB LUMINANCE_QT_QM i18n-qt/*.qm) # ${QT_TRANSLATIONS_DIR}/qt_*.qm)
    SET(LUMINANCE_FILES ${LUMINANCE_FILES} ${LUMINANCE_QT_QM})

    # icon files to copy in the bundle
    SET( OSX_ICON_FILES "${CMAKE_SOURCE_DIR}/images/luminance.icns")
    # set where in the bundle to put the icns files
    SET_SOURCE_FILES_PROPERTIES(${OSX_ICON_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    # set where in the bundle to put the translation files
    SET_SOURCE_FILES_PROPERTIES(${LUMINANCE_QM} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/i18n)
    SET_SOURCE_FILES_PROPERTIES(${LUMINANCE_QT_QM} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/i18n)

    # include the icns files in the target
    SET(LUMINANCE_FILES ${LUMINANCE_FILES} ${OSX_ICON_FILES})

    SET( PROGNAME luminance-hdr )
    SET( MACOSX_BUNDLE_ICON_FILE "${CMAKE_SOURCE_DIR}/images/luminance.icns" )
    SET( MACOSX_BUNDLE_SHORT_VERSION_STRING 2.3.0 )
    SET( MACOSX_BUNDLE_VERSION 2.3.0 )
    SET( MACOSX_BUNDLE_LONG_VERSION_STRING Luminance HDR 2.3.0 )
    #SET( CMAKE_OSX_ARCHITECTURES x86_64;i386 )  # Universal build

    ADD_EXECUTABLE(luminance-hdr MACOSX_BUNDLE ${LUMINANCE_FILES})
    ADD_EXECUTABLE(luminance-hdr-cli ${LUMINANCE_FILES})
  
    SET_TARGET_PROPERTIES(luminance-hdr PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/build/macosx/Info.plist")
# Unix
ELSEIF(UNIX)
    FILE(GLOB LUMINANCE_QT_QM i18n-qt/*.qm)
    SET(LUMINANCE_FILES ${LUMINANCE_FILES} ${LUMINANCE_QT_QM})

    ADD_EXECUTABLE(luminance-hdr ${LUMINANCE_FILES})
    ADD_EXECUTABLE(luminance-hdr-cli ${LUMINANCE_FILES})

    # make install directivies
    INSTALL(TARGETS luminance-hdr RUNTIME DESTINATION bin) # main GUI exec
    INSTALL(TARGETS luminance-hdr-cli RUNTIME DESTINATION bin) # main CLI exec
    INSTALL(FILES ${LUMINANCE_QM} DESTINATION share/luminance-hdr/i18n) # i18n
    INSTALL(FILES ${LUMINANCE_QT_QM} DESTINATION share/luminance-hdr/i18n) # i18n

    INSTALL(FILES ${CMAKE_SOURCE_DIR}/images/luminance-hdr.png DESTINATION share/icons/hicolor/48x48/apps) # icon
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/luminance-hdr.desktop DESTINATION share/applications) #desktop entry
    INSTALL(FILES
    ${CMAKE_SOURCE_DIR}/AUTHORS
    ${CMAKE_SOURCE_DIR}/README
    ${CMAKE_SOURCE_DIR}/LICENSE
    ${CMAKE_SOURCE_DIR}/Changelog
    DESTINATION share/luminance-hdr) #info files
    INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/help DESTINATION share/luminance-hdr) # help directory
# Microsoft Windows
ELSEIF(WIN32)
    # icon files to copy in the bundle
    SET(LUMINANCE_FILES ${LUMINANCE_FILES} "${CMAKE_SOURCE_DIR}/images/luminance_ico.rc")

    # Subsystem console:
    ADD_EXECUTABLE(luminance-hdr WIN32 ${LUMINANCE_FILES})
    ADD_EXECUTABLE(luminance-hdr-cli ${LUMINANCE_FILES})
  
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
        #STRING(REPLACE "4" "d4" debugdll ${dll})
        #FILE(COPY "${QT_BINARY_DIR}/${debugdll}" DESTINATION "${CMAKE_BINARY_DIR}/QtDlls/Debug")
    ENDFOREACH()
    
    FILE(COPY "${QT_PLUGINS_DIR}/imageformats/qjpeg4.dll" DESTINATION "${CMAKE_BINARY_DIR}/QtDlls/Release/imageformats")
    #FILE(COPY "${QT_PLUGINS_DIR}/imageformats/qjpegd4.dll" DESTINATION "${CMAKE_BINARY_DIR}/QtDlls/Debug/imageformats")
    FILE(COPY "${QT_PLUGINS_DIR}/sqldrivers/qsqlite4.dll" DESTINATION "${CMAKE_BINARY_DIR}/QtDlls/Release/sqldrivers")
    #FILE(COPY "${QT_PLUGINS_DIR}/sqldrivers/qsqlited4.dll" DESTINATION "${CMAKE_BINARY_DIR}/QtDlls/Debug/sqldrivers")

    FILE(GLOB QtI18nAll "${QT_TRANSLATIONS_DIR}/qt_*.qm")
    FILE(GLOB QtI18nHelp "${QT_TRANSLATIONS_DIR}/qt_help_*.qm")
    LIST(LENGTH QtI18nHelp QtI18nHelp_length)
    IF(${QtI18nHelp_length} GREATER 0)
        LIST(REMOVE_ITEM QtI18nAll ${QtI18nHelp})
    ENDIF()
    
    FILE(COPY ${QtI18nAll} DESTINATION "${CMAKE_BINARY_DIR}/QtDlls/i18n/")
ENDIF()

# Link sub modules
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(luminance-hdr ${LUMINANCE_MODULES_GUI})
    TARGET_LINK_LIBRARIES(luminance-hdr-cli ${LUMINANCE_MODULES_CLI})
ELSE()
    TARGET_LINK_LIBRARIES(luminance-hdr -Xlinker --start-group ${LUMINANCE_MODULES_GUI} -Xlinker --end-group)
    TARGET_LINK_LIBRARIES(luminance-hdr-cli -Xlinker --start-group ${LUMINANCE_MODULES_CLI} -Xlinker --end-group)
ENDIF()
# Link shared library
TARGET_LINK_LIBRARIES(luminance-hdr ${LIBS})
TARGET_LINK_LIBRARIES(luminance-hdr-cli ${LIBS})
