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
  IF(UNIX)
    IF(APPLE)
      EXECUTE_PROCESS(COMMAND ${CMAKE_SOURCE_DIR}/build/macosx/find_demosaicing_gpl2.sh
                    ${LIBRAW_LIBRARY} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_VARIABLE _output OUTPUT_STRIP_TRAILING_WHITESPACE)
    ELSE()
      EXECUTE_PROCESS(COMMAND ${CMAKE_SOURCE_DIR}/build/linux/find_demosaicing_gpl2.sh
                    ${LIBRAW_LIBRARY} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_VARIABLE _output OUTPUT_STRIP_TRAILING_WHITESPACE)
    ENDIF()
    IF(_output)
      MESSAGE(STATUS "Found demosaicing pack GPL2")
      ADD_DEFINITIONS("-DDEMOSAICING_GPL2")
    ELSE(_output)
      MESSAGE(STATUS "Demosaicing pack GPL2 not found")
    ENDIF(_output)

    IF(APPLE)
      EXECUTE_PROCESS(COMMAND ${CMAKE_SOURCE_DIR}/build/macosx/find_demosaicing_gpl3.sh
                    ${LIBRAW_LIBRARY} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_VARIABLE _output OUTPUT_STRIP_TRAILING_WHITESPACE)
    ELSE()
      EXECUTE_PROCESS(COMMAND ${CMAKE_SOURCE_DIR}/build/linux//find_demosaicing_gpl3.sh
                    ${LIBRAW_LIBRARY} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_VARIABLE _output OUTPUT_STRIP_TRAILING_WHITESPACE)
    ENDIF()
    IF(_output)
      MESSAGE(STATUS "Found demosaicing pack GPL3")
      ADD_DEFINITIONS("-DDEMOSAICING_GPL3")
    ELSE(_output)
      MESSAGE(STATUS "Demosaicing pack GPL3 not found")
    ENDIF(_output)
  ENDIF(UNIX)
ELSE(LIBRAW_FOUND)
  MESSAGE(FATAL_ERROR "Could not find LibRAW")
ENDIF(LIBRAW_FOUND)

MARK_AS_ADVANCED(LIBRAW_INCLUDE_DIR LIBRAW_LIBRARIES)
