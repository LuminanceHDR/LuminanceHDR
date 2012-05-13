@echo off
SETLOCAL

devenv /? > NUL
IF ERRORLEVEL 1 (
	echo.
	echo.ERROR: This file must be run inside a VS command prompt!
	echo.
	goto error_end
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
) ELSE (
	set Platform=x64
	set RawPlatform=x64
)
IF DEFINED VS100COMNTOOLS (
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


IF NOT EXIST %CYGWIN_DIR%\bin\cvs.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\git.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\gzip.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\sed.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\ssh.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\svn.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\tar.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\unzip.exe GOTO cygwin_error
IF NOT EXIST %CYGWIN_DIR%\bin\wget.exe GOTO cygwin_error
GOTO cygwin_ok

:cygwin_error
echo ERROR: Cygwin with 
echo    cvs
echo    git 
echo    gzip 
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

cls
echo.
echo.--- %VS_CMAKE% ---
echo.Configuration = %Configuration%
echo.Platform = %Platform% (%RawPlatform%)
echo.

IF NOT EXIST %TEMP_DIR% (
	mkdir %TEMP_DIR%
)

IF NOT EXIST %TEMP_DIR%\align_image_stack_%RawPlatform%.exe (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/align_image_stack_%RawPlatform%.exe qtpfsgui.sourceforge.net/win/align_image_stack_%RawPlatform%.exe
)

IF NOT EXIST %TEMP_DIR%\zlib125.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/zlib125.zip http://prdownloads.sourceforge.net/libpng/zlib125.zip?download
)
IF NOT EXIST zlib-1.2.5 (
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/zlib125.zip
	pushd zlib-1.2.5\contrib\masmx64
	call bld_ml64.bat
	cd ..\masmx86
	call bld_ml32.bat
	cd ..\vstudio\%VS_SHORT%
	devenv zlibvc.sln /build "%Configuration%|%Platform%"
	popd
)

REM zlib copy for libpng
IF NOT EXIST zlib (
	mkdir zlib
	copy zlib-1.2.5\*.h zlib
	copy zlib-1.2.5\contrib\vstudio\%VS_SHORT%\%RawPlatform%\ZlibDll%Configuration%\*.lib zlib
	copy zlib-1.2.5\contrib\vstudio\%VS_SHORT%\%RawPlatform%\ZlibDll%Configuration%\*.dll zlib
)

IF NOT EXIST %TEMP_DIR%\lpng1510.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/lpng1510.zip http://prdownloads.sourceforge.net/libpng/lpng1510.zip?download
)
IF NOT EXIST lpng1510 (
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/lpng1510.zip
	pushd lpng1510
	nmake /f scripts\makefile.vcwin32
	popd
)

IF NOT EXIST %TEMP_DIR%\expat-2.0.1.tar (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/expat-2.0.1.tar.gz http://sourceforge.net/projects/expat/files/expat/2.0.1/expat-2.0.1.tar.gz/download
	%CYGWIN_DIR%\bin\gzip.exe -d %TEMP_DIR%/expat-2.0.1.tar.gz
)
IF NOT EXIST expat-2.0.1 (
	%CYGWIN_DIR%\bin\tar.exe -xf %TEMP_DIR%/expat-2.0.1.tar
)


IF NOT EXIST exiv2-trunk (
	set exiv2-compile=true
	%CYGWIN_DIR%\bin\svn.exe co -r 2732 svn://dev.exiv2.org/svn/trunk exiv2-trunk
) ELSE (
	rem svn update exiv2-trunk
	rem set exiv2-compile=true
)

IF DEFINED exiv2-compile (
	REM msvc64 is the right one for Win32 too
	pushd exiv2-trunk\msvc64 		
	devenv exiv2.sln /upgrade
	devenv exiv2.sln /build "%Configuration%DLL|%Platform%" 
	popd
)


