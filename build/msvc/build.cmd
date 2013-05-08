@echo off
SETLOCAL

REM http://dev.exiv2.org/projects/exiv2/repository/
SET EXIV2_COMMIT=3015
REM sourceforge.net/p/libjpeg-turbo/code/
SET LIBJPEG_COMMIT=942


rem https://github.com/madler/zlib/commits
rem SET ZLIB_COMMIT=0b16609
rem SET ZLIB_COMMIT_LONG=0b166094092efa2b92200cbb67f390e86c181ab4
SET ZLIB_COMMIT=b06dee4
SET ZLIB_COMMIT_LONG=b06dee43696b5057ee8e1b9700655ad9e7d89669

SET OPENEXR_COMMIT=3c2f7b9
SET OPENEXR_COMMIT_LONG=3c2f7b956e57d8569f53823b4889921716276474


SET LCMS_COMMIT=cde00fd
SET LCMS_COMMIT_LONG=cde00fd7dbe74e275aceb4a9055bbb1ae6bf93b2

SET LIBRAW_COMMIT=869ed7e
SET LIBRAW_COMMIT_LONG=869ed7e2ebca24c654401008903f188ac3e0d287
SET LIBRAW_DEMOS2_COMMIT=028c410
SET LIBRAW_DEMOS2_COMMIT_LONG=028c41031044c8f2bece04c3fb68d2d01368b7ae
SET LIBRAW_DEMOS3_COMMIT=f089589
SET LIBRAW_DEMOS3_COMMIT_LONG=f0895891fdaa775255af02275fce426a5bf5c9fc

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

IF NOT EXIST %TEMP_DIR%\%RawPlatform% (
    mkdir %TEMP_DIR%\%RawPlatform%
)

IF NOT EXIST %TEMP_DIR%\%RawPlatform%\align_image_stack.exe (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/%RawPlatform%/align_image_stack.exe qtpfsgui.sourceforge.net/win/%RawPlatform%/align_image_stack.exe
  	IF %Platform% EQU Win32 (
        %CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/%RawPlatform%/huginbase.dll qtpfsgui.sourceforge.net/win/%RawPlatform%/huginbase.dll
        %CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/%RawPlatform%/huginvigraimpex.dll qtpfsgui.sourceforge.net/win/%RawPlatform%/huginvigraimpex.dll
        %CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/%RawPlatform%/msvcp100.dll qtpfsgui.sourceforge.net/win/%RawPlatform%/msvcp100.dll
        %CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/%RawPlatform%/msvcr100.dll qtpfsgui.sourceforge.net/win/%RawPlatform%/msvcr100.dll
	)
)

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
	devenv zlib.sln /build "%Configuration%|%Platform%" /Project zlib
	IF errorlevel 1 goto error_end
	popd
)

IF NOT EXIST %TEMP_DIR%\lpng161.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/lpng161.zip http://sourceforge.net/projects/libpng/files/libpng16/1.6.1/lpng161.zip/download
)
IF NOT EXIST lpng161 (
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/lpng161.zip
	pushd lpng161
	%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" . -DZLIB_ROOT=..\zlib-%ZLIB_COMMIT%;..\zlib-%ZLIB_COMMIT%\%Configuration%
	IF errorlevel 1	goto error_end
	devenv libpng.sln /build "%Configuration%|%Platform%" /Project png16
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
	devenv expat.sln /build "%Configuration%|%Platform%" /Project expat
	IF errorlevel 1 goto error_end
	popd
)

IF NOT EXIST exiv2-%EXIV2_COMMIT% (
	%CYGWIN_DIR%\bin\svn.exe co -r %EXIV2_COMMIT% svn://dev.exiv2.org/svn/trunk exiv2-%EXIV2_COMMIT%
    
    pushd exiv2-%EXIV2_COMMIT%
	%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%"  -DZLIB_ROOT=..\zlib-%ZLIB_COMMIT%;..\zlib-%ZLIB_COMMIT%\Release
	IF errorlevel 1 goto error_end
	devenv exiv2.sln /build "%Configuration%|%Platform%" /Project exiv2
	IF errorlevel 1 goto error_end
    copy bin\%Platform%\Dynamic\*.h include
	popd  
)

