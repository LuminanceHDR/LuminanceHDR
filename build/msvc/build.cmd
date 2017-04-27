@echo off
SETLOCAL

REM SANITY CHECKS

where /Q cmake
IF %ERRORLEVEL% NEQ 0 (
	echo Error: "cmake" command not in the PATH.
	echo You must have CMake installed and added to your PATH, aborting!
	goto error_end
)

where /Q svn
IF %ERRORLEVEL% NEQ 0 (
	echo Error: "svn" command not in the PATH.
	echo You must have SVN installed and added to your PATH, aborting!
	goto error_end
)

where /Q git
IF %ERRORLEVEL% NEQ 0 (
	echo Error: "git" command not in the PATH.
	echo You must have GIT installed and added to your PATH, aborting!
	goto error_end
)

REM End SANITY CHECKS

REM Start Lib Versions

REM  http://dev.exiv2.org/projects/exiv2/repository/
SET EXIV2_COMMIT=4753

REM  http://github.com/libjpeg-turbo/libjpeg-turbo
SET LIBJPEG_COMMIT_LONG=da2a27ef056a0179cbd80f9146e58b89403d9933

REM  https://github.com/madler/zlib/commits
SET ZLIB_COMMIT_LONG=cacf7f1d4e3d44d871b605da3b647f07d718623f

REM  https://github.com/openexr/openexr
SET OPENEXR_COMMIT_LONG=20d043d017d4b752356bb76946ffdffaa9c15c72
SET OPENEXR_CMAKE_VERSION=2.2

REM  http://www.boost.org/
SET BOOST_MINOR=63

REM ftp://ftp.fftw.org/pub/fftw/
SET FFTW_VER=3.3.5

REM https://github.com/mm2/Little-CMS
SET LCMS_COMMIT_LONG=f9d75ccef0b54c9f4167d95088d4727985133c52

REM https://github.com/ampl/gsl
SET GSL_COMMIT_LONG=48e0194da0d8921aff57c293b4f5083877d3f55b

REM https://github.com/LibRaw/LibRaw
SET LIBRAW_COMMIT_LONG=d7c3d2cb460be10a3ea7b32e9443a83c243b2251
SET LIBRAW_DEMOS2_COMMIT_LONG=194f592e205990ea8fce72b6c571c14350aca716
SET LIBRAW_DEMOS3_COMMIT_LONG=f0895891fdaa775255af02275fce426a5bf5c9fc

REM ftp://sourceware.org/pub/pthreads-win32/
SET PTHREADS_DIR=prebuilt-dll-2-9-1-release

REM http://heasarc.gsfc.nasa.gov/FTP/software/fitsio/c
SET CFITSIO_VER=3360
REM broken 3370

REM Internal version number for  http://qtpfsgui.sourceforge.net/win/hugin-*
SET HUGIN_VER=201600

REM http://download.osgeo.org/libtiff/
SET TIFF_VER=4.0.7

REM End Lib Versions

IF EXIST .settings\vsexpress.txt (
    SET VSCOMMAND=vcexpress
) ELSE IF EXIST .settings\devent.txt (
   SET VSCOMMAND=devenv
) ELSE (
    vcexpress XXXXXXXXXXXXX 2>NUL >NUL
    IF ERRORLEVEL 1 (
        devenv /? 2>NUL >NUL
        IF ERRORLEVEL 1 (
            wdexpress /? 2>NUL >NUL
            IF ERRORLEVEL 1 (
                echo.
                echo.ERROR: This file must be run inside a VS command prompt!
                echo.
                goto error_end
            ) ELSE (
                SET VSCOMMAND=msbuild
            )
        ) ELSE (
            SET VSCOMMAND=devenv
        )
    ) ELSE (
        SET VSCOMMAND=vcexpress
    )
    mkdir .settings 2>NUL >NUL
    echo x>.settings\%VSCOMMAND%.txt
)

IF EXIST ..\msvc (
	echo.
	echo.ERROR: This file should NOT be executed within the LuminanceHDR source directory,
	echo.       but in a new empty folder!
	echo.
	goto error_end
)

ml64.exe > NUL
IF ERRORLEVEL 1 (
	set Platform=Win32
	set RawPlatform=x86
	set CpuPlatform=ia32
) ELSE (
	set Platform=x64
	set RawPlatform=x64
	set CpuPlatform=intel64
)

SET VISUAL_STUDIO_VC_REDIST=%VCINSTALLDIR%\redist\%RawPlatform%

IF DEFINED VS140COMNTOOLS (
	REM Visual Studio 2015
	set VS_SHORT=vc14
	set VS_CMAKE=Visual Studio 14
	set VS_PROG_FILES=Microsoft Visual Studio 14.0
	set VS_LCMS=VC2015

) ELSE IF DEFINED VS120COMNTOOLS (
	REM Visual Studio 2013
	set VS_SHORT=vc12
	set VS_CMAKE=Visual Studio 12
	set VS_PROG_FILES=Microsoft Visual Studio 12.0
	set VS_LCMS=VC2013
	
) ELSE IF DEFINED VS110COMNTOOLS (
	REM Visual Studio 2012
	set VS_SHORT=vc11
	set VS_CMAKE=Visual Studio 11
	set VS_PROG_FILES=Microsoft Visual Studio 11.0
	set VS_LCMS=VC2012
	
) ELSE IF DEFINED VS100COMNTOOLS (
	REM Visual Studio 2010
	set VS_SHORT=vc10
	set VS_CMAKE=Visual Studio 10
	set VS_PROG_FILES=Microsoft Visual Studio 10.0
	set VS_LCMS=VC2010
	
) ELSE (
	REM Visual Studio 2008
	set VS_SHORT=vc9
	set VS_CMAKE=Visual Studio 9 2008
	set VS_PROG_FILES=Microsoft Visual Studio 9.0
	set VS_LCMS=VC2008
)
IF %Platform% EQU x64 (
	set VS_CMAKE=%VS_CMAKE% Win64
)

