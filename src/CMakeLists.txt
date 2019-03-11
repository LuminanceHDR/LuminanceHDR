find_package(PkgConfig)

pkg_check_modules(GLIBMM glibmm-2.4)

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
    preprocess/CA_correct.cc)

add_library(rtprocess SHARED ${rtprocess_SRCS})
target_include_directories(rtprocess
                           PRIVATE
                               ${CMAKE_CURRENT_SOURCE_DIR}/include
                               ${GLIBMM_INCLUDE_DIRS})
target_compile_options(rtprocess
                       PRIVATE
                           ${OpenMP_CXX_FLAGS})
set_property(TARGET
                 rtprocess
             PROPERTY
                 PUBLIC_HEADER "include/librtprocess.h")

target_link_libraries(rtprocess PRIVATE ${GLIBMM_LIBRARIES})

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
