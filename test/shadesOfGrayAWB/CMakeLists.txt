
QT4_WRAP_CPP(FILES_MOC)

ADD_EXECUTABLE(shadesOfGrayAWB
    shadesOfGrayAWBMain.cpp
    ${FILES_MOC})

# Link sub modules
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(shadesOfGrayAWB ${LUMINANCE_MODULES_CLI})
ELSE()
    TARGET_LINK_LIBRARIES(shadesOfGrayAWB -Xlinker --start-group ${LUMINANCE_MODULES_CLI} -Xlinker --end-group)
ENDIF()
# Link shared library
TARGET_LINK_LIBRARIES(shadesOfGrayAWB ${LIBS})
