@echo off
SET CYGWIN_DIR=c:\cygwin
SET TEMP_DIR=.downloaddir
SET CMAKE_DIR=C:\Data\Programs\cmake-2.8.12.1-win32-x86
SET VISUAL_STUDIO_VC_REDIST=C:\Program Files (x86)\%VS_PROG_FILES%\VC\redist\%RawPlatform%
rem SET QTDIR=C:\Data\Develop\Qt\5.0.1-x64\qtbase
SET QTDIR=C:\Qt\5.2.0\msvc2012_64_opengl

REM Options:  -------------------------------------------

REM enable another configuration (defaulting to Release, RelWithDebInfo)
REM SET Configuration=Debug
REM SET ConfigurationLuminance=Release

REM updates/overwrites the original .ts files doing a Qt lupdate
SET OPTION_LUMINANCE_UPDATE_TRANSLATIONS=0
SET OPTION_LUPDATE_NOOBSOLETE=0

REM Optional variables
REM SET L_BOOST_DIR=C:\Data\Develop\libhdrStuff

REM should the Luminance git repo be updated (defaulting to true for read-only git download)
SET UPDATE_REPO_LUMINANCE=0