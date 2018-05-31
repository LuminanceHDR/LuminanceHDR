
QT5_WRAP_CPP(FILES_MOC)

ADD_EXECUTABLE(WhiteBalance
    WhiteBalanceMain.cpp
    ${FILES_MOC})

TARGET_LINK_LIBRARIES(WhiteBalance Qt5::Core Qt5::Gui Qt5::Widgets)

# Link sub modules
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(WhiteBalance ${LUMINANCE_MODULES_CLI})
ELSE()
    TARGET_LINK_LIBRARIES(WhiteBalance -Xlinker --start-group ${LUMINANCE_MODULES_CLI} -Xlinker --end-group)
ENDIF()
# Link shared library
TARGET_LINK_LIBRARIES(WhiteBalance ${LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})
