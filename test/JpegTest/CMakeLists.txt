ADD_EXECUTABLE(JpegTest JpegTestMain.cpp)

# Link sub modules
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(JpegTest fileformat pfs)
ELSE()
    TARGET_LINK_LIBRARIES(JpegTest -Xlinker --start-group fileformat pfs -Xlinker --end-group)
ENDIF()
# Link shared library
TARGET_LINK_LIBRARIES(JpegTest ${LIBS})
