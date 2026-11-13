ADD_EXECUTABLE(ImageInspector
    ImageInspectorMain.cpp)

qt_wrap_cpp(FILES_MOC ${FILES_H})

TARGET_LINK_LIBRARIES(ImageInspector Qt6::Core Qt6::Gui Qt6::Widgets)

# Link sub modules
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(ImageInspector pfs)
ELSE()
    TARGET_LINK_LIBRARIES(ImageInspector -Xlinker --start-group pfs -Xlinker --end-group)
ENDIF()
# Link shared library
TARGET_LINK_LIBRARIES(ImageInspector ${LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})