IF NOT EXIST libjpeg-turbo-%LIBJPEG_COMMIT% (
    %CYGWIN_DIR%\bin\svn.exe co -r %LIBJPEG_COMMIT% svn://svn.code.sf.net/p/libjpeg-turbo/code/trunk libjpeg-turbo-%LIBJPEG_COMMIT%
    IF NOT EXIST libjpeg-turbo-%LIBJPEG_COMMIT%.build (
        mkdir libjpeg-turbo-%LIBJPEG_COMMIT%.build
    )
	pushd libjpeg-turbo-%LIBJPEG_COMMIT%.build
	%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" -DCMAKE_BUILD_TYPE=%Configuration% -DNASM="%CYGWIN_DIR%\bin\nasm.exe" -DWITH_JPEG8=TRUE ..\libjpeg-turbo-%LIBJPEG_COMMIT%
	IF errorlevel 1 goto error_end
	devenv libjpeg-turbo.sln /build "%Configuration%|%Platform%"
	IF errorlevel 1 goto error_end
    copy jconfig.h ..\libjpeg-turbo-%LIBJPEG_COMMIT%
	popd
)

IF NOT EXIST %TEMP_DIR%\lcms2-%LCMS_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/lcms2-%LCMS_COMMIT%.zip --no-check-certificate https://github.com/mm2/Little-CMS/zipball/%LCMS_COMMIT_LONG%
)


IF NOT EXIST lcms2-%LCMS_COMMIT% (
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/lcms2-%LCMS_COMMIT%.zip
	%CYGWIN_DIR%\bin\mv.exe mm2-Little-CMS-* lcms2-%LCMS_COMMIT%
	
	pushd lcms2-%LCMS_COMMIT%
	devenv Projects\VC2010\lcms2.sln /Upgrade
	devenv Projects\VC2010\lcms2.sln /build "%Configuration%|%Platform%"  /Project lcms2_DLL
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
	
	rem echo.COPT_OPT="/openmp"> qtpfsgui_commands.in
	echo.CFLAGS_DP2=/I..\LibRaw-demosaic-pack-GPL2-%LIBRAW_DEMOS2_COMMIT%> qtpfsgui_commands.in
	echo.CFLAGSG2=/DLIBRAW_DEMOSAIC_PACK_GPL2>> qtpfsgui_commands.in
	echo.CFLAGS_DP3=/I..\LibRaw-demosaic-pack-GPL3-%LIBRAW_DEMOS3_COMMIT%>> qtpfsgui_commands.in
	echo.CFLAGSG3=/DLIBRAW_DEMOSAIC_PACK_GPL3>> qtpfsgui_commands.in
	echo.LCMS_DEF="/DUSE_LCMS2 /DCMS_DLL /I..\lcms2-%LCMS_COMMIT%\include">> qtpfsgui_commands.in
	echo.LCMS_LIB="..\lcms2-%LCMS_COMMIT%\bin\lcms2_dll.lib">> qtpfsgui_commands.in
    echo.JPEG_DEF="/DUSE_JPEG8 /DUSE_JPEG /I..\libjpeg-turbo-%LIBJPEG_COMMIT%">> qtpfsgui_commands.in
    echo.JPEG_LIB="..\libjpeg-turbo-%LIBJPEG_COMMIT%.build\sharedlib\%Configuration%\jpeg.lib">> qtpfsgui_commands.in
	
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
	
	copy zlib-%ZLIB_COMMIT%\*.h OpenExrStuff\Deploy\include
	copy zlib-%ZLIB_COMMIT%\%Configuration%\*.lib OpenExrStuff\Deploy\lib\%Platform%\%Configuration%
	copy zlib-%ZLIB_COMMIT%\%Configuration%\*.dll OpenExrStuff\Deploy\bin\%Platform%\%Configuration%
)

IF NOT EXIST %TEMP_DIR%\OpenEXR-%OPENEXR_COMMIT%.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/OpenEXR-%OPENEXR_COMMIT%.zip --no-check-certificate https://github.com/openexr/openexr/zipball/%OPENEXR_COMMIT_LONG%
)

