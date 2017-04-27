project("LuminanceHDR External Dependencies")
cmake_minimum_required(VERSION 3.5)
####################################################################################################
#
# WINDOWS USAGE:
#   Don't call this cmake file your self, use the build_deps batch file
#   build_deps 2013 x64 / build_deps 2013 x86
#   build_deps 2015 x64 / build_deps 2015 x86
#
#   Note this currently only builds 64 bit versions of the dependencies, 32 bit is not supported or tested
#
# MAC OS X USAGE:
#	Install autotools and yasm, for example with homebrew:
#		brew install autoconf automake libtool yasm
#	Build with cmake as usual
#
####################################################################################################

include(ExternalProject)
include(cmake/boost.cmake)
