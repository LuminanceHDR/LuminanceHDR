set(CPACK_PACKAGE_CONTACT "Davide Anastasia <davideanastasia@users.sourceforge.net>")
# set(CPACK_GENERATOR "DragNDrop")
# set(CPACK_DMG_FORMAT "UDBZ")
# set(CPACK_DMG_VOLUME_NAME "${LHDR_NAME} ${LHDR_VERSION}")
# set(CPACK_SYSTEM_NAME "OSX")
# set(CPACK_PACKAGE_FILE_NAME "${LHDR_NAME}-${LHDR_VERSION}")
# set(CPACK_PACKAGE_ICON "${ICONS_DIR}/DMG.icns")
# set(CPACK_DMG_DS_STORE "${ICONS_DIR}/DMGDSStore")
# set(CPACK_DMG_BACKGROUND_IMAGE "${ICONS_DIR}/DMGBackground.png")

SET(plugin_dest_dir ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/PlugIns)
SET(qtconf_dest_dir ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources)
SET(APPS "\${CMAKE_INSTALL_PREFIX}/${LHDR_OSX_EXECUTABLE_NAME}.app")

#--------------------------------------------------------------------------------
# Install needed Qt plugins by copying directories from the qt installation
# One can cull what gets copied by using 'REGEX "..." EXCLUDE'
#install(DIRECTORY "${QT_PLUGINS_DIR}/sqldrivers"
#        DESTINATION ${plugin_dest_dir} COMPONENT Runtime)
#install(FILES ${LIBS}
#        DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/MacOS)

# install align_image_stack
install(FILES ${CMAKE_SOURCE_DIR}/build/macosx/align_image_stack
        DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/MacOS)

# install help
install(DIRECTORY ${CMAKE_SOURCE_DIR}/help
        DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/html
        DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources)

# install a qt.conf file
install(FILES ${CMAKE_SOURCE_DIR}/build/macosx/qt.conf
        DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources)

# install README and other files
install(FILES
        ${CMAKE_SOURCE_DIR}/AUTHORS
        ${CMAKE_SOURCE_DIR}/README.md
        ${CMAKE_SOURCE_DIR}/LICENSE
        ${CMAKE_SOURCE_DIR}/Changelog
#        ${CMAKE_SOURCE_DIR}/BUGS
#        ${CMAKE_SOURCE_DIR}/INSTALL
#        ${CMAKE_SOURCE_DIR}/TODO
        DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app)

# directories to look for dependencies
set(DIRS ${QT_LIBRARY_DIRS})

#INSTALL(CODE "
#    file(GLOB_RECURSE QTPLUGINS
#      \"\${CMAKE_INSTALL_PREFIX}/${plugin_dest_dir}/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
#    include(BundleUtilities)
#    fixup_bundle(\"${APPS}\" \"\${QTPLUGINS}\" \"${DIRS}\")
#    " COMPONENT Runtime)

# create drag and drop installer
#set(CPACK_BINARY_DRAGNDROP ON)
#include(CPack)
