list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

### Add defaults for cmake
# These defaults need to be included before the project() call.
include(DefineCMakeDefaults)

# This will provide -DCMAKE_BUILD_TYPE=Profiling
# and -DCMAKE_BUILD_TYPE=AddressSanitizer
include(DefineCompilerFlags)

# include cmake files
include(DefineOptions.cmake)

add_subdirectory(src)

ADD_LIBRARY(librtprocess STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/demosaic/ahd.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/demosaic/amaze.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/demosaic/bayerfast.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/demosaic/border.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/demosaic/dcb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/demosaic/hphd.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/demosaic/igv.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/demosaic/lmmse.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/demosaic/markesteijn.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/demosaic/rcd.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/demosaic/vng4.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/demosaic/xtransfast.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/postprocess/hilite_recon.cc)

SET(LUMINANCE_MODULES_CLI ${LUMINANCE_MODULES_CLI} librtprocess PARENT_SCOPE)
SET(LUMINANCE_MODULES_GUI ${LUMINANCE_MODULES_GUI} librtprocess PARENT_SCOPE)
