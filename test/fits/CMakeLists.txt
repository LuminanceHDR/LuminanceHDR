
QT4_WRAP_CPP(FILES_MOC)

ADD_EXECUTABLE(fits
    fitsMain.cpp
    ${FILES_MOC})

ADD_EXECUTABLE(fits3ch
    fits3chMain.cpp
    ${FILES_MOC})

# Link sub modules
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(fits ${LUMINANCE_MODULES_CLI})
ELSE()
    TARGET_LINK_LIBRARIES(fits -Xlinker --start-group ${LUMINANCE_MODULES_CLI} -Xlinker --end-group)
    TARGET_LINK_LIBRARIES(fits3ch -Xlinker --start-group ${LUMINANCE_MODULES_CLI} -Xlinker --end-group)
ENDIF()
# Link shared library
TARGET_LINK_LIBRARIES(fits ${LIBS})
TARGET_LINK_LIBRARIES(fits3ch ${LIBS})
