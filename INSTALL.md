# INSTALLATION

## Table of Contents

1. [Introduction](#intro)
2. [Install Prebuilt Binaries](#bin)
3. [Dependencies](#dep)
4. [Compilation](#compilation)
    1. [Obtain the Source Code](#clone)
    2. [Configure CMake](#cmake)
    3. [Compile](#compile)
    4. [Update](#update)
5. [Platform Notes](#pnotes)
    1. [Windows](#pnote_win)
    2. [macOS](#pnote_macos)

## Introduction <a name="intro"></a>

This document covers the compilation and installation of Luminance HDR.

- Copyrights
    - (C) 2006-2010 Giuseppe Rota
    <grota@users.sourceforge.net>
    - (C) 2010-2017 Davide Anastasia
    <davideanastasia@users.sourceforge.net>
    - (C) Franco Comida
    <fcomida@users.sourceforge.net>
    - (C) Daniel Kaneider
    <danielkaneider@users.sourceforge.net>
    - (C) 2019 Maciej Dworak
    <https://github.com/Beep6581>
- Project homepage:
    http://qtpfsgui.sourceforge.net/
- Source code and issue tracker:
    https://github.com/LuminanceHDR/LuminanceHDR
- Forum:
    https://discuss.pixls.us/c/software/luminancehdr

## Install Prebuilt Binaries <a name="bin"></a>

Luminance HDR is readily available on all major platforms.

- Get a [Windows installer](https://sourceforge.net/projects/qtpfsgui/files/luminance/)
- Linux
    - Check your package manager.
    - Get an [AppImage](https://github.com/aferrero2707/lhdr-appimage/releases)
    - Get a [FlatPak](https://flathub.org/apps/details/net.sourceforge.qtpfsgui.LuminanceHDR)
- Get a [macOS DMG](https://sourceforge.net/projects/qtpfsgui/files/luminance/)

The rest of this document concerns compiling Luminance HDR yourself.

## Dependencies <a name="dep"></a>

To compile Luminance HDR your system will need a set of tools and code libraries called "dependencies". The following is a list of dependencies needed to compile the latest version of Luminance HDR:

- [Qt6](https://www.qt.io/), the widget toolkit used by the graphical user interface (GUI).
- [Exiv2](https://www.exiv2.org/), used to read and write image metadata (Exif, IPTC, XMP).
- [Little CMS](http://www.littlecms.com/), LCMS2 is used for color management.
- [libjpeg-turbo](https://libjpeg-turbo.org/) (or libjpeg), used to read and write JPEG files.
- [LibTIFF](http://www.libtiff.org/), used to read and write TIFF files.
- [libpng](http://www.libpng.org/pub/png/libpng.html), used to read and write PNG files.
- [LibRaw](https://www.libraw.org/), used to read raw files.
- [OpenEXR](http://www.openexr.com/), used to read and write high dynamic range EXR files. Some distributions refer to the package as `ilmbase`.
- [CFITSIO](https://heasarc.gsfc.nasa.gov/fitsio/), an optional library for reading and writing FITS files, commonly used by the astrophotographer community.
- [FFTW](www.fftw.org), used for computing discrete Fourier transforms. Luminance HDR requires the single-precision "float" version of FFTW3, usually called `fftw3f` or `fftw-3-single` on MacPorts.
- [Boost](https://www.boost.org/), a set of C++ support libraries.
- [GNU Scientific Library](https://www.gnu.org/software/gsl/), GSL is used by the Mantiuk08 tone mapping operator.
- [Eigen3](http://eigen.tuxfamily.org/), a C++ template library required by the Lischinski tone mapping operator.

## Compilation <a name="compilation"></a>

Compilation involves obtaining the source code, configuring the build process using CMake, and finally performing the actual compilation.

In order to keep your file system clean and to isolate self-compiled programs from those installed using your system's package manager, we will be cloning the source code into `~/programs/code-lhdr`, performing an out-of-source build in `~/programs/code-lhdr/build` and installing the compiled program into `~/programs/lhdr`. To this end:

```bash
mkdir ~/programs
cd ~/programs
```


### 1. Obtain the Source Code <a name="clone"></a>

First, you need to clone Luminance HDR's source code repository. Bring up your console and run this:

```bash
git clone https://github.com/LuminanceHDR/LuminanceHDR.git code-lhdr
cd code-lhdr
```

### 2. Configure CMake <a name="cmake"></a>

In order to keep your source code folder clean, the build will be created in a folder called `build`. If you just cloned the source code, you will need to create this folder:

```bash
mkdir build
cd build
```

CMake allows you to configure the build process. The `-D` option allows you to customize settings for the project. Multiple settings can be specified - each must be prefixed with `-D`. Multiple options must be separated by whitespace. Setting values must be enclosed in parentheses. The last argument must point to the source code (as you are in the `build` sub-folder, you can point to the source code using double dots `..`). Refer to the example at the end of this section.

The most significant options follow:
- `CMAKE_BUILD_TYPE`
    Values:`Debug`, `Release`, `RelWithDebInfo` and `MinSizeRel`.
    This controls whether the build will favor faster execution times, more verbose debugging output, or a smaller executable. The `Debug` and `RelWithDebInfo` builds will let you get a useful stack-backtrace if Luminance HDR crashes while running through GDB - you can then submit the backtrace to us so that we can find the problem and fix it. The `Debug` build is the slowest but generates the most detailed information. The `RelWithDebInfo` build is as fast as a `Release` build and generates often sufficient information, though not as detailed as a `Debug` build. The `Release` build provides very little useful information when it crashes, but does contain many speed optimizations resulting in a program that works several times faster than the `Debug` build would.
    For normal use, make a `Release` build. If you find a reproducible bug, then make a `Debug` build and send us a stack-backtrace (or fix it yourself and send us the patch).
- `CMAKE_INSTALL_PREFIX`
    Points to a folder into which the compiled program will be installed. Defaults to `/usr/local` in Linux/macOS and `c:/Program Files/Luminance HDR` in Windows.
- `UPDATE_TRANSLATIONS`
    Values:`OFF` (default), `ON`.
    Luminance HDR ships with translations files for localization. These are stored inside the `i18n` folder. Should you wish to update the translation `.ts` files then set this option to `ON`, compile, then set it back to `OFF`.
- `ENABLE_UNIT_TEST`
    Values:`OFF` (default), `ON`.
    Enables unit testing. Requires Google's gtest framework. The resulting test executables are placed in the `test` sub-folder.

Your final CMake command (split into multiple lines for readability) should look something like this:

```bash
cmake \
    -DCMAKE_BUILD_TYPE="Release"  \
    -DCMAKE_INSTALL_PREFIX="$HOME/programs/lhdr" \
    ..
```

### 3. Compile <a name="compile"></a>

Find out how many processing units are available:

```bash
nproc
```

On a typical modern machine with simultaneous multithreading, the number of processing units would be twice the number of actual physical cores. Pass this number as the value for the `--jobs` parameter below. This influences only the compilation speed, it has no influence over how fast the compiled build runs. As an example, a typical dual-core CPU would return "4" processing units.

To start compiling, run:

```bash
make install --jobs 4
```

Compilation will take a few minutes. When completed successfully, you will see that it had installed the built files.

To run your self-compiled build of Luminance HDR, assuming you used the settings as laid out in this document, type:

```bash
~/programs/lhdr/bin/luminance-hdr
```

### 4. Update <a name="update"></a>

Every time you want to update Luminance HDR to the latest code available, just run the following:

```bash
cd ~/programs/code-lhdr
git pull
cd build
```

Then repeat the compilation step above.

You can safely delete the source code folder `~/programs/code-lhdr` if you so wish. The compiled program in `~/programs/lhdr` will still work, but then you will have to redo all the steps if you want to update. Rather, leave the repository intact so that you can just update in a week or a month's time without redoing all the steps.

## Platform Notes <a name="pnotes"></a>

### Windows <a name="pnote_win"></a>

Currently, Luminance HDR can be compiled using Microsoft Visual Studio. Either use the precompiled libraries available at https://sourceforge.net/projects/qtpfsgui/files/DEPs/ , or self-compile the dependencies using the provided script, as described in `/build_files/platforms/msvc/README.txt`

It is also possible to build Luminance HDR using MSYS2/MinGW, see `build_files/platforms/msys2/README.txt`

### macOS <a name="pnote_macos"></a>

Apple Silicon builds use AppleClang, Qt 6, Homebrew's native arm64
dependencies, and Hugin's `align_image_stack` helper. Install the build tools
and libraries with native arm64 Homebrew:

```bash
brew install cmake ninja qtbase qtsvg qttools qttranslations qtwebengine \
    boost exiv2 libraw fftw little-cms2 eigen libtiff libpng openexr \
    gsl jpeg-turbo cfitsio libomp libpano wxwidgets glew sqlite zlib
```

Build the pinned Hugin 2025.0.1 helper from source, then configure Luminance HDR
for arm64:

```bash
mkdir -p build-arm64-tools
build_files/platforms/macosx/build_align_image_stack.sh \
    build-arm64-tools/align_image_stack

cmake -S . -B build-arm64 -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
    -DCMAKE_PREFIX_PATH="$(brew --prefix)" \
    -DLHDR_ALIGN_IMAGE_STACK="$PWD/build-arm64-tools/align_image_stack"
cmake --build build-arm64 --parallel
```

`LHDR_ALIGN_IMAGE_STACK` may be omitted for a development build. In that case,
Luminance HDR looks for `align_image_stack` on `PATH` at runtime. Release
bundles should always provide a native helper.

Package and verify a self-contained app from a fresh build bundle:

```bash
source_app=$(find build-arm64 -maxdepth 1 -type d -name 'Luminance HDR*.app' -print -quit)
build_files/platforms/macosx/package_app.sh \
    "$source_app" "dist/$(basename "${source_app%.app}")-arm64.app" arm64
```

The packaging script deliberately refuses an existing output and runs
`macdeployqt` exactly once. It then verifies that every bundled Mach-O file is
arm64, no Homebrew or user-directory dependency remains, the signatures are
valid, and both command-line tools start. The default signature is ad hoc for
local testing. Set `LHDR_CODESIGN_IDENTITY` to use a Developer ID identity; an
official distributable must additionally use the project's signing,
notarization, and release credentials.

Set `LHDR_CREATE_DMG=1` to create a compressed DMG next to the packaged app.
Build release artifacts on the oldest supported macOS runner rather than a
newer developer machine: Homebrew bottles can inherit the minimum OS version
of the host on which they were built.