IF NOT EXIST OpenEXR-%OPENEXR_COMMIT% (
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/OpenEXR-%OPENEXR_COMMIT%.zip
	%CYGWIN_DIR%\bin\mv.exe openexr-openexr-* OpenEXR-%OPENEXR_COMMIT%

	pushd OpenEXR-%OPENEXR_COMMIT%

    IF NOT EXIST zlib (
        mkdir zlib
        mkdir zlib\include
        mkdir zlib\lib
        robocopy ..\zlib-%ZLIB_COMMIT%\ zlib\include *.h  >nul
        robocopy ..\zlib-%ZLIB_COMMIT%\%Configuration%\ zlib\lib *.dll *.exp *.lib  >nul
    )
	
    IF NOT EXIST IlmBase.build (
        mkdir IlmBase.build
    )
	pushd IlmBase.build
	%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" -DCMAKE_BUILD_TYPE=%Configuration% ../IlmBase -DCMAKE_INSTALL_PREFIX=output
	IF errorlevel 1 goto error_end
	devenv IlmBase.sln /build "%Configuration%|%Platform%"
	IF errorlevel 1 goto error_end
	devenv IlmBase.sln /build "%Configuration%|%Platform%" /Project INSTALL
	IF errorlevel 1 goto error_end
	popd
	
    copy IlmBase\Half\halfExport.h IlmBase.build\output\include\OpenEXR
    
    IF NOT EXIST OpenEXR.build (
        mkdir OpenEXR.build
    )
      
	pushd OpenEXR.build
	%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" -DCMAKE_BUILD_TYPE=%Configuration% ^
        -DZLIB_ROOT=..\zlib ^
        -DILMBASE_PACKAGE_PREFIX=%CD%\OpenEXR-%OPENEXR_COMMIT%\IlmBase.build\output ^
        -DCMAKE_INSTALL_PREFIX=output ^
        ../OpenEXR
	IF errorlevel 1 goto error_end
	devenv OpenEXR.sln /build "%Configuration%|%Platform%" /Project IlmImf
	rem IF errorlevel 1 goto error_end
	popd
    copy OpenEXR\IlmImf\*.h IlmBase.build\output\include\OpenEXR
    copy OpenEXR.build\IlmImf\%Configuration%\*.lib IlmBase.build\output\lib
    copy OpenEXR\config\*.h IlmBase.build\output\include\OpenEXR 
    
	popd
)
    
IF %Platform% EQU Win32 (
	IF NOT EXIST %TEMP_DIR%\fftw-3.3.3-dll32.zip (
		%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/fftw-3.3.3-dll32.zip ftp://ftp.fftw.org/pub/fftw/fftw-3.3.3-dll32.zip
	)
) ELSE (
	IF NOT EXIST %TEMP_DIR%\fftw-3.3.3-dll64.zip (
		%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/fftw-3.3.3-dll64.zip ftp://ftp.fftw.org/pub/fftw/fftw-3.3.3-dll64.zip
		
	)
)