IF NOT EXIST %TEMP_DIR%\jpegsr8d.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/jpegsr8d.zip http://www.ijg.org/files/jpegsr8d.zip
)
IF NOT EXIST libjpeg (
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/jpegsr8d.zip
	ren jpeg-8d libjpeg

	pushd libjpeg
	copy jconfig.vc jconfig.h
	copy makejsln.v10 makejsln.sln
	copy makeasln.v10 makeasln.sln
	copy makejvcx.v10 jpeg.vcxproj
	copy makecvcx.v10 cjpeg.vcxproj
	copy makedvcx.v10 djpeg.vcxproj
	copy maketvcx.v10 jpegtran.vcxproj
	copy makewvcx.v10 wrjpgcom.vcxproj
	copy makervcx.v10 rdjpgcom.vcxpr

	nmake /f makefile.vc
	popd
)


REM IF NOT EXIST %TEMP_DIR%\lcms-1.19.VC10.x64x86.zip (
REM 	REM Custom download for having Visual Studio Solution with x64 support configured
REM 	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/lcms-1.19.VC10.x64x86.zip qtpfsgui.sourceforge.net/win/lcms-1.19.VC10.x64x86.zip
REM )
REM 
REM IF NOT EXIST lcms-1.19 (
REM 	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/lcms-1.19.VC10.x64x86.zip
REM 	
REM 	pushd lcms-1.19
REM 	devenv Projects\VC2008\lcms.sln /Upgrade
REM 	devenv Projects\VC2008\lcms.sln /build "%Configuration%|%Platform%"  /Project lcmsdll
REM 	copy Lib\MS\lcmsdll.lib bin\lcms.lib
REM 	popd
REM )

IF NOT EXIST %TEMP_DIR%\lcms2-2.3.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/lcms2-2.3.zip http://sourceforge.net/projects/lcms/files/lcms/2.3/lcms2-2.3.zip/download
)


IF NOT EXIST lcms2-2.3 (
	%CYGWIN_DIR%\bin\git.exe clone git://github.com/danielkaneider/Little-CMS.git lcms2-2.3
	REM %CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/lcms2-2.3.zip
	
	pushd lcms2-2.3
	devenv Projects\VC2010\lcms2.sln /Upgrade
	devenv Projects\VC2010\lcms2.sln /build "%Configuration%|%Platform%"  /Project lcms2_DLL
	popd
)

IF NOT EXIST %TEMP_DIR%\tiff-4.0.1.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/tiff-4.0.1.zip http://download.osgeo.org/libtiff/tiff-4.0.1.zip
)

IF NOT EXIST tiff-4.0.1 (
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/tiff-4.0.1.zip

	echo.JPEG_SUPPORT=^1> tiff-4.0.1\qtpfsgui_commands.in
	echo.JPEGDIR=..\..\libjpeg>> tiff-4.0.1\qtpfsgui_commands.in
	echo.JPEG_INCLUDE=-I$^(JPEGDIR^)>> tiff-4.0.1\qtpfsgui_commands.in
	echo.JPEG_LIB=$^(JPEGDIR^)\libjpeg.lib>> tiff-4.0.1\qtpfsgui_commands.in
	echo.ZIP_SUPPORT=^1>> tiff-4.0.1\qtpfsgui_commands.in
	echo.ZLIBDIR=..\..\zlib-1.2.5\contrib\vstudio\%VS_SHORT%\%RawPlatform%\ZlibDll%Configuration%>> tiff-4.0.1\qtpfsgui_commands.in
	echo.ZLIB_INCLUDE=-I..\..\zlib-1.2.5>> tiff-4.0.1\qtpfsgui_commands.in
	echo.ZLIB_LIB=$^(ZLIBDIR^)\zlibwapi.lib>> tiff-4.0.1\qtpfsgui_commands.in

	pushd tiff-4.0.1
	nmake /s /c /f Makefile.vc @qtpfsgui_commands.in
	popd
)


