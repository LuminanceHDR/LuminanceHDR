CMAKE_MINIMUM_REQUIRED(VERSION 3.3)
CMAKE_POLICY(SET CMP0025 NEW)
CMAKE_POLICY(SET CMP0043 NEW)
CMAKE_POLICY(SET CMP0054 NEW)
PROJECT("Luminance HDR")

# set include path for FindXXX.cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/build_files/Modules")

# Global variables for CMake
include(build_files/Modules/Version.cmake)
include(build_files/Modules/GlobalSettings.cmake)
include(build_files/Modules/CompilerSettings.cmake)
include(build_files/Modules/Internationalization.cmake)

IF(TIMER_PROFILING)
    ADD_DEFINITIONS(-DTIMER_PROFILING)
ENDIF()

# ======== Enable GNU gsl inline code =======
IF(UNIX OR APPLE OR MINGW) # Visual Studio doesn't like this
    ADD_DEFINITIONS(-DHAVE_INLINE )
ENDIF()

# ======== Has branch prediction =======
include( CheckCXXSourceCompiles )

check_cxx_source_compiles( "int main() { int x = 0; if( __builtin_expect((x),0) ) x = 1; return 0; }" HAS_BRANCH_PREDICTION )

if( HAS_BRANCH_PREDICTION )
  set( BRANCH_PREDICTION 1 )
else( HAS_BRANCH_PREDICTION )
  set( BRANCH_PREDICTION 0 )
endif( HAS_BRANCH_PREDICTION )

# find and setup Qt5 for this project

find_package(Qt5Core             REQUIRED)
find_package(Qt5Concurrent       REQUIRED)
find_package(Qt5Widgets          REQUIRED)
find_package(Qt5Gui              REQUIRED)
IF(MINGW)
    find_package(Qt5WebKit           REQUIRED)
    find_package(Qt5WebKitWidgets    REQUIRED)
ELSE()
    find_package(Qt5WebEngineCore    REQUIRED)
    find_package(Qt5WebEngineWidgets REQUIRED)
ENDIF()
find_package(Qt5Xml              REQUIRED)
find_package(Qt5Sql              REQUIRED)
find_package(Qt5Svg              REQUIRED)
find_package(Qt5Network          REQUIRED)
find_package(Qt5LinguistTools    REQUIRED)
find_package(Qt5PrintSupport     REQUIRED)
IF(WIN32)
    find_package(Qt5WinExtras)
ENDIF()

# Include the qtmain lib if we are on the windows platform
IF(WIN32)
    SET(LIBS ${LIBS} ${Qt5Core_QTMAIN_LIBRARIES})
ENDIF(WIN32)

set(LIBS ${LIBS}
    ${QT_QTCORE_LIBRARIES}     ${QT_QTGUI_LIBRARIES}  ${QT_QTNETWORK_LIBRARIES}
    ${QT_QTWEBENGINE_LIBRARIES} ${QT_QTXML_LIBRARIES} ${QT_QTSQL_LIBRARIES})

FIND_PACKAGE(Git)
IF(GIT_FOUND)
    include(GetGitRevisionDescription)
    get_git_head_revision(GIT_REFSPEC GIT_SHA1)
ENDIF()

