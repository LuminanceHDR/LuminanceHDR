IF(UNIX)
ADD_DEFINITIONS(-Wall -Wno-unknown-pragmas)
ADD_DEFINITIONS(-Wno-deprecated)
ADD_DEFINITIONS(-Wno-deprecated-declarations)

IF(APPLE)
# optimize for native architecture on Apple
# ADD_DEFINITIONS(-march=native)  # Disabled: not supported on all Apple Silicon
ENDIF()
ENDIF()

# AppleClang does not discover Homebrew's OpenMP runtime automatically. Seed
# FindOpenMP with the installed libomp paths while still allowing callers to
# override every value explicitly.
IF(APPLE AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND
   NOT OpenMP_CXX_FLAGS)
    FIND_PROGRAM(LHDR_HOMEBREW_EXECUTABLE brew)
    IF(LHDR_HOMEBREW_EXECUTABLE)
        EXECUTE_PROCESS(
            COMMAND ${LHDR_HOMEBREW_EXECUTABLE} --prefix libomp
            RESULT_VARIABLE LHDR_LIBOMP_RESULT
            OUTPUT_VARIABLE LHDR_LIBOMP_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        IF(LHDR_LIBOMP_RESULT EQUAL 0 AND
           EXISTS "${LHDR_LIBOMP_PREFIX}/include/omp.h" AND
           EXISTS "${LHDR_LIBOMP_PREFIX}/lib/libomp.dylib")
            SET(LHDR_OPENMP_FLAGS
                "-Xpreprocessor -fopenmp -I${LHDR_LIBOMP_PREFIX}/include")
            SET(OpenMP_C_FLAGS "${LHDR_OPENMP_FLAGS}" CACHE STRING
                "C flags for Homebrew libomp")
            SET(OpenMP_CXX_FLAGS "${LHDR_OPENMP_FLAGS}" CACHE STRING
                "C++ flags for Homebrew libomp")
            SET(OpenMP_C_LIB_NAMES omp CACHE STRING
                "OpenMP C runtime library")
            SET(OpenMP_CXX_LIB_NAMES omp CACHE STRING
                "OpenMP C++ runtime library")
            SET(OpenMP_omp_LIBRARY
                "${LHDR_LIBOMP_PREFIX}/lib/libomp.dylib" CACHE FILEPATH
                "Homebrew OpenMP runtime")
        ENDIF()
    ENDIF()
ENDIF()

#Activate C++11 support, when available
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    add_definitions(-DBRANCH_PREDICTION)
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
        message(STATUS "C++11 activated for GCC ${GCC_VERSION}")
        set(CMAKE_CXX_STANDARD 11)
        add_definitions(-DLHDR_CXX11_ENABLED)
    elseif(GCC_VERSION VERSION_GREATER 4.3 OR GCC_VERSION VERSION_EQUAL 4.3)
        message(WARNING "C++0x activated for GCC ${GCC_VERSION}. If you get any errors update to a compiler that fully supports C++11")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")  # add_definitions("-std=gnu++0x")
        add_definitions(-DLHDR_CXX11_ENABLED)
    else()
        message(WARNING "C++11 NOT available. GCC >= 4.3 is needed.")
    endif()
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    message(STATUS "C++11 activated for Clang")
    set(CMAKE_CXX_STANDARD 11)
    if (APPLE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++abi -stdlib=libc++ -rpath @loader_path/../Frameworks/ -Wl,-no_warn_duplicate_libraries")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
    add_definitions(-DLHDR_CXX11_ENABLED)
    add_definitions(-DBRANCH_PREDICTION)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnull-dereference -Wdelete-non-virtual-dtor -Wsign-compare -Wswitch")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wreturn-type -Wself-assign -Wunused-argument -Wunused-function -Wtautological-compare")
    #OpenMP support under Linux with clang
    if (UNIX)
        FIND_PACKAGE(OpenMP REQUIRED)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    endif()
elseif(MSVC)
    message(STATUS "C++11 is active by default on Microsoft Visual Studio")
    add_definitions(-DLHDR_CXX11_ENABLED)
else()
    message(WARNING "Your C++ compiler does not support C++11.")
endif()
#OpenMP Support under Linux, Windows with MSVC & MacOS X with GCC >= 4.3
IF(MSVC)
    FIND_PACKAGE(OpenMP REQUIRED)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
ELSEIF(CMAKE_COMPILER_IS_GNUCC AND UNIX)
    EXECUTE_PROCESS(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    IF(GCC_VERSION VERSION_GREATER 4.3 OR GCC_VERSION VERSION_EQUAL 4.3)
        MESSAGE(STATUS "GCC >= 4.3")
        FIND_PACKAGE(OpenMP REQUIRED)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    ENDIF()
ENDIF()
