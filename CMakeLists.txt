# Required cmake version
cmake_minimum_required(VERSION 3.5.0)
cmake_policy(SET CMP0048 NEW)

if (NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()
# Specify search path for CMake modules to be loaded by include()
# and find_package()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

### Add defaults for cmake
# These defaults need to be included before the project() call.
include(DefineCMakeDefaults)

# This will provide -DCMAKE_BUILD_TYPE=Profiling
# and -DCMAKE_BUILD_TYPE=AddressSanitizer
include(DefineCompilerFlags)

project(rtprocess VERSION 0.11.0 LANGUAGES CXX)

# include cmake files
include(DefineOptions.cmake)
include(CompilerChecks.cmake)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CPackConfig.cmake)

# By default we don't use a specific processor target, so PROC_TARGET_NUMBER is set to 0.
# Specify other values to optimize for specific processor architecture as listed in ProcessorTargets.cmake:
set(PROC_TARGET_NUMBER 0 CACHE STRING "Selected target processor from the list taken from ProcessorTargets.cmake")
# Loads the ProcessorTargets list:
include(ProcessorTargets.cmake)
set(PROC_FLAGS "" CACHE STRING "Target processor related build/link flags")
if(NOT(PROC_TARGET_NUMBER EQUAL 0))
    set(PROC_FLAGS ${PROC_TARGET_${PROC_TARGET_NUMBER}_FLAGS})
endif()

add_subdirectory(src)
add_subdirectory(pkgconfig)

# cmake config files
if (WIN32)
    set(RTPROCESS_LIBRARY_NAME ${CMAKE_SHARED_LIBRARY_PREFIX}${PROJECT_NAME}${CMAKE_IMPORT_LIBRARY_SUFFIX})
else()
    set(RTPROCESS_LIBRARY_NAME ${CMAKE_SHARED_LIBRARY_PREFIX}${PROJECT_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
endif()

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE INTERNAL "")
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR} CACHE INTERNAL "")
configure_package_config_file(${PROJECT_NAME}-config.cmake.in
                              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
                              PATH_VARS
                                  INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
                              INSTALL_DESTINATION
                                  ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

write_basic_package_version_file(${PROJECT_NAME}-config-version.cmake
                                 COMPATIBILITY
                                     AnyNewerVersion)

install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
        DESTINATION
            ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
        COMPONENT
            devel)

# Add 'make dist' target which makes sure to invoke cmake before
add_custom_target(dist
                  COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