call setenv.cmd

IF NOT EXIST %CYGWIN_DIR%\bin\nasm.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\sed.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\ssh.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\wget.exe GOTO cygwin_error
GOTO cygwin_ok

:cygwin_error
echo ERROR: Cygwin with
echo    nasm
echo    sed 
echo    ssh
echo    wget
echo is required
GOTO error_end

:cygwin_ok

SET INSTALL_DIR=dist
IF NOT EXIST %INSTALL_DIR% (
	mkdir %INSTALL_DIR%
)

IF NOT DEFINED Configuration (
	set Configuration=Release
)
IF NOT DEFINED ConfigurationLuminance (
	set ConfigurationLuminance=RelWithDebInfo
)

cls
echo.
echo.--- %VS_CMAKE% ---
echo.Configuration = %Configuration%
echo.ConfigurationLuminance = %ConfigurationLuminance%
echo.Platform = %Platform% (%RawPlatform%)
echo.

IF NOT EXIST %TEMP_DIR% (
	mkdir %TEMP_DIR%
)


IF NOT EXIST vcDlls (
	mkdir vcDlls
	robocopy "%vcinstalldir%redist\%RawPlatform%" vcDlls /MIR >nul
)

IF NOT EXIST vcDlls\selected (
	mkdir vcDlls\selected

	copy vcDlls\**\vcomp* vcDlls\selected
	copy vcDlls\**\msv* vcDlls\selected
)

IF NOT EXIST %TEMP_DIR%\hugin-%HUGIN_VER%-%RawPlatform%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/hugin-%HUGIN_VER%-%RawPlatform%.zip ^
			qtpfsgui.sourceforge.net/win/hugin-%HUGIN_VER%-%RawPlatform%.zip
)
IF NOT EXIST hugin-%HUGIN_VER%-%RawPlatform% (
    cmake -E tar hugin-%HUGIN_VER%-%RawPlatform% %TEMP_DIR%\hugin-%HUGIN_VER%-%RawPlatform%.zip
)

SET ZLIB_COMMIT=%ZLIB_COMMIT_LONG:~0,7%
IF NOT EXIST %TEMP_DIR%\zlib-%ZLIB_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/zlib-%ZLIB_COMMIT%.zip ^
			https://github.com/madler/zlib/archive/%ZLIB_COMMIT_LONG%.zip
)

IF NOT EXIST zlib-%ZLIB_COMMIT% (
	cmake -E tar %TEMP_DIR%/zlib-%ZLIB_COMMIT%.zip
	move zlib-* zlib-%ZLIB_COMMIT%
)

IF NOT EXIST zlib-%ZLIB_COMMIT%.build (
    mkdir zlib-%ZLIB_COMMIT%.build
    
    pushd zlib-%ZLIB_COMMIT%.build
	cmake -G "%VS_CMAKE%" -DCMAKE_INSTALL_PREFIX=..\%INSTALL_DIR% ..\zlib-%ZLIB_COMMIT%
	IF errorlevel 1 goto error_end
	cmake --build . --config %Configuration%
	IF errorlevel 1 goto error_end
	cmake --build . --config %Configuration% --target install
	IF errorlevel 1 goto error_end
    
    popd
)


IF NOT EXIST %TEMP_DIR%\lpng170b75.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/lpng170b75.zip ^
			http://sourceforge.net/projects/libpng/files/libpng17/1.7.0beta75/lp170b75.zip/download
				IF errorlevel 1	goto error_end
)
IF NOT EXIST lp170b75 (
	cmake -E tar %TEMP_DIR%/lpng170b75.zip
	pushd lp170b75
	cmake -G "%VS_CMAKE%" . -DCMAKE_INSTALL_PREFIX=..\%INSTALL_DIR%
	IF errorlevel 1	goto error_end
	cmake --build . --config %Configuration% --target install
	IF errorlevel 1	goto error_end
	popd
)

IF NOT EXIST %TEMP_DIR%\expat-2.1.0.tar (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/expat-2.1.0.tar.gz ^
			http://sourceforge.net/projects/expat/files/expat/2.1.0/expat-2.1.0.tar.gz/download
	cmake -E tar %TEMP_DIR%/expat-2.1.0.tar.gz
)
IF NOT EXIST expat-2.1.0 (
	cmake -E tar %TEMP_DIR%/expat-2.1.0.tar
)

IF NOT EXIST expat-2.1.0.build (
    mkdir expat-2.1.0.build
    
    pushd expat-2.1.0.build
	cmake -G "%VS_CMAKE%" -DCMAKE_INSTALL_PREFIX=..\%INSTALL_DIR% ..\expat-2.1.0
	IF errorlevel 1 goto error_end
	cmake --build . --config %Configuration%
	IF errorlevel 1 goto error_end
	cmake --build . --config %Configuration% --target install
	IF errorlevel 1 goto error_end
	popd
)

