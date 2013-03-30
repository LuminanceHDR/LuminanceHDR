SET(FILES_H ImageInspector.h)

QT4_WRAP_CPP(FILES_MOC ${FILES_H})

ADD_EXECUTABLE(ImageInspector
    ImageInspectorMain.cpp
    ImageInspector.cpp
    ImageInspectorStats.cpp
    ImageInspectorStats.h
    ${FILES_MOC} ${FILES_H})

# Link sub modules
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(ImageInspector ${LUMINANCE_MODULES_CLI})
ELSE()
    TARGET_LINK_LIBRARIES(ImageInspector -Xlinker --start-group ${LUMINANCE_MODULES_CLI} -Xlinker --end-group)
ENDIF()
# Link shared library
TARGET_LINK_LIBRARIES(ImageInspector ${LIBS})
