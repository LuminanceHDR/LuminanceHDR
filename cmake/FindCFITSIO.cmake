# - Try to find CFITSIO
# Once done this will define
#
#  CFITSIO_FOUND - system has CFITSIO
#  CFITSIO_INCLUDE_DIR - the CFITSIO include directory
#  CFITSIO_LIBRARIES - Link these to use CFITSIO
#  CFITSIO_VERSION_STRING - Human readable version number of cfitsio
#  CFITSIO_VERSION_MAJOR  - Major version number of cfitsio
#  CFITSIO_VERSION_MINOR  - Minor version number of cfitsio
#
# Redistribution and use is allowed according to the terms of the BSD license.

find_path(CFITSIO_INCLUDE_DIR fitsio.h
    PATH_SUFFIXES libcfitsio3 libcfitsio0 cfitsio
    PATHS
    $ENV{CFITSIO}
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
    /opt/local/include
)

find_library(CFITSIO_LIBRARIES NAMES cfitsio
    PATHS
    $ENV{CFITSIO}
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
    /opt/local/lib
)

if(CFITSIO_INCLUDE_DIR AND CFITSIO_LIBRARIES)
    # Find the version of the cfitsio header
    FILE(READ "${CFITSIO_INCLUDE_DIR}/fitsio.h" FITSIO_H)
    STRING(REGEX REPLACE ".*#define CFITSIO_VERSION[^0-9]*([0-9]+)\\.([0-9]+).*" "\\1.\\2" CFITSIO_VERSION_STRING "${FITSIO_H}")
    STRING(REGEX REPLACE "^([0-9]+)[.]([0-9]+)" "\\1" CFITSIO_VERSION_MAJOR ${CFITSIO_VERSION_STRING})
    STRING(REGEX REPLACE "^([0-9]+)[.]([0-9]+)" "\\2" CFITSIO_VERSION_MINOR ${CFITSIO_VERSION_STRING})

    SET(CFITSIO_VERSION_STRING "${CFITSIO_VERSION_MAJOR}.${CFITSIO_VERSION_MINOR}")
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(CFITSIO
    REQUIRED_VARS CFITSIO_LIBRARIES CFITSIO_INCLUDE_DIR CFITSIO_VERSION_STRING)
mark_as_advanced(CFITSIO_INCLUDE_DIR CFITSIO_LIBRARIES)