IF NOT EXIST exiv2-%EXIV2_COMMIT% (
    svn co -r %EXIV2_COMMIT% svn://dev.exiv2.org/svn/trunk exiv2-%EXIV2_COMMIT%
)

IF NOT EXIST exiv2-%EXIV2_COMMIT%.build (
	mkdir exiv2-%EXIV2_COMMIT%.build

    pushd exiv2-%EXIV2_COMMIT%
    SET EXIV2_CMAKE=
	cmake -G "%VS_CMAKE%" -DCMAKE_INSTALL_PREFIX=..\%INSTALL_DIR% ^
			-DEXIV2_ENABLE_BUILD_SAMPLES=OFF -DEXIV2_ENABLE_CURL=OFF -DEXIV2_ENABLE_SSH=OFF
    
	IF errorlevel 1 goto error_end
	cmake --build . --config %Configuration% --target install
	IF errorlevel 1 goto error_end
	popd	
)


SET LIBJPEG_COMMIT=%LIBJPEG_COMMIT_LONG:~0,7%
IF NOT EXIST %TEMP_DIR%\libjpeg-%LIBJPEG_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/libjpeg-%LIBJPEG_COMMIT%.zip ^
			https://github.com/libjpeg-turbo/libjpeg-turbo/archive/%LIBJPEG_COMMIT_LONG%.zip
)

IF NOT EXIST libjpeg-turbo-%LIBJPEG_COMMIT% (
	cmake -E tar %TEMP_DIR%/libjpeg-%LIBJPEG_COMMIT%.zip
	move libjpeg-* libjpeg-turbo-%LIBJPEG_COMMIT%
)


IF NOT EXIST libjpeg-turbo-%LIBJPEG_COMMIT%.build (
	mkdir libjpeg-turbo-%LIBJPEG_COMMIT%.build
	
	
	pushd libjpeg-turbo-%LIBJPEG_COMMIT%.build
	cmake -G "%VS_CMAKE%" -DCMAKE_INSTALL_PREFIX=..\%INSTALL_DIR% ^
			-DCMAKE_BUILD_TYPE=%Configuration% ^
			-DNASM="%CYGWIN_DIR%\bin\nasm.exe" ^
			-DWITH_JPEG8=TRUE ..\libjpeg-turbo-%LIBJPEG_COMMIT%
	IF errorlevel 1 goto error_end
	cmake --build . --config %Configuration% --target install
	IF errorlevel 1 goto error_end
    copy jconfig.h ..\libjpeg-turbo-%LIBJPEG_COMMIT%
	popd
)

SET LCMS_COMMIT=%LCMS_COMMIT_LONG:~0,7%
IF NOT EXIST %TEMP_DIR%\lcms2-%LCMS_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/lcms2-%LCMS_COMMIT%.zip ^
			https://github.com/mm2/Little-CMS/archive/%LCMS_COMMIT_LONG%.zip
)


IF NOT EXIST lcms2-%LCMS_COMMIT% (
	cmake -E tar %TEMP_DIR%/lcms2-%LCMS_COMMIT%.zip
	move Little-CMS-* lcms2-%LCMS_COMMIT%
	
	pushd lcms2-%LCMS_COMMIT%
	REM %VSCOMMAND% Projects\%VS_LCMS%\lcms2.sln /Upgrade
	REM devenv Projects\VC2013\lcms2.sln /build Release /project lcms2_DLL
	%VSCOMMAND% Projects\%VS_LCMS%\lcms2.sln /Rebuild "%Configuration%|%Platform%" /project lcms2_DLL
	IF errorlevel 1	goto error_end
	popd
)

IF NOT EXIST %TEMP_DIR%\tiff-%TIFF_VER%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/tiff-%TIFF_VER%.zip http://download.osgeo.org/libtiff/tiff-%TIFF_VER%.zip
)


IF NOT EXIST tiff-%TIFF_VER% (
	cmake -E tar -q %TEMP_DIR%/tiff-%TIFF_VER%.zip

	pushd tiff-%TIFF_VER%
	cmake -G "%VS_CMAKE%" -DCMAKE_INSTALL_PREFIX=..\%INSTALL_DIR% -DCMAKE_BUILD_TYPE=%Configuration% .
	IF errorlevel 1 goto error_end
	cmake --build . --config %Configuration% --target install
	IF errorlevel 1 goto error_end
	popd
)

SET LIBRAW_COMMIT=%LIBRAW_COMMIT_LONG:~0,7%
SET LIBRAW_DEMOS2_COMMIT=%LIBRAW_DEMOS2_COMMIT_LONG:~0,7%
SET LIBRAW_DEMOS3_COMMIT=%LIBRAW_DEMOS3_COMMIT_LONG:~0,7%

IF NOT EXIST %TEMP_DIR%\LibRaw-%LIBRAW_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/LibRaw-%LIBRAW_COMMIT%.zip ^
			https://github.com/LibRaw/LibRaw/archive/%LIBRAW_COMMIT_LONG%.zip
)

IF NOT EXIST %TEMP_DIR%\LibRaw-demosaic-pack-GPL2-%LIBRAW_DEMOS2_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/LibRaw-demosaic-pack-GPL2-%LIBRAW_DEMOS2_COMMIT%.zip ^
			https://github.com/LibRaw/LibRaw-demosaic-pack-GPL2/archive/%LIBRAW_DEMOS2_COMMIT_LONG%.zip
)

