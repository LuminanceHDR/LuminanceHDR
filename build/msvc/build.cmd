@echo off
SETLOCAL

REM  http://dev.exiv2.org/projects/exiv2/repository/
SET EXIV2_COMMIT=3364

REM  http://sourceforge.net/p/libjpeg-turbo/code/
SET LIBJPEG_COMMIT=1093
rem error 1406

rem  https://github.com/madler/zlib/commits
SET ZLIB_COMMIT_LONG=50893291621658f355bc5b4d450a8d06a563053d

rem  https://github.com/openexr/openexr
SET OPENEXR_COMMIT_LONG=91015147e5a6a1914bcb16b12886aede9e1ed065
SET OPENEXR_CMAKE_VERSION=2.2

rem  http://www.boost.org/
SET BOOST_MINOR=55

REM ftp://ftp.fftw.org/pub/fftw/
SET FFTW_VER=3.3.4

rem https://github.com/mm2/Little-CMS
SET LCMS_COMMIT_LONG=d61231a1efb9eb926cbf0235afe03452302c6009

rem https://github.com/LibRaw/LibRaw
SET LIBRAW_COMMIT_LONG=32e21b28e24b6cecdae8813b5ef9c103b8a8ccf0
SET LIBRAW_DEMOS2_COMMIT_LONG=ffea825e121e92aa780ae587b65f80fc5847637c
SET LIBRAW_DEMOS3_COMMIT_LONG=f0895891fdaa775255af02275fce426a5bf5c9fc

rem ftp://sourceware.org/pub/pthreads-win32/
SET PTHREADS_DIR=prebuilt-dll-2-9-1-release

rem http://heasarc.gsfc.nasa.gov/FTP/software/fitsio/c
SET CFITSIO_VER=3360
rem broken 3370

rem Internal version number for  http://qtpfsgui.sourceforge.net/win/hugin-*
SET HUGIN_VER=201300


IF EXIST .settings\vsexpress.txt (
    SET VSCOMMAND=vcexpress
) ELSE IF EXIST .settings\devent.txt (
   SET VSCOMMAND=devenv
) ELSE (
    vcexpress XXXXXXXXXXXXX 2>NUL >NUL
    IF ERRORLEVEL 1 (
        devenv /? 2>NUL >NUL
        IF ERRORLEVEL 1 (
            echo.
            echo.ERROR: This file must be run inside a VS command prompt!
            echo.
            goto error_end
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

IF DEFINED VS110COMNTOOLS (
	REM Visual Studio 2012
	set VS_SHORT=vc11
	set VS_CMAKE=Visual Studio 11
	set VS_PROG_FILES=Microsoft Visual Studio 11.0
	
) ELSE IF DEFINED VS100COMNTOOLS (
	REM Visual Studio 2010
	set VS_SHORT=vc10
	set VS_CMAKE=Visual Studio 10
	set VS_PROG_FILES=Microsoft Visual Studio 10.0
	
) ELSE (
	REM Visual Studio 2008
	set VS_SHORT=vc9
	set VS_CMAKE=Visual Studio 9 2008
	set VS_PROG_FILES=Microsoft Visual Studio 9.0
)
IF %Platform% EQU x64 (
	set VS_CMAKE=%VS_CMAKE% Win64
)

call setenv.cmd

IF NOT EXIST %CMAKE_DIR%\bin\cmake.exe (
	echo.
	echo.ERROR: CMake not found: %CMAKE_DIR%\bin\cmake.exe
	echo.
	goto error_end
)

IF NOT EXIST %CYGWIN_DIR%\bin\cp.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\cvs.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\git.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\gzip.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\mv.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\nasm.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\sed.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\ssh.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\svn.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\tar.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\unzip.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\wget.exe GOTO cygwin_error
GOTO cygwin_ok

:cygwin_error
echo ERROR: Cygwin with 
echo    cp
echo    cvs
echo    git 
echo    gzip 
echo    mv
echo    nasm
echo    sed 
echo    ssh 
echo    svn 
echo    tar 
echo    unzip 
echo    wget
echo is required
GOTO error_end

:cygwin_ok


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

	%CYGWIN_DIR%\bin\cp.exe vcDlls/**/vcomp* vcDlls/selected
	%CYGWIN_DIR%\bin\cp.exe vcDlls/**/msv* vcDlls/selected
)

IF NOT EXIST %TEMP_DIR%\hugin-%HUGIN_VER%-%RawPlatform%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/hugin-%HUGIN_VER%-%RawPlatform%.zip qtpfsgui.sourceforge.net/win/hugin-%HUGIN_VER%-%RawPlatform%.zip
)
IF NOT EXIST hugin-%HUGIN_VER%-%RawPlatform% (
    %CYGWIN_DIR%\bin\unzip.exe -o -q -d hugin-%HUGIN_VER%-%RawPlatform% %TEMP_DIR%\hugin-%HUGIN_VER%-%RawPlatform%.zip
)

SET ZLIB_COMMIT=%ZLIB_COMMIT_LONG:~0,7%
IF NOT EXIST %TEMP_DIR%\zlib-%ZLIB_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/zlib-%ZLIB_COMMIT%.zip --no-check-certificate http://github.com/madler/zlib/zipball/%ZLIB_COMMIT_LONG%
)

