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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

# Stop compilation on typos such as std:swap (missing colon will be detected as unused label):
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=unused-label")

#bring headers into the project
include_directories(src)

#get source files
file(GLOB_RECURSE SOURCES "src/*.cc")

#set location for library installation
if(WIN32)
#generate static library
add_library(rtprocess STATIC ${SOURCES})
install(TARGETS rtprocess DESTINATION lib)
install(FILES "pkgconfig/msys2/rtprocess.pc" DESTINATION lib/pkgconfig)
install(FILES "src/librtprocess.h" DESTINATION include/rtprocess)
else()
#generate shared library
add_library(rtprocess SHARED ${SOURCES})
install(TARGETS rtprocess DESTINATION /usr/lib)
install(FILES "pkgconfig/linux/rtprocess.pc" DESTINATION /usr/lib/pkgconfig)
install(FILES "src/librtprocess.h" DESTINATION /usr/include/rtprocess)
endif()
