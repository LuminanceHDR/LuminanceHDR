ADD_EXECUTABLE(ImageInspector
    ImageInspectorMain.cpp)

QT5_WRAP_CPP(FILES_MOC ${FILES_H})

qt5_use_modules(ImageInspector Core Gui Widgets)

# Link sub modules
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(ImageInspector pfs)
ELSE()
    TARGET_LINK_LIBRARIES(ImageInspector -Xlinker --start-group pfs -Xlinker --end-group)
ENDIF()
# Link shared library
TARGET_LINK_LIBRARIES(ImageInspector ${LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})
