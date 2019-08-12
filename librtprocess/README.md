# librtprocess

This is a project that aims to make some of RawTherapee's highly optimized raw processing routines readily available for other FOSS photo editing software.

The goal is to move certain source files from RawTherapee into this library.
Thus, any changes to the source can be done here and will be used by the projects which use librtprocess.

librtprocess currently is maintained by developers of the following projects:

. Filmulator https://github.com/CarVac/filmulator-gui

. HDRMerge https://github.com/jcelaya/hdrmerge

. LuminanceHdr https://github.com/LuminanceHDR/LuminanceHDR

. PhotoFlow https://github.com/aferrero2707/PhotoFlow

. rawproc https://github.com/butcherg/rawproc

. RawTherapee https://github.com/Beep6581/RawTherapee

... the latter is where currently all the code comes from ;-)

This is version 0.11.0, which furnishes the following routines:

* ahd_demosaic
* amaze_demosaic
* bayerfast_demosaic
* dcb_demosaic
* hphd_demosaic
* igv_demosaic
* lmmse_demosaic
* rcd_demosaic
* vng4_demosaic
* markesteijn_demosaic
* xtransfast_demosaic
* CA_correct
* HLRecovery_inpaint

## Build instructions:

1. Make a subdirectory named `build`, and `cd` to that directory.
2. Run `cmake -DCMAKE_BUILD_TYPE="Release" ..`
3. Run `make`
4. Run `make install` as root.

Build instructions for Windows msys2 environment:

1. Make a subdirectory named `build`, and `cd` to that directory.
2. Run `cmake -G "MSYS Makefiles" -DCMAKE_INSTALL_PREFIX="$MSYSTEM_PREFIX" -DCMAKE_BUILD_TYPE="Release" ..`
3. Run `make`
4. Run `make install`.

Build instructions for macOS:

Prerequisites: XCode/XCode command line tools.  An optional SDK (this example uses macOS 10.9).  An implementation of OpenMP, for example `libiomp.5`.
1. Make a subdirectory named `build`, and `cd` to that directory.
2. On macOS 10.12 _Sierra_, run `sudo cmake -DCMAKE_BUILD_TYPE="release"  -DPROC_TARGET_NUMBER="1" -DCMAKE_C_COMPILER="clang-mp-3.9" -DCMAKE_CXX_COMPILER="clang++-mp-3.9" -DCMAKE_CXX_FLAGS=-I/opt/local/include -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.9" -DOpenMP_C_FLAGS=-fopenmp="libiomp5" -DOpenMP_CXX_FLAGS=-fopenmp="libiomp5" -DOpenMP_C_LIB_NAMES="libiomp5" -DOpenMP_CXX_LIB_NAMES="libiomp5" -DOpenMP_libiomp5_LIBRARY="/opt/local" -DCMAKE_INSTALL_PREFIX=/opt/local ..`
<br><br>On macOS 10.14 _Mojave_, run `cmake -DCMAKE_BUILD_TYPE="release" -DPROC_TARGET_NUMBER="1" -DCMAKE_CXX_COMPILER="clang++" -DCMAKE_CXX_FLAGS=-I/opt/local/include -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.9"  -DOpenMP_CXX_FLAGS=-fopenmp=lomp -DOpenMP_CXX_LIB_NAMES="libomp" -DOpenMP_CXX_FLAGS="-Xpreprocessor -fopenmp /opt/local/lib/libomp.dylib -I/opt/local/include" -DOpenMP_CXX_LIB_NAMES="libomp" -DOpenMP_libomp_LIBRARY=/opt/local/lib/libomp.dylib -DCMAKE_INSTALL_PREFIX=/opt/local  -DCMAKE_SHARED_LINKER_FLAGS=-L/opt/local/lib ..`
3. Run `sudo make -j$(sysctl -n hw.ncpu) install`

Optional switches to be included in the `cmake` command:

1. To build in verbose mode, include `-DVERBOSE=ON`
2. If you make your own builds, include `-DPROC_TARGET_NUMBER=2` for maximum speed. Keep in mind that this build will only work on the machine you built it.

## Using librtprocess:

Include `-lrtprocess`, and `#include <rtprocess/librtprocess.h>` to use this library.

