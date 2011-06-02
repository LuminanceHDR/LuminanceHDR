# - Try to find the Exiv2 library
#
#  EXIV2_MIN_VERSION - You can set this variable to the minimum version you need 
#                      before doing FIND_PACKAGE(Exiv2). The default is 0.12.
# 
# Once done this will define
#
#  EXIV2_FOUND - system has libexiv2
#  EXIV2_INCLUDE_DIR - the libexiv2 include directory
#  EXIV2_LIBRARIES - Link these to use libexiv2
#  EXIV2_DEFINITIONS - Compiler switches required for using libexiv2
#

# Copyright (c) 2008, Gilles Caulier, <caulier.gilles@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


FIND_PATH(EXIV2_INCLUDE_DIR exiv2/exif.hpp
  /usr/local/include
  /usr/include
  /opt/local/include
)

SET(EXIV2_NAMES ${EXIV2_NAMES} exiv2)
FIND_LIBRARY(EXIV2_LIBRARY NAMES ${EXIV2_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set EXIV2_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EXIV2  DEFAULT_MSG  EXIV2_LIBRARY  EXIV2_INCLUDE_DIR)

IF(EXIV2_FOUND)
  SET( EXIV2_LIBRARIES ${EXIV2_LIBRARY} )
  IF(NOT EXIV2_FIND_QUIETLY)
    MESSAGE(STATUS "Found Exiv2 (EXIV2_LIBRARIES = ${EXIV2_LIBRARIES})")
    MESSAGE(STATUS "Found Exiv2 (EXIV2_INCLUDE_DIR = ${EXIV2_INCLUDE_DIR})")
  ENDIF(NOT EXIV2_FIND_QUIETLY)
ELSE(EXIV2_FOUND)
  IF(EXIV2_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find Exiv2")
  ENDIF(EXIV2_FIND_REQUIRED)
ENDIF(EXIV2_FOUND)



MARK_AS_ADVANCED(EXIV2_INCLUDE_DIR EXIV2_LIBRARY)