IF NOT EXIST %TEMP_DIR%\LibRaw-0.14.6.tar (
	rem %CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/LibRaw-0.14.6.tar.gz http://www.libraw.org/data/LibRaw-0.14.6.tar.gz
	rem %CYGWIN_DIR%\bin\gzip.exe -d %TEMP_DIR%/LibRaw-0.14.6.tar.gz
)
IF NOT EXIST %TEMP_DIR%\LibRaw-demosaic-pack-GPL2-0.14.6.tar (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/LibRaw-demosaic-pack-GPL2-0.14.6.tar.gz http://www.libraw.org/data/LibRaw-demosaic-pack-GPL2-0.14.6.tar.gz
	%CYGWIN_DIR%\bin\gzip.exe -d %TEMP_DIR%/LibRaw-demosaic-pack-GPL2-0.14.6.tar.gz
)
IF NOT EXIST LibRaw-demosaic-pack-GPL2-0.14.6 (
	rem %CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/LibRaw-demosaic-pack-GPL2-0.14.6.tar.gz
	
	%CYGWIN_DIR%\bin\tar.exe -xf %TEMP_DIR%/LibRaw-demosaic-pack-GPL2-0.14.6.tar
)

IF NOT EXIST LibRaw-0.14.6 (
	rem %CYGWIN_DIR%\bin\gzip.exe -d %TEMP_DIR%/LibRaw-0.14.6.tar.gz
	rem %CYGWIN_DIR%\bin\tar.exe -xf %TEMP_DIR%/LibRaw-0.14.6.tar
	REM %CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/LibRaw-0.14.6.zip
	%CYGWIN_DIR%\bin\git.exe clone git://github.com/danielkaneider/LibRaw.git LibRaw-0.14.6
	
	
	pushd LibRaw-0.14.6
	
	rem echo.COPT_OPT="/openmp"> qtpfsgui_commands.in
	echo.CFLAGS_DP2=/I..\LibRaw-demosaic-pack-GPL2-0.14.6> qtpfsgui_commands.in
	echo.CFLAGSG2=/DLIBRAW_DEMOSAIC_PACK_GPL2>> qtpfsgui_commands.in
	rem echo.LCMS_DEF="/DUSE_LCMS2 /DCMS_DLL /I..\lcms2-2.3\include">> qtpfsgui_commands.in
	rem echo.LCMS_LIB="..\lcms2-2.3\bin\lcms2_dll.lib">> qtpfsgui_commands.in
	echo.LCMS_DEF="/DUSE_LCMS /DLCMS_DLL /I..\lcms-1.19\include">> qtpfsgui_commands.in
	echo.LCMS_LIB="..\lcms-1.19\bin\lcms.lib">> qtpfsgui_commands.in

	nmake /f Makefile.msvc @qtpfsgui_commands.in clean
	nmake /f Makefile.msvc @qtpfsgui_commands.in bin\libraw.dll
	popd
)

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
	devenv gsl.lib.sln /Upgrade
	devenv gsl.lib.sln /build "%Configuration%|%Platform%" /Project gslhdrs
	gslhdrs\%Platform%\%Configuration%\gslhdrs.exe
	devenv gsl.lib.sln /build "%Configuration%|%Platform%" /Project gsllib
	popd
)

IF NOT EXIST OpenExrStuff (
	pushd .
	mkdir OpenExrStuff
	cd OpenExrStuff
	
	for %%v in ("Deploy", "Deploy\include", "Deploy\lib\%Platform%\%Configuration%", "Deploy\bin\%Platform%\%Configuration%", "openexr-cvs") do (
		mkdir %%v
	)
	popd
	
	copy zlib-1.2.5\*.h OpenExrStuff\Deploy\include
	copy zlib-1.2.5\contrib\vstudio\%VS_SHORT%\%RawPlatform%\ZlibDll%Configuration%\*.lib OpenExrStuff\Deploy\lib\%Platform%\%Configuration%
	copy zlib-1.2.5\contrib\vstudio\%VS_SHORT%\%RawPlatform%\ZlibDll%Configuration%\*.dll OpenExrStuff\Deploy\bin\%Platform%\%Configuration%
)
	
pushd OpenExrStuff\openexr-cvs
IF NOT EXIST IlmBase (
	%CYGWIN_DIR%\bin\cvs.exe -d :pserver:anonymous:anonymous@cvs.savannah.nongnu.org:/sources/openexr co IlmBase
	set openexr-compile=true
) ELSE (
	rem %CYGWIN_DIR%\bin\cvs.exe -d :pserver:anonymous:anonymous@cvs.savannah.nongnu.org:/sources/openexr update IlmBase
)
IF NOT EXIST OpenEXR (
	%CYGWIN_DIR%\bin\cvs.exe -d :pserver:anonymous:anonymous@cvs.savannah.nongnu.org:/sources/openexr co OpenEXR
	set openexr-compile=true
) ELSE (
	rem %CYGWIN_DIR%\bin\cvs.exe -d :pserver:anonymous:anonymous@cvs.savannah.nongnu.org:/sources/openexr update OpenEXR
)
popd