### Demosaic

The demosaic routines expect raw data in the form 1) single-channel, 2) float, 3) range 0.0 - 65535.0.  This roughly
corresponds to what the raw libraries deliver, e.g. Libraw's mosaic is single-channel unsigned short 0-65535, except 
for the float number format, so at least a unsigned short -> float cast is probably required.  

The raw data array expected by the demosaic routines is float**, which is an array of pointers to pointers. This 
hierarchical pointer arrangement is "row-major", that is, the first array of pointers point to pointers that point to a contiguous block of memory containing the pixel data for a row.  This choice of storage is for fast performance, as 
individual pixels can be accessed with pointer dereferencing, but at the expense of rather convoluted memory management
in C.

The demosaic routines' output are three separate float** arrays, one for each channel.  This output organization helps
certain applications that like to start with the separate channels; for 'regular' use, the red, green, and blue floats
have to be loaded separately into each channel of the destination RGB struct or array.

Here's a code segment that demonstrates the marshalling/demarshalling of data, taken from the rawproc application. Note:
rawproc's internal image is a row-major contiguous array of RGB floats in the range 0.0 - 1.0, so the code includes
the logic to convert from/to this structure.

```
    //build the input and output data structures. 'w' and 'h' are the image width and height.
		float **rawdata = (float **)malloc(h * sizeof(float *));
		rawdata[0] = (float *)malloc(w*h * sizeof(float));
		for (unsigned i=1; i<h; i++) 
			rawdata[i] = rawdata[i - 1] + w; 

		float **red     = (float **)malloc(h * sizeof(float *)); 
		red[0] = (float *)malloc(w*h * sizeof(float));
		for (unsigned i=1; i<h; i++) 
			red[i]     = red[i - 1] + w;

		float **green     = (float **)malloc(h * sizeof(float *)); 
		green[0] = (float *)malloc(w*h * sizeof(float));
		for (unsigned i=1; i<h; i++) 
			green[i]     = green[i - 1] + w;

		float **blue     = (float **)malloc(h * sizeof(float *)); 
		blue[0] = (float *)malloc(w*h * sizeof(float));
		for (unsigned i=1; i<h; i++) 
			blue[i]     = blue[i - 1] + w;

    //loads the internal data to the librtprocess rawData structure.  The interal data's red channel 
    //is arbitrarily chosen as a monochrome image is represented R=G=B:
		#pragma omp parallel for num_threads(threadcount)
		for (unsigned y=0; y<h; y++) {
			for (unsigned x=0; x<w; x++) {
				unsigned pos = x + y*w;
				rawdata[y][x] = image[pos].r * 65535.f;
			}
		}
	
		vng4_demosaic (w, h, rawdata, red, green, blue, cfarray, f);

    //assemble the demosaiced RGB array from the individual channels:
		#pragma omp parallel for num_threads(threadcount)	
		for (unsigned y=0; y<h; y++) {
			for (unsigned x=0; x<w; x++) {
				unsigned pos = x + y*w;
				image[pos].r = red[y][x] /65535.f;
				image[pos].g = green[y][x] /65535.f;
				image[pos].b = blue[y][x] /65535.f;
			}
		}
		
		free (blue[0]);
		free( blue );
		free (green[0]);
		free( green );
		free (red[0]);
		free( red );
		free (rawdata[0]);
		free( rawdata );
```

The Bayer demosaic routines also take in a `cfarray` parameter that is a 2x2 array corresponding to the indexing of the color filters corresponding to the top left corner of the raw image plane. The `xtrans` parameter for the X-Trans routines is similar but has dimensions of 6 by 6. For these, `0` corresponds to red, `1` corresponds to green channel one, `2` corresponds to blue, and `3` corresponds to green channel two. Some algorithms require both greens, others only one.

### Highlight Recovery

The highlight recovery algorithm uses inpainting to reconstruct clipped highlights when not all channels are clipped. The input data should be full RGB for each pixel, in the raw color space, with the white balance multipliers already applied to it. `chmax` is simply the maximum pixel value in each of the three color channels. `clmax` is the raw clip point for each channel; that is, the whitepoint minus the blackpoint for each channel, multiplied by the white balance multipliers.
