# - Try to find CCFITS.
# Once executed, this module will define:
# Variables defined by this module:
# CCFITS_FOUND - system has CCFITS
# CCFITS_INCLUDE_DIR - the CCFITS include directory (cached)
# CCFITS_INCLUDE_DIRS - the CCFITS include directories
# (identical to CCFITS_INCLUDE_DIR)
# CCFITS_LIBRARY - the CCFITS library (cached)
# CCFITS_LIBRARIES - the CCFITS libraries
# (identical to CCFITS_LIBRARY)
#
# This module will use the following enviornmental variable
# when searching for CCFITS:
# CCFITS_ROOT_DIR - CCFITS root directory (i.e. where CCfits/ can be found)
#

#
# Copyright (c) 2012 Brian Kloppenborg
#
# This file is part of the C++ OIFITS Library (CCFITS).
#
# CCFITS is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation, either version 3
# of the License, or (at your option) any later version.
#
# CCFITS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with CCFITS. If not, see <http://www.gnu.org/licenses/>.
#

#if(NOT CCFITS_FOUND)

    find_path(CCFITS_INCLUDE_DIR
        NAMES CCfits/CCfits.h
        HINTS $ENV{CCFITS_ROOT_DIR} /opt/local/include
		PATH_SUFFIXES include include/cfitsio /opt/local/include
        DOC "CCFITS include directory.")
        
    find_library(CCFITS_LIBRARY
        NAMES libCCfits.so libCCfits.a libCCfits.dylib libCCfits.la
        HINTS $ENV{CCFITS_ROOT_DIR} /opt/local/lib
        PATH_SUFFIXES lib
        DOC "CCFITS library.")
  
    find_package(CFITSIO REQUIRED)

    mark_as_advanced(CCFITS_INCLUDE_DIR CCFITS_LIBRARY)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(CCFITS DEFAULT_MSG
          CCFITS_LIBRARY CCFITS_INCLUDE_DIR)

    set(CCFITS_INCLUDE_DIRS ${CCFITS_INCLUDE_DIR} ${CFITSIO_INCLUDE_DIR})
    set(CCFITS_LIBRARIES ${CCFITS_LIBRARY} ${CFITSIO_LIBRARY})

#endif(NOT CCFITS_FOUND)

