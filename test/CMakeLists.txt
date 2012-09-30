FIND_PACKAGE(GTest)

IF(GTEST_FOUND)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src)

IF(UNIX)
FIND_PACKAGE(Threads)
ENDIF(UNIX)

# Colorspace test
# LinearRGB -> Y
ADD_EXECUTABLE(TestRGB2Y TestRGB2Y.cpp)
TARGET_LINK_LIBRARIES(TestRGB2Y pfs common
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES})
ADD_TEST(TestRGB2Y TestRGB2Y)

# SRGB -> Y
ADD_EXECUTABLE(TestSRGB2Y TestSRGB2Y.cpp)
TARGET_LINK_LIBRARIES(TestSRGB2Y pfs common
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES})
ADD_TEST(TestSRGB2Y TestSRGB2Y)

# Mantiuk06
ADD_EXECUTABLE(TestMantiuk06Basic
    TestMantiuk06Basic.cpp
    mantiuk06/contrast_domain.cpp
    mantiuk06/contrast_domain.h
)
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(TestMantiuk06Basic pfs pfstmo common
        ${GTEST_BOTH_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBS})
ELSE()
    TARGET_LINK_LIBRARIES(TestMantiuk06Basic -Xlinker --start-group pfs pfstmo common -Xlinker --end-group
        ${GTEST_BOTH_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBS} -lrt)
ENDIF()
ADD_TEST(TestMantiuk06Basic TestMantiuk06Basic)

ADD_EXECUTABLE(TestMantiuk06Pyramid
    TestMantiuk06Pyramid.cpp
    mantiuk06/contrast_domain.cpp
    mantiuk06/contrast_domain.h
)
IF(MSVC OR APPLE)
    TARGET_LINK_LIBRARIES(TestMantiuk06Pyramid pfs pfstmo common
        ${GTEST_BOTH_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBS})
ELSE()
    TARGET_LINK_LIBRARIES(TestMantiuk06Pyramid -Xlinker --start-group pfs pfstmo common -Xlinker --end-group
        ${GTEST_BOTH_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBS} -lrt)
ENDIF()
ADD_TEST(TestMantiuk06Pyramid TestMantiuk06Pyramid)

ADD_EXECUTABLE(TestVex TestVex.cpp)
TARGET_LINK_LIBRARIES(TestVex
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${TBB_LIBRARIES})
ADD_TEST(TestVex TestVex)

ENDIF(GTEST_FOUND)
