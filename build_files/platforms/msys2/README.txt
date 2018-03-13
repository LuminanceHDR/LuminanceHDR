Begin by installing and updating MSYS2 using the instructions from the MSYS2 website http://www.msys2.org/

After installing MSYS2 execute the following commands in msys2 shell:

pacman -S tar gzip nano make diffutils intltool git
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb mingw-w64-x86_64-make mingw-w64-x86_64-pkg-config mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-lcms2 mingw-w64-x86_64-fftw mingw-w64-x86_64-qtwebkit mingw-w64-x86_64-libraw mingw-w64-x86_64-boost mingw-w64-x86_64-exiv2 mingw-w64-x86_64-openexr mingw-w64-x86_64-gsl mingw-w64-x86_64-cfitsio

git clone https://github.com/LuminanceHDR/LuminanceHDR.git /c/code/lhdr
cd /c/code/lhdr
mkdir build
cd build

Currently there is a version conflict in msys2 version of Qt5WebKit.
To fix (hack) it, open the file msys64\mingw64\lib\cmake\Qt5WebKit\Qt5WebKitConfig.cmake
Search for 
find_dependency(Qt5Network 5.10.0 EXACT)
and change to
find_dependency(Qt5Network 5.10.1 EXACT)

run the following commands in msys2 shell

cmake -G "MSYS Makefiles" ..
make

Now you should have a luminance-hdr.exe in your build folder.
try running it using
./luminance-hdr

It still has some missing icons.

make install does not work currently because it wants to installs to C:/Program Files (x86)/Luminance HDR which is not desired for a 64bit executable
I will see how to fix that later
