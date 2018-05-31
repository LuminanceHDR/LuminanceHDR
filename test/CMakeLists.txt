find_package(Boost COMPONENTS program_options REQUIRED)

ADD_SUBDIRECTORY(ImageInspector)
ADD_SUBDIRECTORY(InputOutputTest)
ADD_SUBDIRECTORY(FusionAlgorithms)
ADD_SUBDIRECTORY(WhiteBalance)

# workaround for http://code.google.com/p/googletest/issues/detail?id=408
IF(MSVC_VERSION EQUAL 1700)
ADD_DEFINITIONS(-D_VARIADIC_MAX=10)
ENDIF()

FIND_PACKAGE(GTest)

IF(GTEST_FOUND)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIRS})

IF(UNIX)
FIND_PACKAGE(Threads)
ENDIF(UNIX)

# Colorspace test
# LinearRGB -> XYZ / Y
ADD_EXECUTABLE(TestRGB2XYZ TestRGB2XYZ.cpp)
TARGET_LINK_LIBRARIES(TestRGB2XYZ pfs PrintArray2D
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
ADD_TEST(TestRGB2XYZ TestRGB2XYZ)

# SRGB -> XYZ / Y
ADD_EXECUTABLE(TestSRGB2XYZ TestSRGB2XYZ.cpp)
TARGET_LINK_LIBRARIES(TestSRGB2XYZ pfs PrintArray2D
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
ADD_TEST(TestSRGB2XYZ TestSRGB2XYZ)

ADD_EXECUTABLE(TestXYZ2RGB TestXYZ2RGB.cpp)
TARGET_LINK_LIBRARIES(TestXYZ2RGB pfs PrintArray2D
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
ADD_TEST(TestXYZ2RGB TestXYZ2RGB)

ADD_EXECUTABLE(TestCMYK2RGB TestCMYK2RGB.cpp)
TARGET_LINK_LIBRARIES(TestCMYK2RGB PrintArray2D
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
ADD_TEST(TestCMYK2RGB TestCMYK2RGB)

ADD_LIBRARY(ContrastDomain STATIC
    mantiuk06/contrast_domain.cpp
    mantiuk06/contrast_domain.h)

ADD_LIBRARY(PrintArray2D STATIC
    PrintArray2D.cpp PrintArray2D.h)

TARGET_LINK_LIBRARIES(ContrastDomain Qt5::Core)


# Mantiuk06
ADD_EXECUTABLE(TestMantiuk06Basic
    TestMantiuk06Basic.cpp)
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(TestMantiuk06Basic
        ContrastDomain pfs pfstmo common
        ${GTEST_BOTH_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBS} )
ELSE()
    TARGET_LINK_LIBRARIES(TestMantiuk06Basic
        ContrastDomain pfs pfstmo common
        ${GTEST_BOTH_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBS} -lrt)
ENDIF()
TARGET_LINK_LIBRARIES(TestMantiuk06Basic Qt5::Core)

ADD_TEST(TestMantiuk06Basic TestMantiuk06Basic)

ADD_EXECUTABLE(TestMantiuk06Pyramid
    TestMantiuk06Pyramid.cpp
)
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(TestMantiuk06Pyramid
        ContrastDomain pfs pfstmo common
        ${GTEST_BOTH_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBS} )
ELSE()
    TARGET_LINK_LIBRARIES(TestMantiuk06Pyramid
        ContrastDomain pfs pfstmo common
        ${GTEST_BOTH_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBS} -lrt)
ENDIF()
TARGET_LINK_LIBRARIES(TestMantiuk06Pyramid Qt5::Core)
ADD_TEST(TestMantiuk06Pyramid TestMantiuk06Pyramid)

ADD_EXECUTABLE(TestVex TestVex.cpp)
TARGET_LINK_LIBRARIES(TestVex
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
ADD_TEST(TestVex TestVex)

ADD_EXECUTABLE(TestVexDotProduct TestVexDotProduct.cpp)
TARGET_LINK_LIBRARIES(TestVexDotProduct
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
ADD_TEST(TestVexDotProduct TestVexDotProduct)

ADD_EXECUTABLE(TestPfsRotate TestPfsRotate.cpp CompareVector.h)
TARGET_LINK_LIBRARIES(TestPfsRotate pfs
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
ADD_TEST(TestPfsRotate TestPfsRotate)

ADD_EXECUTABLE(TestPfsShift TestPfsShift.cpp)
TARGET_LINK_LIBRARIES(TestPfsShift pfs
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
ADD_TEST(TestPfsShift TestPfsShift)

ADD_EXECUTABLE(TestConvertSample TestConvertSample.cpp)
TARGET_LINK_LIBRARIES(TestConvertSample PrintArray2D
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
ADD_TEST(TestConvertSample TestConvertSample)

ADD_EXECUTABLE(TestPfsCut TestPfsCut.cpp SeqInt.h)
TARGET_LINK_LIBRARIES(TestPfsCut pfs PrintArray2D
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
ADD_TEST(TestPfsCut TestPfsCut)

ADD_EXECUTABLE(TestFrameArray2D TestFrameArray2D.cpp)
TARGET_LINK_LIBRARIES(TestFrameArray2D pfs
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LIBS})
ADD_TEST(TestFrameArray2D TestFrameArray2D)

ADD_EXECUTABLE(TestFloatRgb TestFloatRgb.cpp)
TARGET_LINK_LIBRARIES(TestFloatRgb common fileformat pfs
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LIBS})
ADD_TEST(TestFloatRgb TestFloatRgb)
TARGET_LINK_LIBRARIES(TestFloatRgb Qt5::Core Qt5::Gui Qt5::Widgets)

ADD_EXECUTABLE(TestMTB TestMTB.cpp)
TARGET_LINK_LIBRARIES(TestMTB common pfs hdrcreation
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LIBS})
ADD_TEST(TestMTB TestMTB)

ADD_EXECUTABLE(TestMinMax TestMinMax.cpp)
TARGET_LINK_LIBRARIES(TestMinMax ${GTEST_BOTH_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
ADD_TEST(TestMinMax TestMinMax)

ADD_EXECUTABLE(TestImageQualityDialog TestImageQualityDialog.cpp)
TARGET_LINK_LIBRARIES(TestImageQualityDialog ui fileformat pfs common ${LIBS})
TARGET_LINK_LIBRARIES(TestImageQualityDialog Qt5::Core Qt5::Gui Qt5::Widgets)

ADD_EXECUTABLE(TestHdrCreationWizard TestHdrCreationWizard.cpp)
IF(APPLE OR MSVC)
    TARGET_LINK_LIBRARIES(TestHdrCreationWizard
    ${LUMINANCE_MODULES_GUI} ${LIBS})
ELSEIF(UNIX)
    TARGET_LINK_LIBRARIES(TestHdrCreationWizard
    -Xlinker --start-group ${LUMINANCE_MODULES_GUI} -Xlinker --end-group
    ${LIBS})
ENDIF()
TARGET_LINK_LIBRARIES(TestHdrCreationWizard Qt5::Core Qt5::Gui Qt5::Widgets)

ADD_EXECUTABLE(TestFusionOperator TestFusionOperator.cpp)
IF(APPLE OR MSVC)
TARGET_LINK_LIBRARIES(TestFusionOperator
    ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LUMINANCE_MODULES_CLI}
    ${LUMINANCE_MODULES_GUI} ${LIBS})
ELSE(UNIX)
TARGET_LINK_LIBRARIES(TestFusionOperator
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    -Xlinker --start-group ${LUMINANCE_MODULES_CLI} ${LUMINANCE_MODULES_GUI} -Xlinker --end-group ${LIBS})
ENDIF()
TARGET_LINK_LIBRARIES(TestFusionOperator Qt5::Core Qt5::Gui Qt5::Widgets)

ADD_EXECUTABLE(TestPoissonSolver TestPoissonSolver.cpp)
TARGET_LINK_LIBRARIES(TestPoissonSolver hdrwizard pfs pfstmo 
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LIBS})
ADD_TEST(TestPoissonSolver TestPoissonSolver)

ENDIF(GTEST_FOUND)
