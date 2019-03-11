configure_file(rtprocess.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/rtprocess.pc)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rtprocess.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        COMPONENT pkgconfig)
