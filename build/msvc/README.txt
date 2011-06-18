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
* MS Visual Studio 2010 or higher
* Qt for MSVC installed
  - official 32 bit Qt from: http://qt.nokia.com/downloads/downloads#qt-lib
  - working 64 bit and 32 bit Qt from: http://code.google.com/p/qt-msvc-installer/
* CMake
* Cygwin installed with: cvs, gzip, svn, tar, unzip, wget


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

In 'LuminanceHdrStuff\qtpfsgui' you find the LuminanceHdr SVN directory,
where the compiled .exe file should be under 
'LuminanceHdrStuff\qtpfsgui\Release'


