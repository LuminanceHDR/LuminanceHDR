# For help take a look at:
# http://www.cmake.org/Wiki/CMake:CPackConfiguration

### general settings
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A project to make RawTherapee's processing algorithms more readily available.")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_VENDOR "RawTherapee")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")


### versions
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})


### source generator
set(CPACK_SOURCE_GENERATOR "TXZ")
set(CPACK_SOURCE_IGNORE_FILES "~$;[.]swp$;/[.]svn/;/[.]git/;.gitignore;/obj*;tags;cscope.*;.ycm_extra_conf.pyc")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

if (WIN32)
    set(CPACK_GENERATOR "ZIP")

    ### nsis generator
    find_package(NSIS)
    if (NSIS_MAKE)
        set(CPACK_GENERATOR "${CPACK_GENERATOR};NSIS")
        set(CPACK_NSIS_DISPLAY_NAME "CMocka")
        set(CPACK_NSIS_COMPRESSOR "/SOLID zlib")
        set(CPACK_NSIS_MENU_LINKS "https://github.com/CarVac/librtprocess" "librtprocess homepage")
    endif (NSIS_MAKE)
endif (WIN32)

set(CPACK_PACKAGE_INSTALL_DIRECTORY "rtprocess")

set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${CPACK_PACKAGE_VERSION})

set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C/C++ Headers")
set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION
  "Libraries used to build programs which use rtprocess")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION
  "C/C++ header files for use with rtprocess")
set(CPACK_COMPONENT_HEADERS_DEPENDS libraries)
set(CPACK_COMPONENT_LIBRARIES_GROUP "Development")
set(CPACK_COMPONENT_HEADERS_GROUP "Development")

include(CPack)