IF NOT EXIST zlib-%ZLIB_COMMIT% (
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/zlib-%ZLIB_COMMIT%.zip
	%CYGWIN_DIR%\bin\mv.exe madler-zlib-* zlib-%ZLIB_COMMIT%
	
	REM zlib must be compiled in the source folder, else exiv2 compilation
	REM fails due to zconf.h rename/compile problems, due to cmake
	pushd zlib-%ZLIB_COMMIT%
	%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%"
	IF errorlevel 1 goto error_end
	%VSCOMMAND% zlib.sln /build "%Configuration%|%Platform%" /Project zlib
	IF errorlevel 1 goto error_end
	popd
)

IF NOT EXIST %TEMP_DIR%\lpng170b35.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/lpng170b35.zip http://sourceforge.net/projects/libpng/files/libpng17/1.7.0beta35/lp170b35.zip/download
    IF errorlevel 1	goto error_end
)
IF NOT EXIST lp170b35 (
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/lpng170b35.zip
	pushd lp170b35
	%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" . -DZLIB_ROOT=..\zlib-%ZLIB_COMMIT%;..\zlib-%ZLIB_COMMIT%\%Configuration%
	IF errorlevel 1	goto error_end
	%VSCOMMAND% libpng.sln /build "%Configuration%|%Platform%" /Project png17
	IF errorlevel 1	goto error_end
	popd
)

IF NOT EXIST %TEMP_DIR%\expat-2.1.0.tar (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/expat-2.1.0.tar.gz http://sourceforge.net/projects/expat/files/expat/2.1.0/expat-2.1.0.tar.gz/download
	%CYGWIN_DIR%\bin\gzip.exe -d %TEMP_DIR%/expat-2.1.0.tar.gz
)
IF NOT EXIST expat-2.1.0 (
	%CYGWIN_DIR%\bin\tar.exe -xf %TEMP_DIR%/expat-2.1.0.tar
    
    pushd expat-2.1.0
	%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%"
	IF errorlevel 1 goto error_end
	%VSCOMMAND% expat.sln /build "%Configuration%|%Platform%" /Project expat
	IF errorlevel 1 goto error_end
	popd
)

IF NOT EXIST exiv2-%EXIV2_COMMIT% (
    %CYGWIN_DIR%\bin\svn.exe co -r %EXIV2_COMMIT% svn://dev.exiv2.org/svn/trunk exiv2-%EXIV2_COMMIT%
    pushd exiv2-%EXIV2_COMMIT%
	%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%"  -DZLIB_ROOT=..\zlib-%ZLIB_COMMIT%;..\zlib-%ZLIB_COMMIT%\Release
    
	IF errorlevel 1 goto error_end
	%VSCOMMAND% exiv2.sln /build "%Configuration%|%Platform%" /Project exiv2
	IF errorlevel 1 goto error_end
    copy bin\%Platform%\Dynamic\*.h include
	popd  
)


IF NOT EXIST libjpeg-turbo-%LIBJPEG_COMMIT% (
    %CYGWIN_DIR%\bin\svn.exe co -r %LIBJPEG_COMMIT% svn://svn.code.sf.net/p/libjpeg-turbo/code/trunk libjpeg-turbo-%LIBJPEG_COMMIT%
)
IF NOT EXIST libjpeg-turbo-%LIBJPEG_COMMIT%.build (
    mkdir libjpeg-turbo-%LIBJPEG_COMMIT%.build

	pushd libjpeg-turbo-%LIBJPEG_COMMIT%.build
	%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" -DCMAKE_BUILD_TYPE=%Configuration% -DNASM="%CYGWIN_DIR%\bin\nasm.exe" -DWITH_JPEG8=TRUE ..\libjpeg-turbo-%LIBJPEG_COMMIT%
	IF errorlevel 1 goto error_end
	%VSCOMMAND% libjpeg-turbo.sln /build "%Configuration%|%Platform%"
	IF errorlevel 1 goto error_end
    copy jconfig.h ..\libjpeg-turbo-%LIBJPEG_COMMIT%
	popd
)