IF NOT EXIST fftw-3.3.3-dll (
	IF %Platform% EQU Win32 (
		%CYGWIN_DIR%\bin\unzip.exe -q -d fftw-3.3.3-dll %TEMP_DIR%/fftw-3.3.3-dll32.zip
	) ELSE (
		%CYGWIN_DIR%\bin\unzip.exe -q -d fftw-3.3.3-dll %TEMP_DIR%/fftw-3.3.3-dll64.zip
	)

	pushd fftw-3.3.3-dll
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

REM GTest Patch for VS2012:
REM in internal_utils.cmake ~60
REM  if (MSVC_VERSION EQUAL 1700)
REM  	set(cxx_base_flags "${cxx_base_flags} -D_VARIADIC_MAX=10")
REM  endif ()
REM
REM IF NOT EXIST %TEMP_DIR%\gtest-1.6.0.zip (
REM 	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/gtest-1.6.0.zip http://googletest.googlecode.com/files/gtest-1.6.0.zip
REM )
REM 
REM IF NOT EXIST gtest-1.6.0 (
REM 	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/gtest-1.6.0.zip
REM 	
REM 	mkdir gtest-1.6.0.build
REM 	pushd gtest-1.6.0.build
REM 	%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" ..\gtest-1.6.0 -DBUILD_SHARED_LIBS=1
REM 	devenv gtest.sln /build "%Configuration%|%Platform%"
REM 	popd
REM )


IF NOT DEFINED L_BOOST_DIR (
	set L_BOOST_DIR=.
)

IF NOT EXIST %TEMP_DIR%\boost_1_53_0.zip (
	%CYGWIN_DIR%\bin\wget.exe -O %TEMP_DIR%/boost_1_53_0.zip http://sourceforge.net/projects/boost/files/boost/1.53.0/boost_1_53_0.zip/download
)

IF NOT EXIST %L_BOOST_DIR%\boost_1_53_0 (
	echo.Extracting boost. Be patient!

	pushd %L_BOOST_DIR%
	%CYGWIN_DIR%\bin\unzip.exe -q %TEMP_DIR%/boost_1_53_0.zip
	popd

	REM Currently only the header files are required of boost.
	REM Therefore the following code block is commented out.

REM 
REM 	pushd %L_BOOST_DIR%\boost_1_53_0
REM 	bootstrap.bat
REM 	popd
REM 	
REM 	pushd %L_BOOST_DIR%\boost_1_53_0
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
	popd
)

REM Set Boost-directory as ENV variable (needed for CMake)
pushd %L_BOOST_DIR%\boost_1_53_0
SET BOOST_ROOT=%CD%
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
	
	for %%v in ("libpng", "libjpeg", "lcms2", "exiv2", "libtiff", "libraw", "OpenEXR", "fftw3", "gsl") do (
		mkdir LuminanceHdrStuff\DEPs\include\%%v
		mkdir LuminanceHdrStuff\DEPs\lib\%%v
		mkdir LuminanceHdrStuff\DEPs\bin\%%v
	)
	
	mkdir LuminanceHdrStuff\DEPs\include\libraw\libraw

    
    
	copy OpenEXR-%OPENEXR_COMMIT%\IlmBase.build\output\include\OpenEXR\*.h LuminanceHdrStuff\DEPs\include\OpenEXR
	copy OpenEXR-%OPENEXR_COMMIT%\IlmBase.build\output\lib\*.lib LuminanceHdrStuff\DEPs\lib\OpenEXR
	rem copy OpenExrStuff\Deploy\bin\%Platform%\%Configuration%\*.dll LuminanceHdrStuff\DEPs\bin\OpenEXR

	mkdir LuminanceHdrStuff\DEPs\include\gsl\gsl
	copy gsl-1.15\gsl\*.h LuminanceHdrStuff\DEPs\include\gsl\gsl
	copy gsl-1.15\build.vc10\lib\%Platform%\%Configuration%\*.lib LuminanceHdrStuff\DEPs\lib\gsl
	rem copy gsl-1.15\build.vc10\dll\*.dll LuminanceHdrStuff\DEPs\bin\gsl
)

robocopy fftw-3.3.3-dll LuminanceHdrStuff\DEPs\include\fftw3 *.h /MIR >nul
robocopy fftw-3.3.3-dll LuminanceHdrStuff\DEPs\lib\fftw3 *.lib /MIR /NJS >nul
robocopy fftw-3.3.3-dll LuminanceHdrStuff\DEPs\bin\fftw3 *.dll /MIR /NJS >nul


robocopy tiff-4.0.3\libtiff LuminanceHdrStuff\DEPs\include\libtiff *.h /MIR >nul
robocopy tiff-4.0.3\libtiff LuminanceHdrStuff\DEPs\lib\libtiff *.lib /MIR /NJS >nul
robocopy tiff-4.0.3\libtiff LuminanceHdrStuff\DEPs\bin\libtiff *.dll /MIR /NJS >nul

