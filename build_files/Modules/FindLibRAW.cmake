# - Find LibRAW library
# Find the native LibRAW includes and library
# This module defines
#  LibRAW_INCLUDE_DIR, where to find libraw.h, etc.
#  LibRAW_LIBRARIES, libraries to link against to use LibRAW.
#  LibRAW_FOUND, If false, do not try to use LibRAW.
# also defined, but not for general use are
#  LibRAW_LIBRARY, where to find the LibRAW library.

FIND_PATH(LibRAW_INCLUDE_DIR libraw/libraw.h)

SET(LibRAW_NAMES ${LibRAW_NAMES} raw_r)
FIND_LIBRARY(LibRAW_LIBRARY NAMES ${LibRAW_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set LibRAW_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibRAW  DEFAULT_MSG  LibRAW_LIBRARY  LibRAW_INCLUDE_DIR)

IF(LibRAW_FOUND)
  SET( LibRAW_LIBRARIES ${LibRAW_LIBRARY} )
  MESSAGE(STATUS "Found LibRAW (LibRAW_INCLUDE_DIR = ${LibRAW_INCLUDE_DIR})")
  MESSAGE(STATUS "Found LibRAW (LibRAW_LIBRARIES = ${LibRAW_LIBRARIES})")
  IF(UNIX)
    IF(APPLE)
      EXECUTE_PROCESS(COMMAND ${CMAKE_SOURCE_DIR}/build_files/platforms/macosx/find_demosaicing_gpl2.sh
                    ${LibRAW_LIBRARY} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_VARIABLE _output OUTPUT_STRIP_TRAILING_WHITESPACE)
    ELSE()
      EXECUTE_PROCESS(COMMAND ${CMAKE_SOURCE_DIR}/build_files/platforms/linux/find_demosaicing_gpl2.sh
                    ${LibRAW_LIBRARY} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_VARIABLE _output OUTPUT_STRIP_TRAILING_WHITESPACE)
    ENDIF()
    IF(_output)
      MESSAGE(STATUS "Found demosaicing pack GPL2")
      ADD_DEFINITIONS("-DDEMOSAICING_GPL2")
    ELSE(_output)
      MESSAGE(STATUS "Demosaicing pack GPL2 not found")
    ENDIF(_output)

    IF(APPLE)
      EXECUTE_PROCESS(COMMAND ${CMAKE_SOURCE_DIR}/build_files/platforms/macosx/find_demosaicing_gpl3.sh
                    ${LibRAW_LIBRARY} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_VARIABLE _output OUTPUT_STRIP_TRAILING_WHITESPACE)
    ELSE()
      EXECUTE_PROCESS(COMMAND ${CMAKE_SOURCE_DIR}/build_files/platforms/linux/find_demosaicing_gpl3.sh
                    ${LibRAW_LIBRARY} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_VARIABLE _output OUTPUT_STRIP_TRAILING_WHITESPACE)
    ENDIF()
    IF(_output)
      MESSAGE(STATUS "Found demosaicing pack GPL3")
      ADD_DEFINITIONS("-DDEMOSAICING_GPL3")
    ELSE(_output)
      MESSAGE(STATUS "Demosaicing pack GPL3 not found")
    ENDIF(_output)
  ENDIF(UNIX)
ELSE(LibRAW_FOUND)
  MESSAGE(FATAL_ERROR "Could not find LibRAW")
ENDIF(LibRAW_FOUND)

MARK_AS_ADVANCED(LibRAW_INCLUDE_DIR LibRAW_LIBRARIES)
