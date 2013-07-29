
QT4_WRAP_CPP(FILES_MOC)

ADD_EXECUTABLE(robustAWB
    robustAWBMain.cpp
    ${FILES_MOC})

# Link sub modules
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(robustAWB ${LUMINANCE_MODULES_CLI})
ELSE()
    TARGET_LINK_LIBRARIES(robustAWB -Xlinker --start-group ${LUMINANCE_MODULES_CLI} -Xlinker --end-group)
ENDIF()
# Link shared library
TARGET_LINK_LIBRARIES(robustAWB ${LIBS})