rem robocopy expat: included indirectly in in exiv2
rem robocopy zlib: included indirectly in in exiv2
robocopy exiv2-%EXIV2_COMMIT%\include LuminanceHdrStuff\DEPs\include\exiv2 *.h *.hpp /MIR >nul
robocopy exiv2-%EXIV2_COMMIT%\bin\%Platform%\Dynamic\%Configuration% LuminanceHdrStuff\DEPs\lib\exiv2 *.lib /MIR >nul
robocopy exiv2-%EXIV2_COMMIT%\bin\%Platform%\Dynamic\%Configuration% LuminanceHdrStuff\DEPs\bin\exiv2 *.dll /MIR >nul

robocopy lpng161 LuminanceHdrStuff\DEPs\include\libpng *.h /MIR >nul
robocopy lpng161\%Configuration% LuminanceHdrStuff\DEPs\lib\libpng *.lib /MIR >nul
robocopy lpng161\%Configuration% LuminanceHdrStuff\DEPs\bin\libpng *.dll /MIR >nul
	

robocopy LibRaw-%LIBRAW_COMMIT%\libraw LuminanceHdrStuff\DEPs\include\libraw\libraw /MIR >nul
robocopy LibRaw-%LIBRAW_COMMIT%\lib LuminanceHdrStuff\DEPs\lib\libraw *.lib /MIR >nul
robocopy LibRaw-%LIBRAW_COMMIT%\bin LuminanceHdrStuff\DEPs\bin\libraw *.dll /MIR >nul
	
robocopy lcms2-%LCMS_COMMIT%\include LuminanceHdrStuff\DEPs\include\lcms2 *.h /MIR >nul
robocopy lcms2-%LCMS_COMMIT%\bin LuminanceHdrStuff\DEPs\lib\lcms2 *.lib /MIR /NJS >nul
robocopy lcms2-%LCMS_COMMIT%\bin LuminanceHdrStuff\DEPs\bin\lcms2 *.dll /MIR /NJS >nul

robocopy libjpeg-turbo-%LIBJPEG_COMMIT% LuminanceHdrStuff\DEPs\include\libjpeg *.h /MIR >nul
robocopy libjpeg-turbo-%LIBJPEG_COMMIT%.build\sharedlib\%Configuration% LuminanceHdrStuff\DEPs\lib\libjpeg *.lib /MIR /NJS >nul
robocopy libjpeg-turbo-%LIBJPEG_COMMIT%.build\sharedlib\%Configuration% LuminanceHdrStuff\DEPs\bin\libjpeg *.dll /MIR /NJS >nul

REM robocopy tbb40_20120613oss\include LuminanceHdrStuff\DEPs\include\tbb /MIR >nul
REM robocopy tbb40_20120613oss\lib\%CpuPlatform%\%VS_SHORT% LuminanceHdrStuff\DEPs\lib\tbb /MIR >nul
REM robocopy tbb40_20120613oss\bin\%CpuPlatform%\%VS_SHORT% LuminanceHdrStuff\DEPs\bin\tbb /MIR >nul


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


set L_CMAKE_INCLUDE=..\DEPs\include\libtiff;..\DEPs\include\libpng;..\..\zlib-%ZLIB_COMMIT%
set L_CMAKE_LIB=..\DEPs\lib\libtiff;..\DEPs\lib\libpng;..\..\zlib-%ZLIB_COMMIT%\%Configuration%
set L_CMAKE_PROGRAM_PATH=%CYGWIN_DIR%\bin
set L_CMAKE_PREFIX_PATH=%QTDIR%
set CMAKE_OPTIONS=%CMAKE_OPTIONS% -DPC_EXIV2_INCLUDEDIR=..\DEPs\include\exiv2 -DPC_EXIV2_LIBDIR=..\DEPs\lib\exiv2 -DCMAKE_INCLUDE_PATH=%L_CMAKE_INCLUDE% -DCMAKE_LIBRARY_PATH=%L_CMAKE_LIB% -DCMAKE_PROGRAM_PATH=%L_CMAKE_PROGRAM_PATH% -DCMAKE_PREFIX_PATH=%L_CMAKE_PREFIX_PATH% -DPNG_NAMES=libpng16

