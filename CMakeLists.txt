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
install(TARGETS rtprocess DESTINATION /usr/lib)