IF NOT EXIST LibRaw-demosaic-pack-GPL2-%LIBRAW_DEMOS2_COMMIT% (
	cmake -E tar %TEMP_DIR%/LibRaw-demosaic-pack-GPL2-%LIBRAW_DEMOS2_COMMIT%.zip
	move LibRaw-demosaic-pack-GPL2-%LIBRAW_DEMOS2_COMMIT%* LibRaw-demosaic-pack-GPL2-%LIBRAW_DEMOS2_COMMIT%
)

IF NOT EXIST %TEMP_DIR%\LibRaw-demosaic-pack-GPL3-%LIBRAW_DEMOS3_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/LibRaw-demosaic-pack-GPL3-%LIBRAW_DEMOS3_COMMIT%.zip ^
			https://github.com/LibRaw/LibRaw-demosaic-pack-GPL3/archive/%LIBRAW_DEMOS3_COMMIT_LONG%.zip
)

IF NOT EXIST LibRaw-demosaic-pack-GPL3-%LIBRAW_DEMOS3_COMMIT% (
	cmake -E tar %TEMP_DIR%/LibRaw-demosaic-pack-GPL3-%LIBRAW_DEMOS3_COMMIT%.zip
	move LibRaw-demosaic-pack-GPL3-%LIBRAW_DEMOS3_COMMIT%* LibRaw-demosaic-pack-GPL3-%LIBRAW_DEMOS3_COMMIT%
)

IF NOT EXIST LibRaw-%LIBRAW_COMMIT% (
	cmake -E tar %TEMP_DIR%/LibRaw-%LIBRAW_COMMIT%.zip
	move LibRaw-%LIBRAW_COMMIT%* LibRaw-%LIBRAW_COMMIT%

	
	pushd LibRaw-%LIBRAW_COMMIT%
	
    REM /openmp
	echo.COPT_OPT="/arch:SSE2"> qtpfsgui_commands.in
	echo.CFLAGS_DP2=/I..\LibRaw-demosaic-pack-GPL2-%LIBRAW_DEMOS2_COMMIT%>> qtpfsgui_commands.in
	echo.CFLAGSG2=/DLIBRAW_DEMOSAIC_PACK_GPL2>> qtpfsgui_commands.in
	echo.CFLAGS_DP3=/I..\LibRaw-demosaic-pack-GPL3-%LIBRAW_DEMOS3_COMMIT%>> qtpfsgui_commands.in
	echo.CFLAGSG3=/DLIBRAW_DEMOSAIC_PACK_GPL3>> qtpfsgui_commands.in
	echo.LCMS_DEF="/DUSE_LCMS2 /DCMS_DLL /I..\lcms2-%LCMS_COMMIT%\include">> qtpfsgui_commands.in
	echo.LCMS_LIB="..\lcms2-%LCMS_COMMIT%\bin\lcms2.lib">> qtpfsgui_commands.in
    echo.JPEG_DEF="/DUSE_JPEG8 /DUSE_JPEG /I..\libjpeg-turbo-%LIBJPEG_COMMIT%">> qtpfsgui_commands.in
    echo.JPEG_LIB="..\libjpeg-turbo-%LIBJPEG_COMMIT%.build\sharedlib\%Configuration%\jpeg.lib">> qtpfsgui_commands.in
	
	nmake /f Makefile.msvc @qtpfsgui_commands.in clean > nul
	nmake /f Makefile.msvc @qtpfsgui_commands.in bin\libraw.dll
	popd
)

SET PTHREADS_CURRENT_DIR=pthreads_%PTHREADS_DIR%_%RawPlatform%
IF NOT EXIST %TEMP_DIR%\%PTHREADS_CURRENT_DIR% (
    mkdir %TEMP_DIR%\%PTHREADS_CURRENT_DIR%
    pushd  %TEMP_DIR%\%PTHREADS_CURRENT_DIR%
    %CYGWIN_DIR%\bin\wget.exe -O pthread.h --retry-connrefused --tries=5 ftp://sourceware.org/pub/pthreads-win32/%PTHREADS_DIR%/include/pthread.h
    %CYGWIN_DIR%\bin\wget.exe -O sched.h --retry-connrefused --tries=5 ftp://sourceware.org/pub/pthreads-win32/%PTHREADS_DIR%/include/sched.h
    %CYGWIN_DIR%\bin\wget.exe -O semaphore.h --retry-connrefused --tries=5 ftp://sourceware.org/pub/pthreads-win32/%PTHREADS_DIR%/include/semaphore.h
    %CYGWIN_DIR%\bin\wget.exe -O pthreadVC2.dll --retry-connrefused --tries=5 ftp://sourceware.org/pub/pthreads-win32/%PTHREADS_DIR%/dll/%RawPlatform%/pthreadVC2.dll
    %CYGWIN_DIR%\bin\wget.exe -O pthreadVC2.lib --retry-connrefused --tries=5 ftp://sourceware.org/pub/pthreads-win32/%PTHREADS_DIR%/lib/%RawPlatform%/pthreadVC2.lib
    popd
)
IF NOT EXIST %PTHREADS_CURRENT_DIR% (
    mkdir %PTHREADS_CURRENT_DIR%
    robocopy %TEMP_DIR%\%PTHREADS_CURRENT_DIR% %PTHREADS_CURRENT_DIR% >nul
)

