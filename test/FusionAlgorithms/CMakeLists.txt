ADD_EXECUTABLE(PrintTriangularWeights PrintTriangularWeights.cpp)

# Link sub modules
IF(MSVC OR APPLE)
TARGET_LINK_LIBRARIES(PrintTriangularWeights hdrcreation pfs)
ELSE()
TARGET_LINK_LIBRARIES(PrintTriangularWeights -Xlinker --start-group hdrcreation pfs -Xlinker --end-group)
ENDIF()
# Link shared library
TARGET_LINK_LIBRARIES(PrintTriangularWeights ${LIBS})
