This is the README file for Luminance
-------------------------------------------------------------------
Webpage:       http://qtpfsgui.sourceforge.net
Sourceforge:   http://sourceforge.net/projects/qtpfsgui

Contents
---------
1. Requirements
2. Compilation
3. Output


1. Requirements
---------------------------------------------------------------------
This build process works for 32 and 64 bit compilation. 

This build process requires:
* Windows
* MS Visual Studio 2010 or higher, ExpressEditions should work (currently testing with VS2012)
* Qt for MSVC installed
  - either official 32 bit Qt from: http://qt-project.org/downloads
  - or Qt self compiled
  ATTENTION: Do not mix the compilation with different versions of MSVC. If you 
             compile Luminance with VC2012 you should also have Qt compiled
             with VC2010. If you don't find any precompiled Qt with a 
             specific MSVC or platform version, you can just download the 
             latest version of a precompiled Qt with any VC compiler (or just the Qt source version), 
             open the VC command prompt in the Qt main directory and then:
             
             First open this solution and build all projects for your configuration (ex. Release/x64)
             > icu\source\allinone\allinone.sln
             
             And then run: (change your paths to match the installation
             > set include=%include%;C:\Data\Develop\Qt\icu\include
             > set lib=%lib%;C:\Data\Develop\Qt\icu\lib64
             (Change lib64 to lib for the x32 build)
             
             > set path=%path%;C:\Data\Programs\ruby-1.9.3-p385-i386-mingw32\bin
             > configure -release -opengl desktop -no-compile-examples -mp -nomake tests -nomake examples -no-sse3 -no-ssse3 -no-sse4.1 -no-sse4.2 -no-avx -no-avx2
             > nmake
             > cd qtwebkit
             > nmake
             > cd ..
             > nmake install_subtargets
             > nmake -f Makefile confclean
             > del /S /Q *.obj
             This recompiles the whole Qt and takes from 30 minutes to several hours...
             This reconfiguration can also be applied for generating x64 Qt versions!
  ATTENTION: If you move the Qt folder AFTER the compilation you have to redo the 
             > configure ... step
             (without doing the actual compilation with nmake)
             Moreover you need to manually adapt the paths in qtbase\lib\cmake\Qt5LinguistTools\Qt5LinguistToolsConfig.cmake

             Afterwards copy the icu-dlls into the qtbase/bin folder, along with the platforms folder from the plugins directory.
             Don't forget the d3dcompiler_46.dll from the local VS-folder!
             At the end programs like linguist.exe, designer.exe must be able to run!!!
			 
             
* CMake
* Cygwin installed with: cvs, git, gzip, sed, ssh, svn, tar, unzip, wget


2. Compilation
---------------------------------------------------------------------
Copy the .cmd files into a new EMPTY folder. Open the Visual Studio 
command prompt (x64 or Win32) and change (cd) to that directory. If 
there are some problems the batch script should tell you!

Open the setenv.cmd file and adjust the variables paths and options for your needs.

Start the compilation with 
>	build.cmd

Wait, wait, wait....


3. Output
---------------------------------------------------------------------
The build process should download and build all the required libraries.

In  'LuminanceHdrStuff\DEPs' you should find all the compilated DLLs,
librarys and include header files.

In  'LuminanceHdrStuff\qtpfsgui.build' you should find all the binary
related files.

In 'LuminanceHdrStuff\qtpfsgui' you find the LuminanceHdr SVN directory.


