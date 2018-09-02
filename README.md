This is a project that aims to make some of RawTherapee's highly optimized raw processing routines readily available for other FOSS photo editing software.

The goal is for certain files from RawTherapee to be drop-in compatible with this library. Thus, any changes necessary for use in this project must also be made upstream in RawTherapee.

This is version 0.2.0, which furnishes the following routines:

* amaze_demosaic
* CA_correct

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


Include `-lrtprocess`, and `#include <rtprocess/librtprocess.h>` to use this library.
