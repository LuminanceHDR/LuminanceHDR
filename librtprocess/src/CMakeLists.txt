find_package(PkgConfig)

if (OPTION_OMP)
    find_package(OpenMP REQUIRED COMPONENTS CXX)
endif()

set(rtprocess_SRCS
    demosaic/ahd.cc
    demosaic/amaze.cc
    demosaic/bayerfast.cc
    demosaic/border.cc
    demosaic/dcb.cc
    demosaic/hphd.cc
    demosaic/igv.cc
    demosaic/lmmse.cc
    demosaic/markesteijn.cc
    demosaic/rcd.cc
    demosaic/vng4.cc
    demosaic/xtransfast.cc
    preprocess/CA_correct.cc
    postprocess/hilite_recon.cc)

add_library(rtprocess SHARED ${rtprocess_SRCS})
target_include_directories(rtprocess
                           PRIVATE
                               ${CMAKE_CURRENT_SOURCE_DIR}/include
                          )
target_compile_options(rtprocess
                       PRIVATE
                           ${OpenMP_CXX_FLAGS}
                           ${DEFAULT_CXX_COMPILE_FLAGS}
                           ${OpenMP_CXX_FLAGS}
                           ${PROC_FLAGS})
set_property(TARGET
                 rtprocess
             PROPERTY
                 LINKER_FLAGS "${DEFAULT_LINK_FLAGS}"
             PROPERTY
                 PUBLIC_HEADER "include/librtprocess.h")

if (OpenMP_FOUND)
    target_link_libraries(rtprocess PRIVATE ${OpenMP_CXX_LIBRARIES})
endif()

if (VERBOSE)
    target_compile_options(rtprocess
                           PRIVATE
                               -DVERBOSE)
endif()

install(TARGETS rtprocess
        ARCHIVE
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT ${PROJECT_NAME}
        RUNTIME
            DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rtprocess)

if (WITH_STATIC_LIB)
    add_library(rtprocess_static STATIC ${rtprocess_SRCS})
    target_include_directories(rtprocess_static
                               PRIVATE
                                   ${CMAKE_CURRENT_SOURCE_DIR}/include
                                   )
    target_compile_options(rtprocess_static
                           PRIVATE
                               ${OpenMP_CXX_FLAGS}
                               ${DEFAULT_CXX_COMPILE_FLAGS}
                               ${OpenMP_CXX_FLAGS}
                               ${PROC_FLAGS})
    set_property(TARGET
                     rtprocess_static
                 PROPERTY
                     LINKER_FLAGS "${DEFAULT_LINK_FLAGS}"
                 PROPERTY
                     PUBLIC_HEADER "include/librtprocess.h")

    if (OpenMP_FOUND)
        target_link_libraries(rtprocess_static
                              PRIVATE
                                  ${OpenMP_CXX_LIBRARIES})
    endif()

    if (VERBOSE)
        target_compile_options(rtprocess_static
                               PRIVATE
                                   -DVERBOSE)
    endif()

    install(TARGETS rtprocess_static
            ARCHIVE
                DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY
                DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT ${PROJECT_NAME}
            RUNTIME
                DESTINATION ${CMAKE_INSTALL_BINDIR}
            PUBLIC_HEADER
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rtprocess)
endif()
