SET(LIBPFS_H
        vex.h
        array2d.h
        channel.h
        colorspace.h
        domio.h
        frame.h
        pfs.h
        tag.h
)

SET(LIBPFS_CPP
        vex.cpp
        array2d.cpp
        channel.cpp
        colorspace.cpp
        domio.cpp
        frame.cpp
#	pfs.cpp
        tag.cpp
)

ADD_LIBRARY(pfs ${LIBPFS_H} ${LIBPFS_CPP})