IF(WIN32)

    # Windows Section

    # to not define min/max macros, which cause troubles with STL and boost
    ADD_DEFINITIONS(-DNOMINMAX)
    IF(MINGW)
        find_package(PkgConfig)
        pkg_check_modules(FFTW3F REQUIRED fftw3f)
        SET(FFTWF_LIBRARIES "libfftw3f.a")
        pkg_check_modules(LCMS REQUIRED lcms2>=2.6)
        pkg_check_modules(LIBRAW REQUIRED libraw)
        ADD_DEFINITIONS("-DDEMOSAICING_GPL2")
        ADD_DEFINITIONS("-DDEMOSAICING_GPL3")
        find_package(OpenMP)
        IF(OPENMP_FOUND)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} -Werror=unknown-pragmas")
        ENDIF()
    ELSE()
        SET(LIBRAW_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/libraw")
        SET(FFTWF_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/fftw3")
        SET(LCMS2_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../DEPs/include/lcms2")

        # Enable multi processor compilation
        ADD_DEFINITIONS(/MP)

        # Enable SSE2
        IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
            # SSE2 is on by default, on x64 platforms
            ADD_DEFINITIONS(/arch:SSE2)
        ENDIF()

        ADD_DEFINITIONS(-DUNICODE )
        ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_REENTRANT)

        SET(LIBRAW_LIBRARIES "${CMAKE_SOURCE_DIR}/../DEPs/lib/libraw/libraw.lib")
        ADD_DEFINITIONS("-DDEMOSAICING_GPL2")
        ADD_DEFINITIONS("-DDEMOSAICING_GPL3")

        SET(LCMS2_LIBRARIES "${CMAKE_SOURCE_DIR}/../DEPs/lib/lcms2/lcms2.lib")
        ADD_DEFINITIONS(-DCMS_DLL)
        ADD_DEFINITIONS(-DUSE_LCMS2)

        INCLUDE_DIRECTORIES(${LIBRAW_INCLUDE_DIR} "${LIBRAW_INCLUDE_DIR}/libraw")
        INCLUDE_DIRECTORIES(${FFTWF_INCLUDE_DIR})
        INCLUDE_DIRECTORIES(${LCMS2_INCLUDE_DIR})
    ENDIF()
ELSE()
    # Unix & Apple part
    FIND_PACKAGE(LibRAW REQUIRED)
    INCLUDE_DIRECTORIES(${LIBRAW_INCLUDE_DIR} "${LIBRAW_INCLUDE_DIR}/libraw")

    FIND_PACKAGE(fftwf REQUIRED)
    INCLUDE_DIRECTORIES(${FFTWF_INCLUDE_DIR})

    FIND_PACKAGE(lcms2 REQUIRED)
    INCLUDE_DIRECTORIES(${LCMS2_INCLUDE_DIR})
ENDIF()


IF(WIN32)
    SET(Boost_USE_STATIC_LIBS ON)
ENDIF()
FIND_PACKAGE(Boost REQUIRED COMPONENTS program_options thread chrono system date_time)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

FIND_PACKAGE(exiv2 REQUIRED)
INCLUDE_DIRECTORIES(${EXIV2_INCLUDE_DIR} "${EXIV2_INCLUDE_DIR}/exiv2")

FIND_PACKAGE(TIFF REQUIRED)
INCLUDE_DIRECTORIES(${TIFF_INCLUDE_DIR})

FIND_PACKAGE(PNG REQUIRED)
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})

FIND_PACKAGE(OpenEXR REQUIRED)
INCLUDE_DIRECTORIES(${OPENEXR_INCLUDE_DIR} "${OPENEXR_INCLUDE_DIR}/OpenEXR")

FIND_PACKAGE(GSL REQUIRED)
INCLUDE_DIRECTORIES(${GSL_INCLUDE_DIR} ${GSLCBLAS_INCLUDE_DIR})

FIND_PACKAGE(JPEG REQUIRED)
INCLUDE_DIRECTORIES(${JPEGLIB_INCLUDE_DIR})

FIND_PACKAGE(CFITSIO)
IF(CFITSIO_FOUND)
    INCLUDE_DIRECTORIES(${CFITSIO_INCLUDE_DIR})
    SET(LIBS ${LIBS} ${CFITSIO_LIBRARIES})
    ADD_DEFINITIONS(-DHAVE_CFITSIO)
ELSE()
    MESSAGE(WARNING "cfitsio not found! Building without FITS support!")
ENDIF()

SET(LIBS ${LIBS} ${OPENEXR_LIBRARIES})
SET(LIBS ${LIBS} ${TIFF_LIBRARIES})
SET(LIBS ${LIBS} ${LIBRAW_LIBRARIES})
SET(LIBS ${LIBS} ${FFTWF_LIBRARIES})
ADD_DEFINITIONS(-DHAVE_FFTW3F)
SET(LIBS ${LIBS} ${GSL_LIBRARIES} ${GSLCBLAS_LIBRARIES})
SET(LIBS ${LIBS} ${EXIV2_LIBRARIES})
SET(LIBS ${LIBS} ${JPEG_LIBRARIES})
SET(LIBS ${LIBS} ${LCMS2_LIBRARIES})
SET(LIBS ${LIBS} ${PNG_LIBRARIES})
SET(LIBS ${LIBS} ${Boost_LIBRARIES})

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/src/")
INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}/src/")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SET(LUMINANCE_HDR_H )
SET(LUMINANCE_HDR_SRC )

