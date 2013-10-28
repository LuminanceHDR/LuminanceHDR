ADD_EXECUTABLE(ImageInspector
    ImageInspectorMain.cpp)

# Link sub modules
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(ImageInspector pfs)
ELSE()
    TARGET_LINK_LIBRARIES(ImageInspector -Xlinker --start-group pfs -Xlinker --end-group)
ENDIF()
# Link shared library
TARGET_LINK_LIBRARIES(ImageInspector ${LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})
