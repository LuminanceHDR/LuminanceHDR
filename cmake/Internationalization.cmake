IF(APPLE)
# ADD_DEFINITIONS(-DI18NDIR=QCoreApplication::applicationDirPath\(\)+"/../Resources/i18n")
SET(I18NDIR QCoreApplication::applicationDirPath\(\)+"/../Resources/i18n")
SET(HELPDIR QCoreApplication::applicationDirPath\(\)+"/../Resources/help")
ELSEIF(UNIX)
# ADD_DEFINITIONS(-DPREFIX=${CMAKE_INSTALL_PREFIX})
# ADD_DEFINITIONS(-DI18NDIR="${CMAKE_INSTALL_PREFIX}/share/luminance-hdr/i18n")
SET(BASEDIR "\"${CMAKE_INSTALL_PREFIX}\"")
SET(I18NDIR "\"${CMAKE_INSTALL_PREFIX}/share/luminance-hdr/i18n\"")
SET(HELPDIR "\"${CMAKE_INSTALL_PREFIX}/share/luminance-hdr/help\"")
SET(HDRHTMLDIR "\"${CMAKE_INSTALL_PREFIX}/share/luminance-hdr/hdrhtml\"")
ELSEIF(WIN32)
# ADD_DEFINITIONS(-DI18NDIR=QCoreApplication::applicationDirPath\(\)+"/i18n")
SET(I18NDIR QCoreApplication::applicationDirPath\(\)+"/i18n")
SET(HELPDIR QCoreApplication::applicationDirPath\(\)+"/help")
ENDIF()

OPTION(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files (WARNING: make clean will delete the source .ts files! Danger!)")
OPTION(LUPDATE_NOOBSOLETE "While doing an lupdate, remove obsolete entries.")

# Holds al the file that will be translated
SET(FILES_TO_TRANSLATE )

# add a function here that creates the translation when necessary