IF DEFINED openexr-compile (
	pushd OpenExrStuff\openexr-cvs\IlmBase\vc\vc9\IlmBase
	devenv IlmBase.sln /Upgrade
	devenv IlmBase.sln /build "%Configuration%|%Platform%"
	popd

	pushd OpenExrStuff\openexr-cvs\OpenEXR\vc\vc8\OpenEXR
	devenv OpenEXR.sln /Upgrade
	devenv OpenEXR.sln /build "%Configuration%|%Platform%" /Project IlmImf
	popd
)

IF %Platform% EQU Win32 (
	IF NOT EXIST %TEMP_DIR%\fftw-3.3.1.pl1-dll32.zip (
		%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/fftw-3.3.1.pl1-dll32.zip ftp://ftp.fftw.org/pub/fftw/fftw-3.3.1.pl1-dll32.zip
	)
) ELSE (
	IF NOT EXIST %TEMP_DIR%\fftw-3.3.1-dll64.zip (
		%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/fftw-3.3.1-dll64.zip ftp://ftp.fftw.org/pub/fftw/fftw-3.3.1-dll64.zip
		
	)
)

IF NOT EXIST fftw-3.3.1-dll (
	IF %Platform% EQU Win32 (
		%CYGWIN_DIR%\bin\unzip.exe -q -d fftw-3.3.1-dll %TEMP_DIR%/fftw-3.3.1.pl1-dll32.zip
	) ELSE (
		%CYGWIN_DIR%\bin\unzip.exe -q -d fftw-3.3.1-dll %TEMP_DIR%/fftw-3.3.1-dll64.zip
	)

	pushd fftw-3.3.1-dll
	lib /def:libfftw3-3.def
	lib /def:libfftw3f-3.def
	lib /def:libfftw3l-3.def
	popd
)

REM IF NOT DEFINED L_BOOST_DIR (
REM 	set L_BOOST_DIR=.
REM )
REM 
REM IF NOT EXIST %TEMP_DIR%\boost_1_47_0.tar.gz (
REM 	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/boost_1_47_0.tar.gz http://sourceforge.net/projects/boost/files/boost/1.47.0/boost_1_47_0.tar.gz/download
REM 	%CYGWIN_DIR%\bin\gzip.exe -d %TEMP_DIR%/boost_1_47_0.tar.gz
REM 
REM )
REM 
REM IF NOT EXIST %L_BOOST_DIR%\boost_1_47_0 (
REM 	echo.Extracting boost. Be patient!
REM 
REM 	pushd %L_BOOST_DIR%
REM 	%CYGWIN_DIR%\bin\tar.exe -xf %TEMP_DIR%/boost_1_47_0.tar
REM 	popd
REM 
REM 	pushd %L_BOOST_DIR%\boost_1_47_0
REM 	bootstrap.bat
REM 	popd
REM 	
REM 	pushd %L_BOOST_DIR%\boost_1_47_0
REM 	IF %Platform% EQU Win32 (
REM 		IF %Configuration% EQU Release (
REM 			b2.exe toolset=msvc variant=release
REM 		) ELSE (
REM 			b2.exe toolset=msvc variant=debug
REM 		)
REM 	) ELSE (
REM 		IF %Configuration% EQU Release (
REM 			b2.exe toolset=msvc variant=release address-model=64
REM 		) ELSE (
REM 			b2.exe toolset=msvc variant=debug address-model=64
REM 		)
REM 	)
REM 	popd
REM )
REM 
REM REM Set Boost-directory as ENV variable (needed for CMake)
REM pushd %L_BOOST_DIR%\boost_1_47_0
REM SET BOOST_ROOT=%CD%
REM popd

