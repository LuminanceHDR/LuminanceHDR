# List all .h files in this directory
FILE(GLOB FILES_H *.h)
# List all .cpp files in this directory
FILE(GLOB FILES_CPP *.cpp)

ADD_LIBRARY(pfsfilter ${FILES_H} ${FILES_CPP})
SET(LUMINANCE_MODULES_GUI ${LUMINANCE_MODULES_GUI} pfsfilter PARENT_SCOPE)
SET(LUMINANCE_MODULES_CLI ${LUMINANCE_MODULES_CLI} pfsfilter PARENT_SCOPE)
