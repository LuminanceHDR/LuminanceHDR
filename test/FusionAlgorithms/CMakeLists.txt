ADD_EXECUTABLE(PrintWeights PrintWeights.cpp)
ADD_EXECUTABLE(PrintResponses PrintResponses.cpp)

# Link sub modules
IF(MSVC OR APPLE)
TARGET_LINK_LIBRARIES(PrintWeights hdrcreation pfs)
TARGET_LINK_LIBRARIES(PrintResponses hdrcreation pfs)
ELSE()
TARGET_LINK_LIBRARIES(PrintWeights -Xlinker --start-group hdrcreation pfs -Xlinker --end-group)
TARGET_LINK_LIBRARIES(PrintResponses -Xlinker --start-group hdrcreation pfs -Xlinker --end-group)
ENDIF()
# Link shared library
TARGET_LINK_LIBRARIES(PrintWeights ${LIBS})
TARGET_LINK_LIBRARIES(PrintResponses ${LIBS})
