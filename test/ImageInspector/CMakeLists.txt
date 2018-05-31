ADD_EXECUTABLE(ImageInspector
    ImageInspectorMain.cpp)

QT5_WRAP_CPP(FILES_MOC ${FILES_H})

TARGET_LINK_LIBRARIES(ImageInspector Qt5::Core Qt5::Gui Qt5::Widgets)

# Link sub modules
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(ImageInspector pfs)
ELSE()
    TARGET_LINK_LIBRARIES(ImageInspector -Xlinker --start-group pfs -Xlinker --end-group)
ENDIF()
# Link shared library
TARGET_LINK_LIBRARIES(ImageInspector ${LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})