SET LCMS_COMMIT=%LCMS_COMMIT_LONG:~0,7%
IF NOT EXIST %TEMP_DIR%\lcms2-%LCMS_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/lcms2-%LCMS_COMMIT%.zip --no-check-certificate https://github.com/mm2/Little-CMS/zipball/%LCMS_COMMIT_LONG%
)


IF NOT EXIST lcms2-%LCMS_COMMIT% (
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/lcms2-%LCMS_COMMIT%.zip
	%CYGWIN_DIR%\bin\mv.exe mm2-Little-CMS-* lcms2-%LCMS_COMMIT%
	
	pushd lcms2-%LCMS_COMMIT%
	%VSCOMMAND% Projects\VC2010\lcms2.sln /Upgrade
	%VSCOMMAND% Projects\VC2010\lcms2.sln /build "%Configuration%|%Platform%"  /Project lcms2_DLL
	IF errorlevel 1	goto error_end
	popd
)

IF NOT EXIST %TEMP_DIR%\tiff-4.0.3.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/tiff-4.0.3.zip http://download.osgeo.org/libtiff/tiff-4.0.3.zip
)

IF NOT EXIST tiff-4.0.3 (
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/tiff-4.0.3.zip

	echo.JPEG_SUPPORT=^1> tiff-4.0.3\qtpfsgui_commands.in
	echo.JPEGDIR=%CD%\libjpeg-turbo-%LIBJPEG_COMMIT%>> tiff-4.0.3\qtpfsgui_commands.in
	echo.JPEG_INCLUDE=-I%CD%\libjpeg-turbo-%LIBJPEG_COMMIT%>> tiff-4.0.3\qtpfsgui_commands.in
	echo.JPEG_LIB=%CD%\libjpeg-turbo-%LIBJPEG_COMMIT%.build\sharedlib\%Configuration%\jpeg.lib>> tiff-4.0.3\qtpfsgui_commands.in
	echo.ZIP_SUPPORT=^1>> tiff-4.0.3\qtpfsgui_commands.in
	echo.ZLIBDIR=..\..\zlib-%ZLIB_COMMIT%\%Configuration%>> tiff-4.0.3\qtpfsgui_commands.in
	echo.ZLIB_INCLUDE=-I..\..\zlib-%ZLIB_COMMIT%>> tiff-4.0.3\qtpfsgui_commands.in
	echo.ZLIB_LIB=$^(ZLIBDIR^)\zlib.lib>> tiff-4.0.3\qtpfsgui_commands.in

	pushd tiff-4.0.3
	nmake /s /c /f Makefile.vc @qtpfsgui_commands.in
	IF errorlevel 1 goto error_end
	popd
)

SET LIBRAW_COMMIT=%LIBRAW_COMMIT_LONG:~0,7%
SET LIBRAW_DEMOS2_COMMIT=%LIBRAW_DEMOS2_COMMIT_LONG:~0,7%
SET LIBRAW_DEMOS3_COMMIT=%LIBRAW_DEMOS3_COMMIT_LONG:~0,7%

IF NOT EXIST %TEMP_DIR%\LibRaw-%LIBRAW_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/LibRaw-%LIBRAW_COMMIT%.zip --no-check-certificate https://github.com/LibRaw/LibRaw/zipball/%LIBRAW_COMMIT_LONG%
)

IF NOT EXIST %TEMP_DIR%\LibRaw-demosaic-pack-GPL2-%LIBRAW_DEMOS2_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/LibRaw-demosaic-pack-GPL2-%LIBRAW_DEMOS2_COMMIT%.zip --no-check-certificate https://github.com/LibRaw/LibRaw-demosaic-pack-GPL2/zipball/%LIBRAW_DEMOS2_COMMIT_LONG%
)

IF NOT EXIST LibRaw-demosaic-pack-GPL2-%LIBRAW_DEMOS2_COMMIT% (
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/LibRaw-demosaic-pack-GPL2-%LIBRAW_DEMOS2_COMMIT%.zip
	%CYGWIN_DIR%\bin\mv.exe LibRaw-LibRaw-* LibRaw-demosaic-pack-GPL2-%LIBRAW_DEMOS2_COMMIT%
)

