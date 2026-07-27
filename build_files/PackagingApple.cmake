set(CPACK_PACKAGE_CONTACT "Davide Anastasia <davideanastasia@users.sourceforge.net>")

SET(plugin_dest_dir ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/PlugIns)
SET(qtconf_dest_dir ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources)
SET(APPS "\${CMAKE_INSTALL_PREFIX}/${LHDR_OSX_EXECUTABLE_NAME}.app")

# Install the caller-supplied native helper when release packaging is enabled.
if(LHDR_ALIGN_IMAGE_STACK)
    install(FILES ${LHDR_ALIGN_IMAGE_STACK}
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE WORLD_EXECUTE
            DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/MacOS
            RENAME align_image_stack)
endif()

# install help
install(DIRECTORY ${CMAKE_SOURCE_DIR}/help
        DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources)

#install icons theme
install(DIRECTORY ${CMAKE_SOURCE_DIR}/icons
        DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources)

#install HdrHTML stuff
install(DIRECTORY ${CMAKE_SOURCE_DIR}/hdrhtml
        DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources)

# install a qt.conf file
install(FILES ${CMAKE_SOURCE_DIR}/build_files/platforms/macosx/qt.conf
        DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources)

# Install documentation as resources so codesign does not classify it as code.
install(FILES
        ${CMAKE_SOURCE_DIR}/AUTHORS
        ${CMAKE_SOURCE_DIR}/README.md
        ${CMAKE_SOURCE_DIR}/LICENSE
        ${CMAKE_SOURCE_DIR}/Changelog
        DESTINATION ${LHDR_OSX_EXECUTABLE_NAME}.app/Contents/Resources/Documentation)

# directories to look for dependencies
set(DIRS ${QT_LIBRARY_DIRS})

# That's all Folks!
##
