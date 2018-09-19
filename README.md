This is a project that aims to make some of RawTherapee's highly optimized raw processing routines readily available for other FOSS photo editing software.

The goal is to move certain source files from RawTherapee into this library.
Thus, any changes to the source can be done here and will be used by the projects which use librtprocess.

librtprocess currently is maintained by developers of the following projects:

Filmulator https://github.com/CarVac/filmulator-gui
HdrMerge https://github.com/jcelaya/hdrmerge
LumincanceHdr https://github.com/LuminanceHDR/LuminanceHDR
PhotoFlow https://github.com/aferrero2707/PhotoFlow
RawTherapee https://github.com/Beep6581/RawTherapee

... the latter is where currently all the code comes from ;-)

This is version 0.3.0, which furnishes the following routines:

* amaze_demosaic
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

Optional switches to be included in the `cmake` command:

1. To build in verbose mode, include `-DVERBOSE=ON`
2. If you make your own builds, include `-DPROC_TARGET_NUMBER=2` for maximum speed. Keep in mind that this build will only work on the machine you built it.

Include `-lrtprocess`, and `#include <rtprocess/librtprocess.h>` to use this library.
