# - Find LibRAW library
# Find the native LibRAW includes and library
# This module defines
#  LIBRAW_INCLUDE_DIR, where to find libraw.h, etc.
#  LIBRAW_LIBRARIES, libraries to link against to use LibRAW.
#  LIBRAW_FOUND, If false, do not try to use LibRAW.
# also defined, but not for general use are
#  LIBRAW_LIBRARY, where to find the LibRAW library.

FIND_PATH(LIBRAW_INCLUDE_DIR libraw/libraw.h)

SET(LIBRAW_NAMES ${LIBRAW_NAMES} raw_r)
FIND_LIBRARY(LIBRAW_LIBRARY NAMES ${LIBRAW_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set LIBRAW_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBRAW  DEFAULT_MSG  LIBRAW_LIBRARY  LIBRAW_INCLUDE_DIR)

IF(LIBRAW_FOUND)
  SET( LIBRAW_LIBRARIES ${LIBRAW_LIBRARY} )
  MESSAGE(STATUS "Found LibRAW (LIBRAW_INCLUDE_DIR = ${LIBRAW_INCLUDE_DIR})")
  MESSAGE(STATUS "Found LibRAW (LIBRAW_LIBRARIES = ${LIBRAW_LIBRARIES})")
ELSE(LIBRAW_FOUND)
  MESSAGE(FATAL_ERROR "Could not find LibRAW")
ENDIF(LIBRAW_FOUND)

MARK_AS_ADVANCED(LIBRAW_INCLUDE_DIR LIBRAW_LIBRARIES)