IF NOT EXIST %TEMP_DIR%\LibRaw-demosaic-pack-GPL3-%LIBRAW_DEMOS3_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/LibRaw-demosaic-pack-GPL3-%LIBRAW_DEMOS3_COMMIT%.zip --no-check-certificate https://github.com/LibRaw/LibRaw-demosaic-pack-GPL3/zipball/%LIBRAW_DEMOS3_COMMIT_LONG%
)

IF NOT EXIST LibRaw-demosaic-pack-GPL3-%LIBRAW_DEMOS3_COMMIT% (
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/LibRaw-demosaic-pack-GPL3-%LIBRAW_DEMOS3_COMMIT%.zip
	%CYGWIN_DIR%\bin\mv.exe LibRaw-LibRaw-* LibRaw-demosaic-pack-GPL3-%LIBRAW_DEMOS3_COMMIT%
)

IF NOT EXIST LibRaw-%LIBRAW_COMMIT% (
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/LibRaw-%LIBRAW_COMMIT%.zip
	%CYGWIN_DIR%\bin\mv.exe LibRaw-LibRaw-* LibRaw-%LIBRAW_COMMIT%

	
	pushd LibRaw-%LIBRAW_COMMIT%
	
    rem /openmp
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
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/cfit%CFITSIO_VER%.zip ftp://heasarc.gsfc.nasa.gov/software/fitsio/c/cfit%CFITSIO_VER%.zip
)

IF NOT EXIST cfit%CFITSIO_VER% (
    %CYGWIN_DIR%\bin\unzip.exe -o -q -d cfit%CFITSIO_VER% %TEMP_DIR%/cfit%CFITSIO_VER%.zip
)

IF NOT EXIST cfit%CFITSIO_VER%.build (
    mkdir cfit%CFITSIO_VER%.build
    pushd cfit%CFITSIO_VER%.build
    
    %CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" ..\cfit%CFITSIO_VER% -DUSE_PTHREADS=1 -DCMAKE_INCLUDE_PATH=..\%PTHREADS_CURRENT_DIR% -DCMAKE_LIBRARY_PATH=..\%PTHREADS_CURRENT_DIR%
	IF errorlevel 1 goto error_end
    %CMAKE_DIR%\bin\cmake.exe --build . --config %Configuration% --target cfitsio
    IF errorlevel 1 goto error_end   
    popd
)


pushd cfit%CFITSIO_VER%
SET CFITSIO=%CD%
popd
pushd cfit%CFITSIO_VER%.build\%Configuration%
SET CFITSIO=%CFITSIO%;%CD%
popd

rem IF NOT EXIST %TEMP_DIR%\CCfits-2.4.tar (
rem 	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/CCfits-2.4.tar.gz http://heasarc.gsfc.nasa.gov/docs/software/fitsio/CCfits/CCfits-2.4.tar.gz
rem 	%CYGWIN_DIR%\bin\gzip.exe -d %TEMP_DIR%/CCfits-2.4.tar.gz
rem     %CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/CCfits2.4patch.zip http://qtpfsgui.sourceforge.net/win/CCfits2.4patch.zip
rem )
rem IF NOT EXIST CCfits2.4 (
rem 	%CYGWIN_DIR%\bin\tar.exe -xf %TEMP_DIR%/CCfits-2.4.tar
rem     ren CCfits CCfits2.4
rem     %CYGWIN_DIR%\bin\unzip.exe -o -q -d CCfits2.4 %TEMP_DIR%/CCfits2.4patch.zip
rem )
rem IF NOT EXIST CCfits2.4.build (
rem     mkdir CCfits2.4.build
rem     
rem     pushd CCfits2.4.build
rem 	%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" ..\CCfits2.4 -DCMAKE_INCLUDE_PATH=..\cfit%CFITSIO_VER% -DCMAKE_LIBRARY_PATH=..\cfit%CFITSIO_VER%.build\%Configuration%
rem 	IF errorlevel 1 goto error_end
rem     %CMAKE_DIR%\bin\cmake.exe --build . --config %Configuration% --target CCfits
rem 	IF errorlevel 1 goto error_end
rem 	popd
rem )
rem pushd CCfits2.4.build\%Configuration%
rem SET CCFITS_ROOT_DIR=%CD%
rem popd

