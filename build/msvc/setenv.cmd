@echo off
SET CYGWIN_DIR=C:\cygwin64
SET TEMP_DIR=.downloaddir
rem SET CMAKE_DIR=C:\Data\Programs\cmake-2.8.12.1-win32-x86
SET CMAKE_DIR=C:\Data\Programs\cmake-3.4.1-win32-x86

SET VISUAL_STUDIO_VC_REDIST=C:\Program Files (x86)\%VS_PROG_FILES%\VC\redist\%RawPlatform%
rem SET QTDIR=C:\Data\Develop\Qt\5.0.1-x64\qtbase
SET QTDIR=C:\Data\Dev\Qt5.8.0\5.8\msvc2015_64
SET SVN_DIR=C:\Data\Programs\Apache-Subversion-1.8.13\bin

REM Options:  -------------------------------------------

REM enable another configuration (defaulting to Release, RelWithDebInfo)
REM SET Configuration=Debug
REM SET ConfigurationLuminance=Release

SET LuminanceTarget=
rem --target luminance-hdr

REM updates/overwrites the original .ts files doing a Qt lupdate
SET OPTION_LUMINANCE_UPDATE_TRANSLATIONS=0
SET OPTION_LUPDATE_NOOBSOLETE=0

REM Optional variables
REM SET L_BOOST_DIR=C:\Data\Develop\libhdrStuff

REM should the Luminance git repo be updated (defaulting to true for read-only git download)
SET UPDATE_REPO_LUMINANCE=0