IF NOT EXIST %TEMP_DIR%\cfit%CFITSIO_VER%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/cfit%CFITSIO_VER%.zip ^
			ftp://heasarc.gsfc.nasa.gov/software/fitsio/c/cfit%CFITSIO_VER%.zip
)

IF NOT EXIST cfit%CFITSIO_VER% (
    cmake -E tar cfit%CFITSIO_VER% %TEMP_DIR%/cfit%CFITSIO_VER%.zip
)

IF NOT EXIST cfit%CFITSIO_VER%.build (
    mkdir cfit%CFITSIO_VER%.build
    pushd cfit%CFITSIO_VER%.build
    
    cmake -G "%VS_CMAKE%" ..\cfit%CFITSIO_VER% -DUSE_PTHREADS=0 ^
				-DCMAKE_INCLUDE_PATH=..\%PTHREADS_CURRENT_DIR% ^
				-DCMAKE_LIBRARY_PATH=..\%PTHREADS_CURRENT_DIR%
	IF errorlevel 1 goto error_end
    cmake --build . --config %Configuration% --target cfitsio
    IF errorlevel 1 goto error_end   
    popd
)


pushd cfit%CFITSIO_VER%
SET CFITSIO=%CD%
popd
pushd cfit%CFITSIO_VER%.build\%Configuration%
SET CFITSIO=%CFITSIO%;%CD%
popd

REM IF NOT EXIST %TEMP_DIR%\CCfits-2.4.tar (
REM 	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/CCfits-2.4.tar.gz ^
REM				http://heasarc.gsfc.nasa.gov/docs/software/fitsio/CCfits/CCfits-2.4.tar.gz
REM 	%CYGWIN_DIR%\bin\cmake -E tar.exe -d %TEMP_DIR%/CCfits-2.4.tar.gz
REM     %CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/CCfits2.4patch.zip ^
REM					http://qtpfsgui.sourceforge.net/win/CCfits2.4patch.zip
REM )
REM IF NOT EXIST CCfits2.4 (
REM 	cmake -E tar %TEMP_DIR%/CCfits-2.4.tar
REM     ren CCfits CCfits2.4
REM     cmake -E tar CCfits2.4 %TEMP_DIR%/CCfits2.4patch.zip
REM )
REM IF NOT EXIST CCfits2.4.build (
REM     mkdir CCfits2.4.build
REM     
REM     pushd CCfits2.4.build
REM 	cmake -G "%VS_CMAKE%" ..\CCfits2.4 -DCMAKE_INCLUDE_PATH=..\cfit%CFITSIO_VER% ^
REM				-DCMAKE_LIBRARY_PATH=..\cfit%CFITSIO_VER%.build\%Configuration%
REM 	IF errorlevel 1 goto error_end
REM     cmake --build . --config %Configuration% --target CCfits
REM 	IF errorlevel 1 goto error_end
REM 	popd
REM )
REM pushd CCfits2.4.build\%Configuration%
REM SET CCFITS_ROOT_DIR=%CD%
REM popd

SET GSL_COMMIT=%GSL_COMMIT_LONG:~0,7%
IF NOT EXIST %TEMP_DIR%\gsl-ampl-%GSL_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/gsl-ampl-%GSL_COMMIT%.zip ^
			https://github.com/ampl/gsl/archive/%GSL_COMMIT_LONG%.zip
)

IF NOT EXIST gsl-1.16 (
	cmake -E tar %TEMP_DIR%/gsl-ampl-%GSL_COMMIT%.zip
	move gsl-* gsl-1.16
)
IF NOT EXIST gsl-1.16.build (
	mkdir gsl-1.16.build
	pushd gsl-1.16.build
	
    cmake -G "%VS_CMAKE%" -DCMAKE_BUILD_TYPE=%Configuration% -DCMAKE_INSTALL_PREFIX=..\dist ..\gsl-1.16
    IF errorlevel 1 goto error_end
	cmake --build . --config %Configuration%
    IF errorlevel 1 goto error_end
	cmake --build . --config %Configuration% --target install
    IF errorlevel 1 goto error_end
	popd
)


SET OPENEXR_COMMIT=%OPENEXR_COMMIT_LONG:~0,7%
IF NOT EXIST %TEMP_DIR%\OpenEXR-dk-%OPENEXR_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/OpenEXR-dk-%OPENEXR_COMMIT%.zip ^
			https://github.com/openexr/openexr/archive/%OPENEXR_COMMIT_LONG%.zip
)

IF NOT EXIST OpenEXR-dk-%OPENEXR_COMMIT% (
	cmake -E tar %TEMP_DIR%/OpenEXR-dk-%OPENEXR_COMMIT%.zip
	move openexr-* OpenEXR-dk-%OPENEXR_COMMIT%
)
IF NOT EXIST OpenEXR-dk-%OPENEXR_COMMIT%\IlmBase.build (
    mkdir OpenEXR-dk-%OPENEXR_COMMIT%\IlmBase.build
    pushd OpenEXR-dk-%OPENEXR_COMMIT%\IlmBase.build

    cmake -G "%VS_CMAKE%" -DCMAKE_BUILD_TYPE=%Configuration% ^
		-DCMAKE_INSTALL_PREFIX=..\..\dist -DBUILD_SHARED_LIBS=OFF ../IlmBase
    IF errorlevel 1 goto error_end
	cmake --build . --config %Configuration% --target install
    IF errorlevel 1 goto error_end
    popd
)
IF NOT EXIST OpenEXR-dk-%OPENEXR_COMMIT%\OpenEXR.build (
    mkdir OpenEXR-dk-%OPENEXR_COMMIT%\OpenEXR.build
    pushd OpenEXR-dk-%OPENEXR_COMMIT%\OpenEXR.build
    cmake -G "%VS_CMAKE%" -DCMAKE_BUILD_TYPE=%Configuration% ^
        -DILMBASE_PACKAGE_PREFIX=%CD%\dist -DBUILD_SHARED_LIBS=OFF ^
        -DCMAKE_INSTALL_PREFIX=..\..\dist ^
        ../OpenEXR
    IF errorlevel 1 goto error_end
	cmake --build . --config %Configuration% --target install
    IF errorlevel 1 goto error_end
    popd
)

