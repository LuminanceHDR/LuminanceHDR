PROJECT("Luminance HDR") # luminance-hdr)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

# set include path for FindXXX.cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Global variables for CMake
include(cmake/Version.cmake)
include(cmake/GlobalSettings.cmake)
include(cmake/CompilerSettings.cmake)
include(cmake/Internationalization.cmake)

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

set(LIBS ${LIBS}
    ${QT_QTCORE_LIBRARIES} ${QT_QTGUI_LIBRARIES}
    ${QT_QTWEBKIT_LIBRARIES} ${QT_QTXML_LIBRARIES} ${QT_QTSQL_LIBRARIES})

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

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/src/")
INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}/src/")
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
    SET(CMAKE_INSTALL_PREFIX "/Applications")

    # Set Bundle settings
    set(MACOSX_BUNDLE_INFO_STRING "${LHDR_NAME} ${LHDR_VERSION}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${LHDR_NAME} ${LHDR_VERSION}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${LHDR_NAME} ${LHDR_VERSION}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${LHDR_VERSION}")
    set(MACOSX_BUNDLE_VERSION "${LHDR_VERSION}")
    set(MACOSX_BUNDLE_ICON_FILE "${CMAKE_SOURCE_DIR}/images/luminance.icns")
    set(MACOSX_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/images/luminance.icns")
    set(MACOSX_BUNDLE_RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "${LHDR_DOMAIN_SECOND}.${LHDR_DOMAIN_FIRST}")
    set(MACOSX_BUNDLE_BUNDLE_NAME "${LHDR_NAME}")

    configure_file( ${CMAKE_SOURCE_DIR}/build/macosx/Info.plist.in
                    ${CMAKE_BINARY_DIR}/Info.plist )
    configure_file( ${CMAKE_SOURCE_DIR}/build/macosx/create-dmg.sh.in
                    ${CMAKE_BINARY_DIR}/create-dmg.sh )

    FILE(GLOB LUMINANCE_QT_QM i18n-qt/*.qm) # ${QT_TRANSLATIONS_DIR}/qt_*.qm)
    SET(LUMINANCE_FILES ${LUMINANCE_FILES} ${LUMINANCE_QT_QM})

    # icon files to copy in the bundle
    SET(OSX_ICON_FILES "${CMAKE_SOURCE_DIR}/images/luminance.icns")
    # include the icns files in the target
    SET(LUMINANCE_FILES ${LUMINANCE_FILES} ${OSX_ICON_FILES})

    # Build project
    # SET( CMAKE_OSX_ARCHITECTURES x86_64;i386 )  # Universal build
    ADD_EXECUTABLE(${LHDR_OSX_EXECUTABLE_NAME} MACOSX_BUNDLE ${LUMINANCE_FILES}) # luminance-hdr
    ADD_EXECUTABLE(luminance-hdr-cli ${LUMINANCE_FILES})
  
    SET_TARGET_PROPERTIES( ${LHDR_OSX_EXECUTABLE_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST
                           "${CMAKE_BINARY_DIR}/Info.plist" )

    # set where in the bundle to put the icns files
    SET_SOURCE_FILES_PROPERTIES(${OSX_ICON_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    # set where in the bundle to put the translation files
    SET_SOURCE_FILES_PROPERTIES(${LUMINANCE_QM} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/i18n)
    SET_SOURCE_FILES_PROPERTIES(${LUMINANCE_QT_QM} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/i18n)

    INSTALL(TARGETS ${LHDR_OSX_EXECUTABLE_NAME} luminance-hdr-cli
            BUNDLE DESTINATION . COMPONENT Runtime
            RUNTIME DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/MacOS COMPONENT Runtime)

    # Link libraries
    TARGET_LINK_LIBRARIES(${LHDR_OSX_EXECUTABLE_NAME} ${LUMINANCE_MODULES_GUI} ${LIBS})
    TARGET_LINK_LIBRARIES(luminance-hdr-cli ${LUMINANCE_MODULES_CLI} ${LIBS})
# Unix
ELSEIF(UNIX)
    FILE(GLOB LUMINANCE_QT_QM i18n-qt/*.qm)
    SET(LUMINANCE_FILES ${LUMINANCE_FILES} ${LUMINANCE_QT_QM})

    ADD_EXECUTABLE(luminance-hdr ${LUMINANCE_FILES})
    ADD_EXECUTABLE(luminance-hdr-cli ${LUMINANCE_FILES})

    # link libraries
    TARGET_LINK_LIBRARIES(luminance-hdr -Xlinker --start-group ${LUMINANCE_MODULES_GUI} -Xlinker --end-group)
    TARGET_LINK_LIBRARIES(luminance-hdr-cli -Xlinker --start-group ${LUMINANCE_MODULES_CLI} -Xlinker --end-group)
    TARGET_LINK_LIBRARIES(luminance-hdr ${LIBS})
    TARGET_LINK_LIBRARIES(luminance-hdr-cli ${LIBS})

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
  
    # Link libraries
    TARGET_LINK_LIBRARIES(luminance-hdr ${LUMINANCE_MODULES_GUI} ${LIBS})
    TARGET_LINK_LIBRARIES(luminance-hdr-cli ${LUMINANCE_MODULES_CLI} ${LIBS})

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

# Packaging must go at the bottom
include(${CMAKE_SOURCE_DIR}/build/Packaging.cmake)
