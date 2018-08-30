cmake_minimum_required(VERSION 2.8.9)
project(rtprocess)
set(CMAKE_BUILD_TYPE Release)

#bring headers into the project
include_directories(src)

#get source files
file(GLOB SOURCES "src/*.cc")

#generate shared library
add_library(rtprocess SHARED ${SOURCES})

#set location for library installation
if(WIN32)
install(TARGETS rtprocess DESTINATION lib)
install(FILES "src/array2D.h" DESTINATION include/rtprocess)
install(FILES "src/noncopyable.h" DESTINATION include/rtprocess)
install(FILES "src/rawimagesource.h" DESTINATION include/rtprocess)
else()
install(TARGETS rtprocess DESTINATION /usr/lib)
install(FILES "src/array2D.h" DESTINATION /usr/include/rtprocess)
install(FILES "src/noncopyable.h" DESTINATION /usr/include/rtprocess)
install(FILES "src/rawimagesource.h" DESTINATION /usr/include/rtprocess)
endif()