IF %Platform% EQU Win32 (
	IF NOT EXIST %TEMP_DIR%\fftw-%FFTW_VER%-dll32.zip (
		%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/fftw-%FFTW_VER%-dll32.zip ^
				ftp://ftp.fftw.org/pub/fftw/fftw-%FFTW_VER%-dll32.zip
	)
) ELSE (
	IF NOT EXIST %TEMP_DIR%\fftw-%FFTW_VER%-dll64.zip (
		%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/fftw-%FFTW_VER%-dll64.zip ^
				ftp://ftp.fftw.org/pub/fftw/fftw-%FFTW_VER%-dll64.zip
	)
)

IF NOT EXIST fftw-%FFTW_VER%-dll (
	IF %Platform% EQU Win32 (
		cmake -E tar -d fftw-%FFTW_VER%-dll %TEMP_DIR%/fftw-%FFTW_VER%-dll32.zip
	) ELSE (
		cmake -E tar -d fftw-%FFTW_VER%-dll %TEMP_DIR%/fftw-%FFTW_VER%-dll64.zip
	)

	pushd fftw-%FFTW_VER%-dll
	lib /def:libfftw3-3.def /machine:%RawPlatform%
	lib /def:libfftw3f-3.def /machine:%RawPlatform%
	lib /def:libfftw3l-3.def /machine:%RawPlatform%
	popd
)

REM IF NOT EXIST %TEMP_DIR%\tbb40_20120613oss_win.zip (
REM 	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/tbb40_20120613oss_win.zip ^
REM				"http://threadingbuildingblocks.org/uploads/77/187/4.0 update 5/tbb40_20120613oss_win.zip"
REM )
REM 
REM IF NOT EXIST tbb40_20120613oss (
REM 	cmake -E tar %TEMP_DIR%/tbb40_20120613oss_win.zip
REM 	REM Everthing is already compiled, nothing to do!
REM )

REM IF NOT EXIST %TEMP_DIR%\gtest-1.6.0.zip (
SET GTEST_DIR=gtest-r680
IF NOT EXIST %GTEST_DIR% (
	REM %CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/gtest-1.6.0.zip http://googletest.googlecode.com/files/gtest-1.6.0.zip
    svn co -r 680 http://googletest.googlecode.com/svn/trunk/ %GTEST_DIR%
    
    pushd %GTEST_DIR%
   	cmake -G "%VS_CMAKE%" . -DBUILD_SHARED_LIBS=1
	cmake --build . --config %Configuration%
    REN Release lib
    popd
)
SET GTEST_ROOT=%CD%\%GTEST_DIR%

REM IF NOT EXIST %GTEST_DIR%.build (
REM 	mkdir %GTEST_DIR%.build
REM 	pushd %GTEST_DIR%.build
REM 	cmake -G "%VS_CMAKE%" ..\%GTEST_DIR% -DBUILD_SHARED_LIBS=1
REM 	%VSCOMMAND% gtest.sln /t:Build /projectconfig=%Configuration%;Platform=%Platform%
REM 	popd
REM )
REM IF NOT EXIST gtest-1.6.0 (
REM 	cmake -E tar %TEMP_DIR%/gtest-1.6.0.zip
REM )

IF NOT DEFINED L_BOOST_DIR (
	set L_BOOST_DIR=.
)

IF NOT EXIST %TEMP_DIR%\boost_1_%BOOST_MINOR%_0.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/boost_1_%BOOST_MINOR%_0.zip ^
			http://sourceforge.net/projects/boost/files/boost/1.%BOOST_MINOR%.0/boost_1_%BOOST_MINOR%_0.zip/download
)

IF NOT EXIST %L_BOOST_DIR%\boost_1_%BOOST_MINOR%_0 (
	echo.Extracting boost. Be patient!

	pushd %L_BOOST_DIR%
	cmake -E tar %TEMP_DIR%/boost_1_%BOOST_MINOR%_0.zip
	popd

 
 	pushd %L_BOOST_DIR%\boost_1_%BOOST_MINOR%_0
 	bootstrap.bat
 	popd
 	
 	pushd %L_BOOST_DIR%\boost_1_%BOOST_MINOR%_0
 	IF %Platform% EQU Win32 (
 		IF %Configuration% EQU Release (
 			cmd.exe /C b2.exe toolset=msvc variant=release
 		) ELSE (
 			cmd.exe /C b2.exe toolset=msvc variant=debug
 		)
 	) ELSE (
 		IF %Configuration% EQU Release (
 			cmd.exe /C b2.exe toolset=msvc variant=release address-model=64
 		) ELSE (
 			cmd.exe /C b2.exe toolset=msvc variant=debug address-model=64
 		)
 	)
	popd
)