IF EXIST ..\..\gtest-1.6.0 (
	SET GTEST_ROOT=%CD%\..\..\gtest-1.6.0
)

%CMAKE_DIR%\bin\cmake.exe -G "%VS_CMAKE%" ..\qtpfsgui %CMAKE_OPTIONS%
popd

IF EXIST LuminanceHdrStuff\qtpfsgui.build\Luminance HDR.sln (
	pushd LuminanceHdrStuff\qtpfsgui.build	
	rem devenv luminance-hdr.sln /Upgrade
	devenv "Luminance HDR.sln" /build "%ConfigurationLuminance%|%Platform%"
	IF errorlevel 1	goto error_end
	popd
)

IF EXIST LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\luminance-hdr.exe (
	IF EXIST LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% (
		
        robocopy LuminanceHdrStuff\qtpfsgui LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% LICENSE >nul

        IF NOT EXIST LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\align_image_stack.exe (
            copy vcDlls\selected\* LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\
        )
        
        robocopy %TEMP_DIR%\%RawPlatform% LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% align_image_stack.exe >nul
        IF %Platform% EQU Win32 (
            rem robocopy %TEMP_DIR%\%RawPlatform% LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance% huginbase.dll huginvigraimpex.dll msvcp100.dll msvcr100.dll >nul
        )
		
        pushd LuminanceHdrStuff\DEPs\bin
        robocopy libjpeg ..\..\qtpfsgui.build\%ConfigurationLuminance% jpeg8.dll >nul
        robocopy exiv2 ..\..\qtpfsgui.build\%ConfigurationLuminance% exiv2.dll >nul
        robocopy exiv2 ..\..\qtpfsgui.build\%ConfigurationLuminance% expat.dll >nul
        robocopy exiv2 ..\..\qtpfsgui.build\%ConfigurationLuminance% zlib.dll >nul
        robocopy OpenEXR ..\..\qtpfsgui.build\%ConfigurationLuminance% Half.dll >nul
        robocopy OpenEXR ..\..\qtpfsgui.build\%ConfigurationLuminance% Iex.dll >nul
        robocopy OpenEXR ..\..\qtpfsgui.build\%ConfigurationLuminance% IlmImf.dll >nul
        robocopy OpenEXR ..\..\qtpfsgui.build\%ConfigurationLuminance% IlmThread.dll >nul
        robocopy libraw ..\..\qtpfsgui.build\%ConfigurationLuminance% libraw.dll >nul
        robocopy fftw3 ..\..\qtpfsgui.build\%ConfigurationLuminance% libfftw3f-3.dll >nul
        robocopy libpng ..\..\qtpfsgui.build\%ConfigurationLuminance% libpng16.dll >nul
        popd
        
        IF NOT EXIST LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\lcms2.dll (
            pushd LuminanceHdrStuff\DEPs\bin
            for %%v in ("lcms2\lcms2_DLL.dll", ) do (
                copy %%v ..\..\qtpfsgui.build\%ConfigurationLuminance%
            )
            popd
            ren LuminanceHdrStuff\qtpfsgui.build\%ConfigurationLuminance%\lcms2_DLL.dll lcms2.dll
        )

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
        for %%v in ( "QtCore4.dll", "QtGui4.dll", "QtMultimedia4.dll", "QtNetwork4.dll", "QtSql4.dll", "QtWebkit4.dll", "QtXml4.dll", "QtXmlPatterns4.dll") do (
            robocopy %QTDIR%\bin . %%v >nul
        )
        for %%v in ("imageformats", "sqldrivers") do (
            IF NOT EXIST %%v (
                mkdir %%v
            )        
        )
        robocopy %QTDIR%\plugins\imageformats imageformats qjpeg4.dll >nul
        robocopy %QTDIR%\plugins\sqldrivers sqldrivers qsqlite4.dll >nul
		robocopy %QTDIR%\translations i18n qt_??.qm >nul
		robocopy %QTDIR%\translations i18n qt_??_*.qm >nul
        popd
	)
)



goto end

:error_end
pause

:end

endlocal