IF NOT EXIST %TEMP_DIR%\gsl-1.15.tar (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/gsl-1.15.tar.gz ftp://ftp.gnu.org/gnu/gsl/gsl-1.15.tar.gz
	%CYGWIN_DIR%\bin\gzip -d %TEMP_DIR%/gsl-1.15.tar.gz
)
IF NOT EXIST %TEMP_DIR%\gsl-1.15-vc10.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/gsl-1.15-vc10.zip http://gladman.plushost.co.uk/oldsite/computing/gsl-1.15-vc10.zip
)
IF NOT EXIST gsl-1.15 (
	%CYGWIN_DIR%\bin\tar.exe -xf %TEMP_DIR%/gsl-1.15.tar
	%CYGWIN_DIR%\bin\unzip.exe -o -q -d gsl-1.15 %TEMP_DIR%/gsl-1.15-vc10.zip

	pushd gsl-1.15\build.vc10
	IF %VS_SHORT% EQU vc9 (
		%CYGWIN_DIR%\bin\sed.exe -i 's/Format Version 11.00/Format Version 10.00/g' gsl.lib.sln
	)
	%VSCOMMAND% gsl.lib.sln /Upgrade
	%VSCOMMAND% gsl.lib.sln /build "%Configuration%|%Platform%" /Project gslhdrs
	gslhdrs\%Platform%\%Configuration%\gslhdrs.exe
	%VSCOMMAND% gsl.lib.sln /build "%Configuration%|%Platform%" /Project gsllib
	popd
)

SET OPENEXR_COMMIT=%OPENEXR_COMMIT_LONG:~0,7%
IF NOT EXIST %TEMP_DIR%\OpenEXR-dk-%OPENEXR_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/OpenEXR-dk-%OPENEXR_COMMIT%.zip --no-check-certificate https://github.com/openexr/openexr/zipball/%OPENEXR_COMMIT_LONG%
)

IF NOT EXIST OpenEXR-dk-%OPENEXR_COMMIT% (
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/OpenEXR-dk-%OPENEXR_COMMIT%.zip
	%CYGWIN_DIR%\bin\mv.exe openexr-openexr-* OpenEXR-dk-%OPENEXR_COMMIT%
)
IF NOT EXIST OpenEXR-dk-%OPENEXR_COMMIT%\IlmBase.build (
    mkdir OpenEXR-dk-%OPENEXR_COMMIT%\IlmBase.build
    pushd OpenEXR-dk-%OPENEXR_COMMIT%\IlmBase.build

    %CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" -DCMAKE_BUILD_TYPE=%Configuration% -DCMAKE_INSTALL_PREFIX=..\output -DZLIB_ROOT=..\..\zlib-%ZLIB_COMMIT%;..\..\zlib-%ZLIB_COMMIT%\%Configuration% -DBUILD_SHARED_LIBS=OFF ../IlmBase 
    IF errorlevel 1 goto error_end
    %VSCOMMAND% IlmBase.sln /build "%Configuration%|%Platform%"
    IF errorlevel 1 goto error_end
    %VSCOMMAND% IlmBase.sln /build "%Configuration%|%Platform%" /Project INSTALL
    IF errorlevel 1 goto error_end
    popd
)
IF NOT EXIST OpenEXR-dk-%OPENEXR_COMMIT%\OpenEXR.build (
    mkdir OpenEXR-dk-%OPENEXR_COMMIT%\OpenEXR.build
    pushd OpenEXR-dk-%OPENEXR_COMMIT%\OpenEXR.build
    %CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" -DCMAKE_BUILD_TYPE=%Configuration% ^
        -DZLIB_ROOT=..\..\zlib-%ZLIB_COMMIT%;..\..\zlib-%ZLIB_COMMIT%\%Configuration% ^
        -DILMBASE_PACKAGE_PREFIX=%CD%\OpenEXR-dk-%OPENEXR_COMMIT%\output -DBUILD_SHARED_LIBS=OFF ^
        -DCMAKE_INSTALL_PREFIX=..\output ^
        ../OpenEXR
    IF errorlevel 1 goto error_end
    %VSCOMMAND% OpenEXR.sln /build "%Configuration%|%Platform%" /Project IlmImf
    IF errorlevel 1 goto error_end
    %VSCOMMAND% OpenEXR.sln /build "%Configuration%|%Platform%" /Project INSTALL
    IF errorlevel 1 goto error_end
    popd
)

