# Required cmake version
cmake_minimum_required(VERSION 3.5.0)
cmake_policy(SET CMP0048 NEW)

# Specify search path for CMake modules to be loaded by include()
# and find_package()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

### Add defaults for cmake
# These defaults need to be included before the project() call.
include(DefineCMakeDefaults)

# This will provide -DCMAKE_BUILD_TYPE=Profiling
# and -DCMAKE_BUILD_TYPE=AddressSanitizer
include(DefineCompilerFlags)

project(rtprocess VERSION 0.9.0 LANGUAGES CXX)

# include cmake files
include(DefineOptions.cmake)
include(GNUInstallDirs)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++11")

# Stop compilation on typos such as std:swap (missing colon will be detected as unused label):
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=unused-label -O3")

# By default we don't use a specific processor target, so PROC_TARGET_NUMBER is set to 0.
# Specify other values to optimize for specific processor architecture as listed in ProcessorTargets.cmake:
set(PROC_TARGET_NUMBER 0 CACHE STRING "Selected target processor from the list taken from ProcessorTargets.cmake")
# Loads the ProcessorTargets list:
include(ProcessorTargets.cmake)
set(PROC_FLAGS "" CACHE STRING "Target processor related build/link flags")
if(NOT(PROC_TARGET_NUMBER EQUAL 0))
    set(PROC_FLAGS ${PROC_TARGET_${PROC_TARGET_NUMBER}_FLAGS})
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PROC_FLAGS}")

add_subdirectory(src)

#create rtprocess.pc
#file(READ pkgconfig/rtprocessin.pc RTPROCESSIN)
#file(WRITE pkgconfig/rtprocess.pc "prefix=/${PC_PREFIX}")
#file(APPEND pkgconfig/rtprocess.pc ${RTPROCESSIN})