REM Set Boost-directory as ENV variable (needed for CMake)
pushd %L_BOOST_DIR%\boost_1_%BOOST_MINOR%_0
REM SET Boost_DIR=%CD%
REM SET BOOST_ROOT=%CD%
popd


IF NOT EXIST LuminanceHdrStuff (
	mkdir LuminanceHdrStuff
)
IF NOT EXIST LuminanceHdrStuff\qtpfsgui (
	pushd LuminanceHdrStuff
	git clone https://github.com/LuminanceHDR/LuminanceHDR.git qtpfsgui
	popd
) ELSE (
	pushd LuminanceHdrStuff\qtpfsgui
	IF %UPDATE_REPO_LUMINANCE% EQU 1 (
		git pull
	)
	popd
)


IF NOT EXIST LuminanceHdrStuff\DEPs (
	pushd LuminanceHdrStuff
	mkdir DEPs
	cd DEPs
	mkdir include
	mkdir lib
	mkdir bin
	popd
	
	for %%v in ("libpng", "lcms2", "libraw", "fftw3", "gsl", "CCfits") do (
		mkdir LuminanceHdrStuff\DEPs\include\%%v
		mkdir LuminanceHdrStuff\DEPs\lib\%%v
		mkdir LuminanceHdrStuff\DEPs\bin\%%v
	)
	
	mkdir LuminanceHdrStuff\DEPs\include\libraw\libraw

	REM mkdir LuminanceHdrStuff\DEPs\include\gsl\gsl
	REM copy gsl-1.15\gsl\*.h LuminanceHdrStuff\DEPs\include\gsl\gsl
	REM copy gsl-1.15\build.vc10\lib\%Platform%\%Configuration%\*.lib LuminanceHdrStuff\DEPs\lib\gsl
	REM copy gsl-1.15\build.vc10\dll\*.dll LuminanceHdrStuff\DEPs\bin\gsl
)

robocopy fftw-%FFTW_VER%-dll LuminanceHdrStuff\DEPs\include\fftw3 *.h /MIR >nul
robocopy fftw-%FFTW_VER%-dll LuminanceHdrStuff\DEPs\lib\fftw3 *.lib /MIR /NJS >nul
robocopy fftw-%FFTW_VER%-dll LuminanceHdrStuff\DEPs\bin\fftw3 *.dll /MIR /NJS >nul


robocopy LibRaw-%LIBRAW_COMMIT%\libraw LuminanceHdrStuff\DEPs\include\libraw\libraw /MIR >nul
robocopy LibRaw-%LIBRAW_COMMIT%\lib LuminanceHdrStuff\DEPs\lib\libraw *.lib /MIR >nul
robocopy LibRaw-%LIBRAW_COMMIT%\bin LuminanceHdrStuff\DEPs\bin\libraw *.dll /MIR >nul
	
robocopy lcms2-%LCMS_COMMIT%\include LuminanceHdrStuff\DEPs\include\lcms2 *.h /MIR >nul
robocopy lcms2-%LCMS_COMMIT%\bin LuminanceHdrStuff\DEPs\lib\lcms2 *.lib /MIR /NJS >nul
REM robocopy lcms2-%LCMS_COMMIT%\bin LuminanceHdrStuff\DEPs\bin\lcms2 *.dll /MIR /NJS >nul

REM robocopy tbb40_20120613oss\include LuminanceHdrStuff\DEPs\include\tbb /MIR >nul
REM robocopy tbb40_20120613oss\lib\%CpuPlatform%\%VS_SHORT% LuminanceHdrStuff\DEPs\lib\tbb /MIR >nul
REM robocopy tbb40_20120613oss\bin\%CpuPlatform%\%VS_SHORT% LuminanceHdrStuff\DEPs\bin\tbb /MIR >nul

robocopy CCfits2.4 LuminanceHdrStuff\DEPs\include\CCfits *.h CCfits /MIR >nul
pushd LuminanceHdrStuff\DEPs\include
SET CCFITS_ROOT_DIR=%CCFITS_ROOT_DIR%;%CD%
popd

IF NOT EXIST LuminanceHdrStuff\qtpfsgui.build (
	mkdir LuminanceHdrStuff\qtpfsgui.build
)
pushd LuminanceHdrStuff\qtpfsgui.build


IF %OPTION_LUMINANCE_UPDATE_TRANSLATIONS% EQU 1 (
	set CMAKE_OPTIONS=-DUPDATE_TRANSLATIONS=1	
) ELSE (
	set CMAKE_OPTIONS=-UUPDATE_TRANSLATIONS
)
IF %OPTION_LUPDATE_NOOBSOLETE% EQU 1 (
	set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DLUPDATE_NOOBSOLETE=1	
) ELSE (
	set CMAKE_OPTIONS=%CMAKE_OPTIONS% -ULUPDATE_NOOBSOLETE
)

set L_CMAKE_INCLUDE=..\..\boost_1_%BOOST_MINOR%_0
set L_CMAKE_LIB=..\..\boost_1_%BOOST_MINOR%_0\stage\lib
set L_CMAKE_PROGRAM_PATH=%CYGWIN_DIR%\bin
set L_CMAKE_PREFIX_PATH=%QTDIR%
set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCMAKE_INCLUDE_PATH=%L_CMAKE_INCLUDE% ^
		-DCMAKE_LIBRARY_PATH=%L_CMAKE_LIB% -DCMAKE_PROGRAM_PATH=%L_CMAKE_PROGRAM_PATH% ^
		-DCMAKE_INSTALL_PREFIX=..\..\dist -DCMAKE_PREFIX_PATH=%L_CMAKE_PREFIX_PATH% ^
		-DPNG_NAMES=libpng16;libpng17 -DOPENEXR_VERSION=%OPENEXR_CMAKE_VERSION%

