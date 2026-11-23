# cmakefile executed within a makefile target

# we look for the git command in this paths by order of preference
find_program(GIT_CMD git.exe HINTS ENV Path PATH_SUFFIXES ../)

# Fail if Git is not installed
if (GIT_CMD STREQUAL GIT_CMD-NOTFOUND)
    message(FATAL_ERROR "git command not found!")
else ()
    message(STATUS "git command found: ${GIT_CMD}")
endif ()

# Get version description.
# Depending on whether you checked out a branch (dev) or a tag (release),
# "git describe" will return "5.0-gtk2-2-g12345678" or "5.0-gtk2", respectively.
execute_process(COMMAND ${GIT_CMD} describe --tags --always OUTPUT_VARIABLE GIT_DESCRIBE OUTPUT_STRIP_TRAILING_WHITESPACE WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

# Prefer the exact tag for tagged builds. On branch builds, use the version
# computed by CMake so the installer matches the application version even
# when an older reachable tag would otherwise win "git describe".
execute_process(COMMAND ${GIT_CMD} describe --tags --exact-match
    OUTPUT_VARIABLE GIT_EXACT_TAG
    RESULT_VARIABLE GIT_EXACT_TAG_RESULT
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
if(GIT_EXACT_TAG_RESULT EQUAL 0 AND NOT "${GIT_EXACT_TAG}" STREQUAL "")
    set(GIT_DESCRIBE "${GIT_EXACT_TAG}")
elseif(DEFINED LHDR_VERSION AND NOT "${LHDR_VERSION}" STREQUAL "")
    set(GIT_DESCRIBE "${LHDR_VERSION}")
endif()

# Get branch name.
# Will return empty if you checked out a commit or tag. Empty string handled later.
execute_process(COMMAND ${GIT_CMD} symbolic-ref --short -q HEAD OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

# Get commit hash.
execute_process(COMMAND ${GIT_CMD} rev-parse --short --verify HEAD OUTPUT_VARIABLE GIT_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

# Get commit date, YYYY-MM-DD.
execute_process(COMMAND ${GIT_CMD} show -s --format=%cd --date=format:%Y-%m-%d OUTPUT_VARIABLE GIT_COMMIT_DATE OUTPUT_STRIP_TRAILING_WHITESPACE WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

# Get number of commits since tagging. This is what "GIT_DESCRIBE" uses.
# Works when checking out branch, tag or commit.
# Get a list of all tags in repo:
execute_process(COMMAND ${GIT_CMD} tag --merged HEAD OUTPUT_VARIABLE GIT_TAG WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
# Replace newlines with semicolons so that it can be split:
string(REPLACE "\n" ";" GIT_TAG_LIST "${GIT_TAG}")
execute_process(COMMAND ${GIT_CMD} rev-list --count HEAD --not ${GIT_TAG_LIST} OUTPUT_VARIABLE GIT_COMMITS_SINCE_TAG OUTPUT_STRIP_TRAILING_WHITESPACE WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

# Numeric version only. Use it only if you have no choice, because it doesn't tell the branch or commit since tag
#execute_process(COMMAND ${GIT_CMD} describe --tags --always --abbrev=0 OUTPUT_VARIABLE GIT_DESCRIBE_NUMERIC OUTPUT_STRIP_TRAILING_WHITESPACE WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
#string(REGEX REPLACE "-.*" "" GIT_DESCRIBE_NUMERIC ${GIT_DESCRIBE_NUMERIC})
set(GIT_NUMERIC_VERSION_ONLY "${LHDR_VERSION_MAJOR}.${LHDR_VERSION_MINOR}.${LHDR_VERSION_PATCH}")

# Get number of commits since branching.
# Works when checking out branch, tag or commit.
execute_process(COMMAND ${GIT_CMD} rev-list --count HEAD --not --tags OUTPUT_VARIABLE GIT_COMMITS_SINCE_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

# If the user checked out a tag or detached commit, use a generic label.
# Branch names may contain slashes, which are not valid in installer filenames.
if (GIT_BRANCH STREQUAL "")
    set (GIT_BRANCH "release")
endif()
string(REGEX REPLACE "[^A-Za-z0-9._-]" "-" GIT_BRANCH "${GIT_BRANCH}")

# Create numeric version.
# This version is nonsense, either don't use it at all or use it only where you have no other choice, e.g. Inno Setup's VersionInfoVersion.
# Strip everything after hyphen, e.g. "5.0-gtk2" -> "5.0", "5.1-rc1" -> "5.1" (ergo BS).
if (GIT_COMMITS_SINCE_TAG STREQUAL "")
    set (GIT_NUMERIC_VERSION_BS "0.0.0")
else ()
    string(REGEX REPLACE "-.*" "" GIT_NUMERIC_VERSION_BS ${GIT_DESCRIBE})
    set(GIT_NUMERIC_VERSION_BS "${GIT_NUMERIC_VERSION_BS}.${GIT_COMMITS_SINCE_TAG}")
endif ()

message(STATUS "Git checkout information:")
message(STATUS "    Commit description:   ${GIT_DESCRIBE}")
message(STATUS "    Branch:               ${GIT_BRANCH}")
message(STATUS "    Commit:               ${GIT_COMMIT}")
message(STATUS "    Commit date:          ${GIT_COMMIT_DATE}")
message(STATUS "    Commits since tag:    ${GIT_COMMITS_SINCE_TAG}")
message(STATUS "    Commits since branch: ${GIT_COMMITS_SINCE_BRANCH}")
message(STATUS "    Version:              ${GIT_NUMERIC_VERSION_BS}  (unreliable)")
message(STATUS "    Numeric Version:      ${GIT_NUMERIC_VERSION_ONLY}  (unreliable)")

if (BIT_DEPTH EQUAL 4)
    set(BUILD_BIT_DEPTH 32)
    # 32 bits builds has to be installable on 64 bits system, to support WinXP/64.
    set(ARCHITECTURE_ALLOWED "x86 x64 ia64")
    # installing in 32 bits mode even on 64 bits OS and architecture
    set(INSTALL_MODE "")
elseif (BIT_DEPTH EQUAL 8)
    set(BUILD_BIT_DEPTH 64)
    # Restricting the 64 bits builds to 64 bits systems only
    set(ARCHITECTURE_ALLOWED "x64 ia64")
    # installing in 64 bits mode for all 64 bits processors, even for itanium architecture
    set(INSTALL_MODE "x64 ia64")
endif (BIT_DEPTH EQUAL 4)
# set part of the output archive name
set(SYSTEM_NAME "Windows")
set ( QTTRANS "$ENV{MSYSTEM_PREFIX}/share/qt6/translations")
set ( QTWEBENGINE_RESOURCES "$ENV{MSYSTEM_PREFIX}/share/qt6/resources")
set ( QTWEBENGINE_LOCALES "$ENV{MSYSTEM_PREFIX}/share/qt6/translations/qtwebengine_locales")

configure_file ("${PROJECT_SOURCE_DIR}/build_files/platforms/msys2/WindowsInnoSetup.iss.in" "${CMAKE_INSTALL_PREFIX}/WindowsInnoSetup.iss")