SET(LUMINANCE_HDR_H ${LUMINANCE_HDR_H}
    src/arch/math.h
    src/arch/malloc.h
    src/arch/string.h
    src/arch/freebsd/math.h
    src/arch/msvc/math.h
)

SET(LUMINANCE_HDR_RCCS icons.qrc)
QT5_ADD_RESOURCES(LUMINANCE_HDR_RCCS_SRCS ${LUMINANCE_HDR_RCCS})

ADD_SUBDIRECTORY(src)

IF(ENABLE_UNIT_TEST)
    ENABLE_TESTING()
    ADD_SUBDIRECTORY(test)
ENDIF(ENABLE_UNIT_TEST)

# translations
FILE(GLOB LUMINANCE_TS i18n/lang_*.ts)

IF(UPDATE_TRANSLATIONS)
    SET(LUMINANCE_TS ${LUMINANCE_TS} i18n/source.ts)
    IF(LUPDATE_NOOBSOLETE)
        QT5_CREATE_TRANSLATION(LUMINANCE_QM ${FILES_TO_TRANSLATE} ${LUMINANCE_TS} OPTIONS -noobsolete)
    ELSE()
        QT5_CREATE_TRANSLATION(LUMINANCE_QM ${FILES_TO_TRANSLATE} ${LUMINANCE_TS})
    ENDIF()
    IF(UNIX)
        IF(NOT APPLE) # Sadly on Linux Qt5 add global include paths in list of translation files, let's get rid of them
            EXECUTE_PROCESS(COMMAND ${CMAKE_SOURCE_DIR}/build_files/platforms/linux/sanitize_translations.sh
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
        ENDIF(NOT APPLE)
    ENDIF(UNIX)
ELSE(UPDATE_TRANSLATIONS)
    QT5_ADD_TRANSLATION(LUMINANCE_QM ${LUMINANCE_TS})
ENDIF(UPDATE_TRANSLATIONS)

ADD_CUSTOM_TARGET(translations_target DEPENDS ${LUMINANCE_QM})

SET(LUMINANCE_FILES
    ${LUMINANCE_HDR_H}
    ${LUMINANCE_HDR_CPP}
    ${LUMINANCE_HDR_RCCS_SRCS}
    ${LUMINANCE_QM})

# MacOSX
IF(APPLE)
    SET(CMAKE_INSTALL_PREFIX ".")

    # Set Bundle settings
    set(MACOSX_BUNDLE_INFO_STRING          "${LHDR_NAME} ${LHDR_VERSION}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION       "${LHDR_NAME} ${LHDR_VERSION}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING  "${LHDR_NAME} ${LHDR_VERSION}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${LHDR_VERSION}")
    set(MACOSX_BUNDLE_VERSION              "${LHDR_VERSION}")
    set(MACOSX_BUNDLE_ICON_FILE            "${CMAKE_SOURCE_DIR}/program-icons/luminance.icns")
    set(MACOSX_BUNDLE_ICON                 "${CMAKE_SOURCE_DIR}/program-icons/luminance.icns")
    set(MACOSX_BUNDLE_RESOURCES            "${CMAKE_CURRENT_BINARY_DIR}/${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER       "${LHDR_DOMAIN_SECOND}.${LHDR_DOMAIN_FIRST}")
    set(MACOSX_BUNDLE_BUNDLE_NAME          "${LHDR_NAME}")

    configure_file( ${CMAKE_SOURCE_DIR}/build_files/platforms/macosx/Info.plist.in
                    ${CMAKE_BINARY_DIR}/Info.plist )

    # icon files to copy in the bundle
    SET(OSX_ICON_FILES "${CMAKE_SOURCE_DIR}/program-icons/luminance.icns")
    # include the icns files in the target
    SET(LUMINANCE_FILES ${LUMINANCE_FILES} ${OSX_ICON_FILES})

    # Build project (bundle and luminance-hdr-cli)
    SET( CMAKE_OSX_ARCHITECTURES x86_64 )  # Universal/Generic x86_64 build
    ADD_EXECUTABLE(${LHDR_OSX_EXECUTABLE_TARGET} MACOSX_BUNDLE ${LUMINANCE_FILES})
    ADD_EXECUTABLE(luminance-hdr-cli ${LUMINANCE_FILES} )
    set_property(TARGET ${LHDR_OSX_EXECUTABLE_TARGET} PROPERTY OUTPUT_NAME ${LHDR_OSX_EXECUTABLE_NAME})
    set_property(TARGET   luminance-hdr-cli           PROPERTY RUNTIME_OUTPUT_DIRECTORY ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/MacOS)

    # Mac bundle magic
    SET_TARGET_PROPERTIES( ${LHDR_OSX_EXECUTABLE_TARGET} PROPERTIES MACOSX_BUNDLE_INFO_PLIST
                           "${CMAKE_BINARY_DIR}/Info.plist" )

    # set where in the bundle to put the icns files
    SET_SOURCE_FILES_PROPERTIES(${OSX_ICON_FILES}  PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    # set where in the bundle to put the translation files
    SET_SOURCE_FILES_PROPERTIES(${LUMINANCE_QM}    PROPERTIES MACOSX_PACKAGE_LOCATION Resources/i18n)
    SET_SOURCE_FILES_PROPERTIES(${LUMINANCE_QT_QM} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/i18n)

    # QT magic
    qt5_use_modules(${LHDR_OSX_EXECUTABLE_TARGET} Core Gui Widgets)
    qt5_use_modules(luminance-hdr-cli Core Gui Widgets)

    # Link libraries
    TARGET_LINK_LIBRARIES(${LHDR_OSX_EXECUTABLE_TARGET} ${LUMINANCE_MODULES_GUI} ${LIBS})
    TARGET_LINK_LIBRARIES(luminance-hdr-cli             ${LUMINANCE_MODULES_CLI} ${LIBS})
    
    # Rename generated executable binary to luminance-hdr
    add_custom_command(
    TARGET ${LHDR_OSX_EXECUTABLE_TARGET}
    POST_BUILD
    COMMAND mv ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/MacOS/${LHDR_OSX_EXECUTABLE_NAME} ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/MacOS/luminance-hdr
    )
    
    # Copy align_image_stack into bundle/Contents/MacOS
    FILE(COPY
    	${CMAKE_SOURCE_DIR}/build_files/platforms/macosx/align_image_stack
    	DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/MacOS
    	FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE WORLD_EXECUTE
    )

    # Copy help/icons/hdrhtml and qt.conf to bundle resources
    FILE(COPY "${CMAKE_SOURCE_DIR}/help"    DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources)
    FILE(COPY "${CMAKE_SOURCE_DIR}/icons"   DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources)
    FILE(COPY "${CMAKE_SOURCE_DIR}/hdrhtml" DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources)
    FILE(COPY "${CMAKE_SOURCE_DIR}/build_files/platforms/macosx/qt.conf"
                                            DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources)

    # Copy Licence/README/Authors/Changelog to top of bundle
    FILE(COPY
         ${CMAKE_SOURCE_DIR}/AUTHORS
         ${CMAKE_SOURCE_DIR}/README.md
         ${CMAKE_SOURCE_DIR}/LICENSE
         ${CMAKE_SOURCE_DIR}/Changelog
         DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app
    )

# Unix
ELSEIF(UNIX)
    SET(LUMINANCE_FILES ${LUMINANCE_FILES})

    ADD_EXECUTABLE(luminance-hdr ${LUMINANCE_FILES})
    ADD_EXECUTABLE(luminance-hdr-cli ${LUMINANCE_FILES})

    qt5_use_modules(luminance-hdr Core Gui Widgets)
    qt5_use_modules(luminance-hdr-cli Core Gui Widgets)

    # link libraries
    TARGET_LINK_LIBRARIES(luminance-hdr -Xlinker --start-group ${LUMINANCE_MODULES_GUI} -Xlinker --end-group)
    TARGET_LINK_LIBRARIES(luminance-hdr-cli -Xlinker --start-group ${LUMINANCE_MODULES_CLI} -Xlinker --end-group)
    TARGET_LINK_LIBRARIES(luminance-hdr ${LIBS})
    TARGET_LINK_LIBRARIES(luminance-hdr-cli ${LIBS})

    # make install directivies
    INSTALL(TARGETS luminance-hdr RUNTIME DESTINATION bin) # main GUI exec
    INSTALL(TARGETS luminance-hdr-cli RUNTIME DESTINATION bin) # main CLI exec
    INSTALL(FILES ${LUMINANCE_QM} DESTINATION share/luminance-hdr/i18n) # i18n
    #INSTALL(FILES ${LUMINANCE_QT_QM} DESTINATION share/luminance-hdr/i18n) # i18n

    INSTALL(FILES ${CMAKE_SOURCE_DIR}/program-icons/luminance-hdr.png DESTINATION share/icons/hicolor/48x48/apps) # icon
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/luminance-hdr.desktop DESTINATION share/applications) #desktop entry
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/luminance-hdr.appdata.xml DESTINATION share/appdata) #appstream
    INSTALL(FILES
    ${CMAKE_SOURCE_DIR}/AUTHORS
    ${CMAKE_SOURCE_DIR}/README.md
    ${CMAKE_SOURCE_DIR}/LICENSE
    ${CMAKE_SOURCE_DIR}/Changelog
    DESTINATION share/luminance-hdr) #info files
    INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/help DESTINATION share/luminance-hdr) # help directory
    INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/hdrhtml DESTINATION share/luminance-hdr) # hdrhtml directory