echo CMake command line options ------------------------------------
echo %CMAKE_OPTIONS%
echo ---------------------------------------------------------------

REM Eclipse CDT4 - NMake Makefiles

cmake -G "%VS_CMAKE%" ..\qtpfsgui %CMAKE_OPTIONS%
REM cmake -G "Eclipse CDT4 - NMake Makefiles" ..\qtpfsgui %CMAKE_OPTIONS%
REM goto end
IF errorlevel 1 goto error_end
popd

IF EXIST LuminanceHdrStuff\qtpfsgui.build\Luminance HDR.sln (
	pushd LuminanceHdrStuff\qtpfsgui.build	
	REM %VSCOMMAND% luminance-hdr.sln /Upgrade
	cmake --build . --config %ConfigurationLuminance%  %LuminanceTarget%

	IF errorlevel 1	goto error_end
	popd
)

IF EXIST LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\luminance-hdr.exe (
	IF EXIST LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% (
		
        robocopy LuminanceHdrStuff\qtpfsgui LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% LICENSE >nul

        IF NOT EXIST LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\align_image_stack.exe (
            copy vcDlls\selected\* LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\
        )
        
        pushd LuminanceHdrStuff\DEPs\bin
        robocopy exiv2 ..\..\qtpfsgui.build\%ConfigurationLuminance% expat.dll >nul
        robocopy libraw ..\..\qtpfsgui.build\%ConfigurationLuminance% libraw.dll >nul
        robocopy fftw3 ..\..\qtpfsgui.build\%ConfigurationLuminance% libfftw3f-3.dll >nul
        popd
		
		pushd %INSTALL_DIR%
		robocopy bin ..\LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% exiv2.dll >nul
		robocopy bin ..\LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% jpeg8.dll >nul
		robocopy bin ..\LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% libpng17.dll >nul
		robocopy bin ..\LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% zlib.dll >nul
		robocopy bin ..\LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% expat.dll >nul
		robocopy bin ..\LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% tiff.dll >nul
		popd

        robocopy cfit%CFITSIO_VER%.build\%Configuration% LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% cfitsio.dll >nul 
        robocopy %PTHREADS_CURRENT_DIR% LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% pthreadVC2.dll >nul 
       
        robocopy lcms2-%LCMS_COMMIT%\bin LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% lcms2.dll >nul 

		IF NOT EXIST LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\i18n\ (
			mkdir LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\i18n
		)
		robocopy LuminanceHdrStuff\qtpfsgui.build LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\i18n lang_*.qm >nul

		for %%v in ("help", "hdrhtml", "icons") do (
			IF NOT EXIST LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\%%v\ (
				mkdir LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\%%v
			)
        )
		robocopy LuminanceHdrStuff\qtpfsgui\help LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\help /MIR >nul
		robocopy LuminanceHdrStuff\qtpfsgui\hdrhtml LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\hdrhtml /MIR >nul
		robocopy LuminanceHdrStuff\qtpfsgui\icons LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\icons /MIR >nul
		

        REM ----- QT Stuff (Dlls, translations) --------------------------------------------
        pushd LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%

        for %%v in ( "Qt5Concurrent.dll", "Qt5Core.dll", "Qt5Gui.dll", "Qt5Multimedia.dll", ^
						"Qt5MultimediaWidgets.dll", "Qt5Network.dll", "Qt5Positioning.dll", "Qt5WinExtras.dll", ^
						"Qt5OpenGL.dll", "Qt5PrintSupport.dll", "Qt5Qml.dll", "Qt5Quick.dll", "Qt5Sensors.dll", ^
						"Qt5Sql.dll", "Qt5V8.dll", "Qt5WebEngine.dll", "Qt5WebEngineCore.dll", "Qt5WebEngineWidgets.dll", ^
						"Qt5Svg.dll", "Qt5WebKitWidgets.dll", "Qt5Widgets.dll", "Qt5Xml.dll", "Qt5WebChannel.dll", ^
						"Qt5QuickWidgets.dll", "icudt53.dll", "icuin53.dll", "icuuc53.dll" ) do (
								robocopy %QTDIR%\bin . %%v >nul
        )
        for %%v in ("imageformats", "sqldrivers", "platforms") do (
            IF NOT EXIST %%v (
                mkdir %%v
            )        
        )
        robocopy %QTDIR%\plugins\imageformats imageformats qjpeg.dll >nul
		robocopy %QTDIR%\plugins\imageformats imageformats qsvg.dll >nul
        robocopy %QTDIR%\plugins\sqldrivers sqldrivers qsqlite.dll >nul
        robocopy %QTDIR%\plugins\platforms platforms qwindows.dll >nul
		robocopy %QTDIR%\translations i18n qt_??.qm >nul
		robocopy %QTDIR%\translations i18n qt_??_*.qm >nul
        popd
        
        robocopy hugin-%HUGIN_VER%-%RawPlatform% LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\hugin /MIR >nul
	)
)

goto end

:error_end
pause

:end

endlocal