# List all .h files in this directory
FILE(GLOB FILES_H *.h)
# List all .cpp files in this directory
FILE(GLOB FILES_CPP *.cpp)

# add all .h files to the _H module variable
SET(TM_LIBPFS_H ${TM_LIBPFS_H} ${FILES_H} PARENT_SCOPE)
# add all .cpp files to the _CPP module variable
SET(TM_LIBPFS_CPP ${TM_LIBPFS_CPP} ${FILES_CPP} PARENT_SCOPE)