IF NOT EXIST LuminanceHdrStuff (
	mkdir LuminanceHdrStuff
)
IF NOT EXIST LuminanceHdrStuff\qtpfsgui (
	pushd LuminanceHdrStuff
	%CYGWIN_DIR%\bin\git.exe clone git://qtpfsgui.git.sourceforge.net/gitroot/qtpfsgui/qtpfsgui qtpfsgui
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
	
	for %%v in ("libpng", "libjpeg", "lcms2", "exiv2", "libtiff", "libraw", "OpenEXR", "fftw3", "gsl") do (
		mkdir LuminanceHdrStuff\DEPs\include\%%v
		mkdir LuminanceHdrStuff\DEPs\lib\%%v
		mkdir LuminanceHdrStuff\DEPs\bin\%%v
	)
	

	copy lpng1510\*.h   LuminanceHdrStuff\DEPs\include\libpng
	copy lpng1510\*.lib LuminanceHdrStuff\DEPs\lib\libpng
	rem copy lpng1510\*.dll LuminanceHdrStuff\DEPs\bin\libpng
	
	copy libjpeg\*.h LuminanceHdrStuff\DEPs\include\libjpeg
	
	copy lcms2-2.3\include\*.h LuminanceHdrStuff\DEPs\include\lcms2
	copy lcms2-2.3\bin\*.lib LuminanceHdrStuff\DEPs\lib\lcms2
	copy lcms2-2.3\bin\*.dll LuminanceHdrStuff\DEPs\bin\lcms2

	REM copy lcms-1.19\include\*.h LuminanceHdrStuff\DEPs\include\lcms2
	REM copy lcms-1.19\bin\*.lib LuminanceHdrStuff\DEPs\lib\lcms2
	REM copy lcms-1.19\bin\*.dll LuminanceHdrStuff\DEPs\bin\lcms2
	
	copy exiv2-trunk\msvc64\include\* LuminanceHdrStuff\DEPs\include\exiv2
	copy exiv2-trunk\msvc64\include\exiv2\* LuminanceHdrStuff\DEPs\include\exiv2

	copy exiv2-trunk\msvc64\exiv2lib\%Platform%\%Configuration%DLL\*.lib LuminanceHdrStuff\DEPs\lib\exiv2
	copy exiv2-trunk\msvc64\exiv2lib\%Platform%\%Configuration%DLL\*.dll LuminanceHdrStuff\DEPs\bin\exiv2
	
	copy tiff-4.0.1\libtiff\*.h LuminanceHdrStuff\DEPs\include\libtiff
	copy tiff-4.0.1\libtiff\*.lib LuminanceHdrStuff\DEPs\lib\libtiff
	copy tiff-4.0.1\libtiff\*.dll LuminanceHdrStuff\DEPs\bin\libtiff
	
	mkdir LuminanceHdrStuff\DEPs\include\libraw\libraw
	copy LibRaw-0.14.6\libraw\*.h LuminanceHdrStuff\DEPs\include\libraw\libraw
	copy LibRaw-0.14.6\lib\*.lib LuminanceHdrStuff\DEPs\lib\libraw
	copy LibRaw-0.14.6\bin\*.dll LuminanceHdrStuff\DEPs\bin\libraw
	
	copy OpenExrStuff\Deploy\include\*.h LuminanceHdrStuff\DEPs\include\OpenEXR
	copy OpenExrStuff\Deploy\lib\%Platform%\%Configuration%\*.lib LuminanceHdrStuff\DEPs\lib\OpenEXR
	copy OpenExrStuff\Deploy\bin\%Platform%\%Configuration%\*.dll LuminanceHdrStuff\DEPs\bin\OpenEXR

	copy fftw-3.3.1-dll\*.h LuminanceHdrStuff\DEPs\include\fftw3
	copy fftw-3.3.1-dll\*.lib LuminanceHdrStuff\DEPs\lib\fftw3
	copy fftw-3.3.1-dll\*.dll LuminanceHdrStuff\DEPs\bin\fftw3

	mkdir LuminanceHdrStuff\DEPs\include\gsl\gsl
	copy gsl-1.15\gsl\*.h LuminanceHdrStuff\DEPs\include\gsl\gsl
	copy gsl-1.15\build.vc10\lib\%Platform%\%Configuration%\*.lib LuminanceHdrStuff\DEPs\lib\gsl
	copy gsl-1.15\build.vc10\dll\*.dll LuminanceHdrStuff\DEPs\bin\gsl
	
)

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
%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" ..\qtpfsgui %CMAKE_OPTIONS%
popd