IF %Platform% EQU Win32 (
	IF NOT EXIST %TEMP_DIR%\fftw-%FFTW_VER%-dll32.zip (
		%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/fftw-%FFTW_VER%-dll32.zip ftp://ftp.fftw.org/pub/fftw/fftw-%FFTW_VER%-dll32.zip
	)
) ELSE (
	IF NOT EXIST %TEMP_DIR%\fftw-%FFTW_VER%-dll64.zip (
		%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/fftw-%FFTW_VER%-dll64.zip ftp://ftp.fftw.org/pub/fftw/fftw-%FFTW_VER%-dll64.zip
	)
)

IF NOT EXIST fftw-%FFTW_VER%-dll (
	IF %Platform% EQU Win32 (
		%CYGWIN_DIR%\bin\unzip.exe -q -d fftw-%FFTW_VER%-dll %TEMP_DIR%/fftw-%FFTW_VER%-dll32.zip
	) ELSE (
		%CYGWIN_DIR%\bin\unzip.exe -q -d fftw-%FFTW_VER%-dll %TEMP_DIR%/fftw-%FFTW_VER%-dll64.zip
	)

	pushd fftw-%FFTW_VER%-dll
	lib /def:libfftw3-3.def
	lib /def:libfftw3f-3.def
	lib /def:libfftw3l-3.def
	popd
)

REM IF NOT EXIST %TEMP_DIR%\tbb40_20120613oss_win.zip (
REM 	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/tbb40_20120613oss_win.zip "http://threadingbuildingblocks.org/uploads/77/187/4.0 update 5/tbb40_20120613oss_win.zip"
REM )
REM 
REM IF NOT EXIST tbb40_20120613oss (
REM 	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/tbb40_20120613oss_win.zip
REM 	REM Everthing is already compiled, nothing to do!
REM )

REM IF NOT EXIST %TEMP_DIR%\gtest-1.6.0.zip (
SET GTEST_DIR=gtest-r680
IF NOT EXIST %GTEST_DIR% (
	REM %CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/gtest-1.6.0.zip http://googletest.googlecode.com/files/gtest-1.6.0.zip
    %CYGWIN_DIR%\bin\svn.exe co -r 680 http://googletest.googlecode.com/svn/trunk/ %GTEST_DIR%
    
    pushd %GTEST_DIR%
   	%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" . -DBUILD_SHARED_LIBS=1
	%VSCOMMAND% gtest.sln /build "%Configuration%|%Platform%"
    REN Release lib
    popd
)
SET GTEST_ROOT=%CD%\%GTEST_DIR%


REM IF NOT EXIST %GTEST_DIR%.build (
REM 	mkdir %GTEST_DIR%.build
REM 	pushd %GTEST_DIR%.build
REM 	%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" ..\%GTEST_DIR% -DBUILD_SHARED_LIBS=1
REM 	%VSCOMMAND% gtest.sln /build "%Configuration%|%Platform%"
REM 	popd
REM )
REM IF NOT EXIST gtest-1.6.0 (
REM 	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/gtest-1.6.0.zip
REM )

IF NOT DEFINED L_BOOST_DIR (
	set L_BOOST_DIR=.
)

IF NOT EXIST %TEMP_DIR%\boost_1_%BOOST_MINOR%_0.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/boost_1_%BOOST_MINOR%_0.zip http://sourceforge.net/projects/boost/files/boost/1.%BOOST_MINOR%.0/boost_1_%BOOST_MINOR%_0.zip/download
)

