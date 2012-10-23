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
# LinearRGB -> Y
ADD_EXECUTABLE(TestRGB2Y TestRGB2Y.cpp)
TARGET_LINK_LIBRARIES(TestRGB2Y pfs common
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
ADD_TEST(TestRGB2Y TestRGB2Y)

# SRGB -> Y
ADD_EXECUTABLE(TestSRGB2Y TestSRGB2Y.cpp)
TARGET_LINK_LIBRARIES(TestSRGB2Y pfs common
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
ADD_TEST(TestSRGB2Y TestSRGB2Y)

ADD_LIBRARY(constrast_domain STATIC
    mantiuk06/contrast_domain.cpp
    mantiuk06/contrast_domain.h)

# Mantiuk06
ADD_EXECUTABLE(TestMantiuk06Basic
    TestMantiuk06Basic.cpp)
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(TestMantiuk06Basic constrast_domain pfs pfstmo common
        ${GTEST_BOTH_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBS} )
ELSE()
    TARGET_LINK_LIBRARIES(TestMantiuk06Basic
        -Xlinker --start-group constrast_domain pfs pfstmo common
        -Xlinker --end-group
        ${GTEST_BOTH_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBS} -lrt)
ENDIF()
ADD_TEST(TestMantiuk06Basic TestMantiuk06Basic)

ADD_EXECUTABLE(TestMantiuk06Pyramid
    TestMantiuk06Pyramid.cpp
)
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(TestMantiuk06Pyramid pfs pfstmo common
        ${GTEST_BOTH_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBS} constrast_domain)
ELSE()
    TARGET_LINK_LIBRARIES(TestMantiuk06Pyramid
        -Xlinker --start-group pfs pfstmo common -Xlinker --end-group
        ${GTEST_BOTH_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBS} -lrt constrast_domain)
ENDIF()
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

ADD_EXECUTABLE(TestPfsRotate TestPfsRotate.cpp)
TARGET_LINK_LIBRARIES(TestPfsRotate pfs pfsfilter common
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
ADD_TEST(TestPfsRotate TestPfsRotate)

ADD_EXECUTABLE(TestFloatRgb TestFloatRgb.cpp)
TARGET_LINK_LIBRARIES(TestFloatRgb
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LIBS} common pfs)
ADD_TEST(TestFloatRgb TestFloatRgb)

ENDIF(GTEST_FOUND)
