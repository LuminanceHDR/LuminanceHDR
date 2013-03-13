IF(APPLE)
SET(I18NDIR QCoreApplication::applicationDirPath\(\)+"/../Resources/i18n")
#ADD_DEFINITIONS(-DI18NDIR=QCoreApplication::applicationDirPath\(\)+"/../Resources/i18n")
ELSEIF(UNIX)
#ADD_DEFINITIONS(-DPREFIX=${CMAKE_INSTALL_PREFIX})
#ADD_DEFINITIONS(-DI18NDIR="${CMAKE_INSTALL_PREFIX}/share/luminance-hdr/i18n")
SET(I18NDIR "\"${CMAKE_INSTALL_PREFIX}/share/luminance-hdr/i18n\"")
SET(HELPDIR "\"${CMAKE_INSTALL_PREFIX}/share/luminance-hdr/help\"")
ELSEIF(WIN32)
#ADD_DEFINITIONS(-DI18NDIR=QCoreApplication::applicationDirPath\(\)+"/i18n")
SET(I18NDIR QCoreApplication::applicationDirPath\(\)+"/i18n")
ENDIF()

OPTION(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files (WARNING: make clean will delete the source .ts files! Danger!)")
OPTION(LUPDATE_NOOBSOLETE "While doing an lupdate, remove obsolete entries.")

# add a function here that creates the translation when necessary