IF EXIST LuminanceHdrStuff\qtpfsgui.build\luminance-hdr.sln (
	pushd LuminanceHdrStuff\qtpfsgui.build	
	devenv luminance-hdr.sln /Upgrade
	devenv luminance-hdr.sln /build "%Configuration%|%Platform%"
	popd
)

IF EXIST LuminanceHdrStuff\qtpfsgui.build\%Configuration%\luminance-hdr.exe (
	IF EXIST LuminanceHdrStuff\qtpfsgui.build\%Configuration% (
		
		IF NOT EXIST LuminanceHdrStuff\qtpfsgui.build\%Configuration%\LICENSE.txt (
			copy LuminanceHdrStuff\qtpfsgui\LICENSE LuminanceHdrStuff\qtpfsgui.build\%Configuration%\LICENSE.txt
		)
		IF NOT EXIST LuminanceHdrStuff\qtpfsgui.build\%Configuration%\align_image_stack.exe (
			copy %TEMP_DIR%\align_image_stack_%RawPlatform%.exe LuminanceHdrStuff\qtpfsgui.build\%Configuration%\align_image_stack.exe
		)
		
		IF EXIST LuminanceHdrStuff\qtpfsgui.build\QtDlls\%Configuration%\ (
			IF NOT EXIST LuminanceHdrStuff\qtpfsgui.build\%Configuration%\zlib1.dll (
				mkdir LuminanceHdrStuff\qtpfsgui.build\%Configuration%\imageformats\
				mkdir LuminanceHdrStuff\qtpfsgui.build\%Configuration%\sqldrivers\
				copy LuminanceHdrStuff\qtpfsgui.build\QtDlls\%Configuration%\* LuminanceHdrStuff\qtpfsgui.build\%Configuration%\
				copy LuminanceHdrStuff\qtpfsgui.build\QtDlls\%Configuration%\imageformats\* LuminanceHdrStuff\qtpfsgui.build\%Configuration%\imageformats\
				copy LuminanceHdrStuff\qtpfsgui.build\QtDlls\%Configuration%\sqldrivers\* LuminanceHdrStuff\qtpfsgui.build\%Configuration%\sqldrivers\
			)
		)

		IF NOT EXIST LuminanceHdrStuff\qtpfsgui.build\%Configuration%\zlib1.dll (
			pushd LuminanceHdrStuff\DEPs\bin
			for %%v in ("lcms2\lcms2_DLL.dll", "lcms2\lcms.dll", "exiv2\exiv2.dll", "exiv2\libexpat.dll", "exiv2\zlib1.dll", "OpenEXR\Half.dll", "OpenEXR\Iex.dll", "OpenEXR\IlmImf.dll", "OpenEXR\IlmThread.dll", "OpenEXR\zlibwapi.dll", "libraw\libraw.dll", "fftw3\libfftw3f-3.dll") do (
				copy %%v ..\..\qtpfsgui.build\%Configuration%
			)
			popd
		)
		IF NOT EXIST LuminanceHdrStuff\qtpfsgui.build\%Configuration%\i18n\ (
			mkdir LuminanceHdrStuff\qtpfsgui.build\%Configuration%\i18n
			copy LuminanceHdrStuff\qtpfsgui.build\QtDlls\i18n\*.qm LuminanceHdrStuff\qtpfsgui.build\%Configuration%\i18n
			copy LuminanceHdrStuff\qtpfsgui.build\*.qm LuminanceHdrStuff\qtpfsgui.build\%Configuration%\i18n
		)
		IF NOT EXIST LuminanceHdrStuff\qtpfsgui.build\%Configuration%\help\ (
			mkdir LuminanceHdrStuff\qtpfsgui.build\%Configuration%\help
			xcopy LuminanceHdrStuff\qtpfsgui\help LuminanceHdrStuff\qtpfsgui.build\%Configuration%\help /D /E /C /R /H /I /K /Y
		)
	)
)



goto end

:error_end
pause

:end

endlocal