# Microsoft Windows
ELSEIF(WIN32)
    IF(MINGW)
        set (PROJECT_VERSION_MAJOR ${LHDR_VERSION_MAJOR})
        set (PROJECT_VERSION_MINOR ${LHDR_VERSION_MINOR})
        set (PROJECT_VERSION_PATCH ${LHDR_VERSION_PATCH})

        find_program(ALIGN_IMAGE_STACK align_image_stack.exe HINTS "${HUGIN_BASE_PATH}" ENV Path ENV ProgramFiles ENV ProgramFiles(x86) PATH_SUFFIXES Hugin/bin bin)
        # Fail if align_image_stack.exe can't be found
        if (ALIGN_IMAGE_STACK STREQUAL ALIGN_IMAGE_STACK-NOTFOUND)
            message(FATAL_ERROR "align_image_stack.exe command not found! Please install Hugin from https://sourceforge.net/projects/hugin/. For non standard base install directory, please define -DHUGIN_BASE_PATH:PATH=xxx in the cmake command line")
        else ()
            message(STATUS "align_image_stack.exe command from Hugin install found: ${ALIGN_IMAGE_STACK}")
        endif ()
        get_filename_component(HUGIN_BIN_DIR "${ALIGN_IMAGE_STACK}" DIRECTORY ABSOLUTE)

        # ------------------ Find out whether we are building out of source ------------------

        get_filename_component(ABS_SOURCE_DIR "${PROJECT_SOURCE_DIR}" ABSOLUTE)
        get_filename_component(ABS_BINARY_DIR "${CMAKE_BINARY_DIR}" ABSOLUTE)
        set(OUT_OF_SOURCE_BUILD TRUE)
        if(ABS_SOURCE_DIR STREQUAL ABS_BINARY_DIR)
            set(OUT_OF_SOURCE_BUILD FALSE)
            message(WARNING "You are performing an in-source build. This is discouraged. For an explanation and the advantages of out-of-source builds, please refer to http://www.cmake.org/Wiki/CMake_FAQ#What_is_an_.22out-of-source.22_build.3F")
        endif()

        # ------------------------------------------------------------------------------------

        get_target_property(QT_LOCATION Qt5::Core LOCATION)
        get_filename_component(LIBS_SRC_DIR "${QT_LOCATION}" DIRECTORY)
        message(STATUS "> Base libs dir:    ${LIBS_SRC_DIR}")
        get_filename_component(QT5_PLUGINS_SRC_DIR "${LIBS_SRC_DIR}/../share/qt5/plugins" ABSOLUTE)
        message(STATUS "> Qt5 plugins libs: ${QT5_PLUGINS_SRC_DIR}")

        get_filename_component(PRGFILES_DIR "$ENV{ProgramFiles}" ABSOLUTE)
        get_filename_component(PRGFILESX86_DIR "$ENV{ProgramFiles\(x86\)}" ABSOLUTE)
        if((CMAKE_INSTALL_PREFIX STREQUAL "${PRGFILES_DIR}/Luminance HDR") OR (CMAKE_INSTALL_PREFIX STREQUAL "${PRGFILESX86_DIR}/Luminance HDR"))
            set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}-build")
        endif()

        set(RELINFO_WITH_ARGS ${CMAKE_COMMAND}
            -DLHDR_NAME:STRING="${LHDR_NAME}"
            -DLHDR_VERSION_MAJOR:STRING=${LHDR_VERSION_MAJOR}
            -DLHDR_VERSION_MINOR:STRING=${LHDR_VERSION_MINOR}
            -DLHDR_VERSION_PATCH:STRING=${LHDR_VERSION_PATCH}
            -DLHDR_VENDOR:STRING=${LHDR_VENDOR}
            -DPROJECT_SOURCE_DIR:STRING="${PROJECT_SOURCE_DIR}"
            -DSYSTEM:STRING=Windows
            -DLIBSSRCDIR:STRING="${LIBS_SRC_DIR}"
            -DCMAKE_INSTALL_PREFIX:STRING="${CMAKE_INSTALL_PREFIX}"
            -DBIT_DEPTH:STRING="${CMAKE_SIZEOF_VOID_P}"
            -DHUGIN_BIN_DIR:STRING="${HUGIN_BIN_DIR}"
            -P "${PROJECT_SOURCE_DIR}/build_files/platforms/msys2/UpdateInfo.cmake")

        add_custom_target(prepare_setup
            COMMAND ${RELINFO_WITH_ARGS}
            COMMENT "Creating ReleaseInfo and InnoSetup file")

        # icon files to copy in the bundle
        SET(LUMINANCE_FILES ${LUMINANCE_FILES} "${CMAKE_SOURCE_DIR}/program-icons/luminance_ico.rc")

        # Subsystem console:
        ADD_EXECUTABLE(luminance-hdr WIN32 ${LUMINANCE_FILES})
        ADD_EXECUTABLE(luminance-hdr-cli ${LUMINANCE_FILES})

        qt5_use_modules(luminance-hdr Core Gui Widgets WinExtras Svg)
        qt5_use_modules(luminance-hdr-cli Core Gui Widgets)

        # link libraries
        TARGET_LINK_LIBRARIES(luminance-hdr -Xlinker --start-group ${LUMINANCE_MODULES_GUI} -Xlinker --end-group)
        TARGET_LINK_LIBRARIES(luminance-hdr-cli -Xlinker --start-group ${LUMINANCE_MODULES_CLI} -Xlinker --end-group)
        TARGET_LINK_LIBRARIES(luminance-hdr ${LIBS})
        TARGET_LINK_LIBRARIES(luminance-hdr-cli ${LIBS})

        INSTALL(TARGETS luminance-hdr RUNTIME DESTINATION ./) # main GUI exec
        INSTALL(TARGETS luminance-hdr-cli RUNTIME DESTINATION ./) # main CLI exec
        INSTALL(FILES ${LUMINANCE_QM} DESTINATION ./i18n) # i18n
        #INSTALL(FILES ${LUMINANCE_QT_QM} DESTINATION ./i18n) # i18n

        INSTALL(FILES ${QT5_PLUGINS_SRC_DIR}/imageformats/qjpeg.dll DESTINATION ./imageformats/)
        INSTALL(FILES ${QT5_PLUGINS_SRC_DIR}/imageformats/qsvg.dll DESTINATION ./imageformats/)
        INSTALL(FILES ${QT5_PLUGINS_SRC_DIR}/platforms/qwindows.dll DESTINATION ./platforms/)
        INSTALL(FILES ${QT5_PLUGINS_SRC_DIR}/sqldrivers/qsqlite.dll DESTINATION ./sqldrivers/)

        INSTALL(FILES ${CMAKE_SOURCE_DIR}/program-icons/luminance-hdr.png DESTINATION ./icons/hicolor/48x48/apps) # icon
        INSTALL(FILES ${CMAKE_SOURCE_DIR}/AUTHORS
                      ${CMAKE_SOURCE_DIR}/README.md
                      ${CMAKE_SOURCE_DIR}/LICENSE
                      ${CMAKE_SOURCE_DIR}/Changelog
                      DESTINATION ./) #info files
        INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/icons DESTINATION ./) # icons directory
        INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/help DESTINATION ./) # help directory
        INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/hdrhtml DESTINATION ./) # hdrhtml directory
    ELSE()
        # Link libraries
        TARGET_LINK_LIBRARIES(luminance-hdr ${LUMINANCE_MODULES_GUI} ${LIBS})
        TARGET_LINK_LIBRARIES(luminance-hdr-cli ${LUMINANCE_MODULES_CLI} ${LIBS})
    ENDIF()
ENDIF()

# Packaging must go at the bottom
include(${CMAKE_SOURCE_DIR}/build_files/Packaging.cmake)
