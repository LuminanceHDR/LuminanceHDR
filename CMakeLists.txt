cmake_minimum_required(VERSION 2.8.9)
project(rtprocess)
set(CMAKE_BUILD_TYPE Release)

option(OPTION_OMP "Build with OpenMP support" ON)

if(OPTION_OMP)
    find_package(OpenMP)
    if(OPENMP_FOUND)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} -Werror=unknown-pragmas")
    endif()
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++11")

# Stop compilation on typos such as std:swap (missing colon will be detected as unused label):
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=unused-label -O3")

option(VERBOSE "Build in verbose mode" OFF)
if(VERBOSE)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVERBOSE")
endif()

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

#bring headers into the project
include_directories(src/include)

#get source files
file(GLOB_RECURSE SOURCES "src/*.cc")

if(WIN32)
    set(INSTALL_PREFIX "")
    set(PC_PREFIX "mingw64")
    set(LIBRARY_TYPE "STATIC")
elseif(APPLE)
    set(INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/)
    set(PC_PREFIX ${CMAKE_INSTALL_PREFIX}/)
    set(LIBRARY_TYPE "SHARED")
else()
    set(INSTALL_PREFIX "/usr/")
    set(PC_PREFIX "usr")
    set(LIBRARY_TYPE "SHARED")
endif()

#create rtprocess.pc
file(READ pkgconfig/rtprocessin.pc RTPROCESSIN)
file(WRITE pkgconfig/rtprocess.pc "prefix=/${PC_PREFIX}")
file(APPEND pkgconfig/rtprocess.pc ${RTPROCESSIN})



#set location for library installation
add_library(rtprocess ${LIBRARY_TYPE} ${SOURCES})
install(TARGETS rtprocess DESTINATION ${INSTALL_PREFIX}lib)
install(FILES "pkgconfig/rtprocess.pc" DESTINATION ${INSTALL_PREFIX}lib/pkgconfig)
install(FILES "src/include/librtprocess.h" DESTINATION ${INSTALL_PREFIX}include/rtprocess)
