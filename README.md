This is a project that aims to make some of RawTherapee's highly optimized raw processing routines readily available for other FOSS photo editing software.

The goal is to move certain source files from RawTherapee into this library.
Thus, any changes to the source can be done here and will be used by the projects which use librtprocess.

librtprocess currently is maintained by developers of the following projects:

. Filmulator https://github.com/CarVac/filmulator-gui

. HDRMerge https://github.com/jcelaya/hdrmerge

. LuminanceHdr https://github.com/LuminanceHDR/LuminanceHDR

. PhotoFlow https://github.com/aferrero2707/PhotoFlow

. RawTherapee https://github.com/Beep6581/RawTherapee

... the latter is where currently all the code comes from ;-)

This is version 0.5.0, which furnishes the following routines:


* amaze_demosaic
* dcb_demosaic
* igv_demosaic
* lmmse_demosaic
* rcd_demosaic
* vng4_demosaic
* markesteijn_demosaic
* xtransfast_demosaic
* CA_correct
* gaussianBlur

Build instructions:

1. Make a subdirectory named `build`, and `cd` to that directory.
2. Run `cmake ..`
3. Run `make`
4. Run `make install` as root.

Build instructions for Windows msys2 environment:

1. Make a subdirectory named `build`, and `cd` to that directory.
2. Run `cmake -G "MSYS Makefiles" -DCMAKE_INSTALL_PREFIX="$MSYSTEM_PREFIX" ..`
3. Run `make`
4. Run `make install`.

Build instructions for macOS:

Prerequisites: XCode/XCode command line tools.  An optional SDK (this example uses macOS 10.9).  An implementation of OpenMP, for example `libiomp.5`.
1. Make a subdirectory named `build`, and `cd` to that directory.
2. On macOS 10.12 _Sierra_, run `sudo cmake -DCMAKE_BUILD_TYPE="release"  -DPROC_TARGET_NUMBER="1" -DCMAKE_C_COMPILER="clang-mp-3.9" -DCMAKE_CXX_COMPILER="clang++-mp-3.9" -DCMAKE_CXX_FLAGS=-I/opt/local/include -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.9" -DOpenMP_C_FLAGS=-fopenmp="libiomp5" -DOpenMP_CXX_FLAGS=-fopenmp="libiomp5" -DOpenMP_C_LIB_NAMES="libiomp5" -DOpenMP_CXX_LIB_NAMES="libiomp5" -DOpenMP_libiomp5_LIBRARY="/opt/local" -DCMAKE_INSTALL_PREFIX=/opt/local ..`
<br><br>On macOS 10.14 _Mojave_, run `cmake -DCMAKE_BUILD_TYPE="release" -DPROC_TARGET_NUMBER="1" -DCMAKE_C_COMPILER="clang" -DCMAKE_CXX_COMPILER="clang++" -DCMAKE_CXX_FLAGS=-I/opt/local/include -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.9" -DOpenMP_C_FLAGS=-fopenmp=lomp -DOpenMP_CXX_FLAGS=-fopenmp=lomp -DOpenMP_C_LIB_NAMES="libiomp5" -DOpenMP_CXX_LIB_NAMES="libiomp5" -DOpenMP_libiomp5_LIBRARY="/opt/local/lib/libiomp5.dylib" -DOpenMP_CXX_FLAGS="-Xpreprocessor -fopenmp /opt/local/lib/libiomp5.dylib -I/opt/local/include" -DOpenMP_CXX_LIB_NAMES="libiomp5" -DOpenMP_omp_LIBRARY=/opt/local/lib/libiomp5.dylib -DOpenMP_C_FLAGS="-Xpreprocessor -fopenmp /opt/local/lib/libiomp5.dylib -I/opt/local/include"  -DCMAKE_INSTALL_PREFIX=/opt/local -DCMAKE_C_FLAGS=I/opt/local/include ..`
3. Run `sudo make -j$(sysctl -n hw.ncpu) install`

Optional switches to be included in the `cmake` command:

1. To build in verbose mode, include `-DVERBOSE=ON`
2. If you make your own builds, include `-DPROC_TARGET_NUMBER=2` for maximum speed. Keep in mind that this build will only work on the machine you built it.

Include `-lrtprocess`, and `#include <rtprocess/librtprocess.h>` to use this library.
