ADD_EXECUTABLE(InputOutputTest InputOutputMain.cpp)

# Link sub modules
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(InputOutputTest fileformat pfs)
ELSE()
    TARGET_LINK_LIBRARIES(InputOutputTest -Xlinker --start-group fileformat pfs -Xlinker --end-group)
ENDIF()
# Link shared library
TARGET_LINK_LIBRARIES(InputOutputTest
    ${LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})