IF NOT EXIST %L_BOOST_DIR%\boost_1_%BOOST_MINOR%_0 (
	echo.Extracting boost. Be patient!

	pushd %L_BOOST_DIR%
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/boost_1_%BOOST_MINOR%_0.zip
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
rem SET Boost_DIR=%CD%
REM SET BOOST_ROOT=%CD%
popd

IF NOT EXIST LuminanceHdrStuff (
	mkdir LuminanceHdrStuff
)
IF NOT EXIST LuminanceHdrStuff\qtpfsgui (
	pushd LuminanceHdrStuff
	%CYGWIN_DIR%\bin\git.exe clone git://git.code.sf.net/p/qtpfsgui/code qtpfsgui
	popd
) ELSE (
	pushd LuminanceHdrStuff\qtpfsgui
	IF %UPDATE_REPO_LUMINANCE% EQU 1 (
		%CYGWIN_DIR%\bin\git.exe pull
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
	
	for %%v in ("libpng", "libjpeg", "lcms2", "exiv2", "libtiff", "libraw", "fftw3", "gsl", "CCfits") do (
		mkdir LuminanceHdrStuff\DEPs\include\%%v
		mkdir LuminanceHdrStuff\DEPs\lib\%%v
		mkdir LuminanceHdrStuff\DEPs\bin\%%v
	)
	
	mkdir LuminanceHdrStuff\DEPs\include\libraw\libraw

	mkdir LuminanceHdrStuff\DEPs\include\gsl\gsl
	copy gsl-1.15\gsl\*.h LuminanceHdrStuff\DEPs\include\gsl\gsl
	copy gsl-1.15\build.vc10\lib\%Platform%\%Configuration%\*.lib LuminanceHdrStuff\DEPs\lib\gsl
	rem copy gsl-1.15\build.vc10\dll\*.dll LuminanceHdrStuff\DEPs\bin\gsl
)


robocopy fftw-%FFTW_VER%-dll LuminanceHdrStuff\DEPs\include\fftw3 *.h /MIR >nul
robocopy fftw-%FFTW_VER%-dll LuminanceHdrStuff\DEPs\lib\fftw3 *.lib /MIR /NJS >nul
robocopy fftw-%FFTW_VER%-dll LuminanceHdrStuff\DEPs\bin\fftw3 *.dll /MIR /NJS >nul


robocopy tiff-4.0.3\libtiff LuminanceHdrStuff\DEPs\include\libtiff *.h /MIR >nul
robocopy tiff-4.0.3\libtiff LuminanceHdrStuff\DEPs\lib\libtiff *.lib /MIR /NJS >nul
robocopy tiff-4.0.3\libtiff LuminanceHdrStuff\DEPs\bin\libtiff *.dll /MIR /NJS >nul

rem robocopy expat: included indirectly in in exiv2
rem robocopy zlib: included indirectly in in exiv2
robocopy exiv2-%EXIV2_COMMIT%\include LuminanceHdrStuff\DEPs\include\exiv2 *.h *.hpp /MIR >nul
robocopy exiv2-%EXIV2_COMMIT%\bin\%Platform%\Dynamic\%Configuration% LuminanceHdrStuff\DEPs\lib\exiv2 *.lib /MIR >nul
robocopy exiv2-%EXIV2_COMMIT%\bin\%Platform%\Dynamic\%Configuration% LuminanceHdrStuff\DEPs\bin\exiv2 *.dll /MIR >nul

robocopy lp170b35 LuminanceHdrStuff\DEPs\include\libpng *.h /MIR >nul
robocopy lp170b35\%Configuration% LuminanceHdrStuff\DEPs\lib\libpng *.lib /MIR >nul
robocopy lp170b35\%Configuration% LuminanceHdrStuff\DEPs\bin\libpng *.dll /MIR >nul
	

robocopy LibRaw-%LIBRAW_COMMIT%\libraw LuminanceHdrStuff\DEPs\include\libraw\libraw /MIR >nul
robocopy LibRaw-%LIBRAW_COMMIT%\lib LuminanceHdrStuff\DEPs\lib\libraw *.lib /MIR >nul
robocopy LibRaw-%LIBRAW_COMMIT%\bin LuminanceHdrStuff\DEPs\bin\libraw *.dll /MIR >nul
	
robocopy lcms2-%LCMS_COMMIT%\include LuminanceHdrStuff\DEPs\include\lcms2 *.h /MIR >nul
robocopy lcms2-%LCMS_COMMIT%\bin LuminanceHdrStuff\DEPs\lib\lcms2 *.lib /MIR /NJS >nul
rem robocopy lcms2-%LCMS_COMMIT%\bin LuminanceHdrStuff\DEPs\bin\lcms2 *.dll /MIR /NJS >nul

robocopy libjpeg-turbo-%LIBJPEG_COMMIT% LuminanceHdrStuff\DEPs\include\libjpeg *.h /MIR >nul
robocopy libjpeg-turbo-%LIBJPEG_COMMIT%.build\sharedlib\%Configuration% LuminanceHdrStuff\DEPs\lib\libjpeg *.lib /MIR /NJS >nul
robocopy libjpeg-turbo-%LIBJPEG_COMMIT%.build\sharedlib\%Configuration% LuminanceHdrStuff\DEPs\bin\libjpeg *.dll /MIR /NJS >nul

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

set L_CMAKE_INCLUDE=..\DEPs\include\libtiff;..\DEPs\include\libpng;..\..\zlib-%ZLIB_COMMIT%;..\..\boost_1_%BOOST_MINOR%_0;..\..\OpenEXR-dk-%OPENEXR_COMMIT%\output\include
set L_CMAKE_LIB=..\DEPs\lib\libtiff;..\DEPs\lib\libpng;..\..\zlib-%ZLIB_COMMIT%\%Configuration%;..\..\boost_1_%BOOST_MINOR%_0\stage\lib;..\..\OpenEXR-dk-%OPENEXR_COMMIT%\output\lib
set L_CMAKE_PROGRAM_PATH=%CYGWIN_DIR%\bin
set L_CMAKE_PREFIX_PATH=%QTDIR%
set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DPC_EXIV2_INCLUDEDIR=..\DEPs\include\exiv2 -DPC_EXIV2_LIBDIR=..\DEPs\lib\exiv2 -DCMAKE_INCLUDE_PATH=%L_CMAKE_INCLUDE% -DCMAKE_LIBRARY_PATH=%L_CMAKE_LIB% -DCMAKE_PROGRAM_PATH=%L_CMAKE_PROGRAM_PATH% -DCMAKE_PREFIX_PATH=%L_CMAKE_PREFIX_PATH% -DPNG_NAMES=libpng16;libpng17 -DOPENEXR_VERSION=%OPENEXR_CMAKE_VERSION%

REM IF EXIST ..\..\gtest-1.6.0 (
REM 	SET GTEST_ROOT=%CD%\..\..\gtest-1.6.0
REM )
echo CMake command line options ------------------------------------
echo %CMAKE_OPTIONS%
echo ---------------------------------------------------------------

%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" ..\qtpfsgui %CMAKE_OPTIONS%
IF errorlevel 1 goto error_end
popd

IF EXIST LuminanceHdrStuff\qtpfsgui.build\Luminance HDR.sln (
	pushd LuminanceHdrStuff\qtpfsgui.build	
	rem %VSCOMMAND% luminance-hdr.sln /Upgrade
	%VSCOMMAND% "Luminance HDR.sln" /build "%ConfigurationLuminance%|%Platform%" /Project luminance-hdr
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
        robocopy libjpeg ..\..\qtpfsgui.build\%ConfigurationLuminance% jpeg8.dll >nul
        robocopy exiv2 ..\..\qtpfsgui.build\%ConfigurationLuminance% exiv2.dll >nul
        robocopy exiv2 ..\..\qtpfsgui.build\%ConfigurationLuminance% zlib.dll >nul
        robocopy exiv2 ..\..\qtpfsgui.build\%ConfigurationLuminance% expat.dll >nul
        robocopy libraw ..\..\qtpfsgui.build\%ConfigurationLuminance% libraw.dll >nul
        robocopy fftw3 ..\..\qtpfsgui.build\%ConfigurationLuminance% libfftw3f-3.dll >nul
        robocopy libpng ..\..\qtpfsgui.build\%ConfigurationLuminance% libpng17.dll >nul
        popd

        robocopy cfit%CFITSIO_VER%.build\%Configuration% LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% cfitsio.dll >nul 
        robocopy %PTHREADS_CURRENT_DIR% LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% pthreadVC2.dll >nul 
       
        robocopy lcms2-%LCMS_COMMIT%\bin LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% lcms2.dll >nul 

		IF NOT EXIST LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\i18n\ (
			mkdir LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\i18n
		)
		IF NOT EXIST LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\help\ (
			mkdir LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\help
		)
		
		robocopy LuminanceHdrStuff\qtpfsgui.build LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\i18n lang_*.qm >nul
		robocopy LuminanceHdrStuff\qtpfsgui\help LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\help /MIR >nul


        REM ----- QT Stuff (Dlls, translations) --------------------------------------------
        pushd LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%

        for %%v in ( "Qt5Concurrent.dll", "Qt5Core.dll", "Qt5Gui.dll", "Qt5Multimedia.dll", "Qt5MultimediaWidgets.dll", "Qt5Network.dll", "Qt5Positioning.dll", "Qt5WinExtras.dll", "Qt5OpenGL.dll", "Qt5PrintSupport.dll", "Qt5Qml.dll", "Qt5Quick.dll", "Qt5Sensors.dll", "Qt5Sql.dll", "Qt5V8.dll", "Qt5WebKit.dll", "Qt5WebKitWidgets.dll", "Qt5Widgets.dll", "Qt5Xml.dll", "icudt51.dll", "icuin51.dll", "icuuc51.dll" ) do (
            robocopy %QTDIR%\bin . %%v >nul
        )
        for %%v in ("imageformats", "sqldrivers", "platforms") do (
            IF NOT EXIST %%v (
                mkdir %%v
            )        
        )
        robocopy %QTDIR%\plugins\imageformats imageformats qjpeg.dll >nul
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