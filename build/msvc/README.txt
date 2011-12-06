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
* MS Visual Studio 2008 or higher
* Qt for MSVC installed
  - official 32 bit Qt from: http://qt.nokia.com/downloads/downloads#qt-lib
  - working 64 bit and 32 bit Qt from: http://code.google.com/p/qt-msvc-installer/
  ATTENTION: Do not mix the compilation with different versions of MSVC. If you 
             compile Luminance with VC2010 you should also have Qt compiled
             with VC2010. If you don't find any precompiled Qt with a 
             specific MSVC or platform version, you can just download the 
             latest version of a precompiled Qt with any VC compiler (or just the Qt source version), 
             open the VC command prompt in the Qt main directory and then run:
             > configure.exe -mp -fast
             > nmake
             > nmake confclean
             > del /S /Q *.obj lib*.dll
             This recompiles the whole Qt and takes from 30 minutes to several hours...
             This reconfiguration can also be applied for generating x64 Qt versions!
  ATTENTION: If you move the Qt folder AFTER the compilation you have to do the 
             > configure.exe -mp -fast
             (without doing the actual compilation with nmake)
			 
             
* CMake
* Cygwin installed with: cvs, git, gzip, sed, ssh, svn, tar, unzip, wget


2. Compilation
---------------------------------------------------------------------
Copy the .cmd files into a new EMPTY folder. Open the Visual Studio 
command prompt (x64 or Win32) and change (cd) to that directory. If 
there are some problems the batch script should tell you!

Open the setenv.cmd file and adjust the variables paths for your needs.

Start the compilation with 
>	getDependencies.cmd

Wait, wait, wait....


3. Output
---------------------------------------------------------------------
The build process should download and build all the required libraries.

In  'LuminanceHdrStuff\DEPs' you should find all the compilated DLLs,
librarys and include header files.

In  'LuminanceHdrStuff\qtpfsgui.build' you should find all the binary
related files.

In 'LuminanceHdrStuff\qtpfsgui' you find the LuminanceHdr SVN directory.


