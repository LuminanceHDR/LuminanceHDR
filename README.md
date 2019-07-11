![Logo](images/luminance.svg)
# Luminance HDR
---------------

[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/LuminanceHDR/LuminanceHDR.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/LuminanceHDR/LuminanceHDR/context:cpp)
[![License](https://img.shields.io/github/license/LuminanceHDR/LuminanceHDR.svg)](https://github.com/LuminanceHDR/LuminanceHDR/blob/master/LICENSE)
[![Issue Stats](https://img.shields.io/github/issues/LuminanceHDR/LuminanceHDR.svg)](https://github.com/LuminanceHDR/LuminanceHDR/issues)
[![Pull Request Stats](https://img.shields.io/github/issues-pr/LuminanceHDR/LuminanceHDR.svg)](https://github.com/LuminanceHDR/LuminanceHDR/pulls)
[![Packaging status](https://repology.org/badge/tiny-repos/luminance-hdr.svg)](https://repology.org/project/luminance-hdr/badges)

Copyright (C) 2010-2019

 - Davide Anastasia <davideanastasia@users.sourceforge.net>
 - Franco Comida <fcomida@users.sourceforge.net>
 - Daniel Kaneider <danielkaneider@users.sourceforge.net>

 and many contributors and translators

Copyright (C) 2006-2010 - Giuseppe Rota <grota@users.sourceforge.net>

- Webpage: http://qtpfsgui.sourceforge.net
- Sourceforge: http://sourceforge.net/projects/qtpfsgui
- Github: https://github.com/LuminanceHDR/LuminanceHDR

What it is
----------
Luminance HDR is a graphical user interface (based on the Qt5 toolkit) that provides a complete workflow for HDR imaging.

Supported HDR formats:
* OpenEXR (extension: exr)
* Radiance RGBE (extension: hdr)
* Tiff formats: 16bit, 32bit (float) and LogLuv (extension: tiff)
* Raw image formats (extension: various)
* PFS native format (extension: pfs)

Supported LDR formats:
* JPEG, PNG, PPM, PBM, TIFF, FITS

Supported features:
* Create an HDR file from a set of images (JPEG, TIFF 8bit and 16bit, RAW)
of the same scene taken at different exposure setting
* Save and load HDR files
* Rotate and resize HDR files
* Tonemap HDR images
* Projective Transformations
* Copy EXIF data between sets of images
* Supports internationalization

Raw image formats are supported - and treated as HDR - thanks to [LibRAW](http://www.libraw.org/).

Make sure you read the "Dependencies" Section in the [INSTALL.md](INSTALL.md) file. If you intend to make a package for a GNU/Linux distribution, please refer to the same file for more information.

The code is in part based on the existing open source packages:
- "pfstools", "pfstmo" and "pfscalibration" by Grzegorz Krawczyk and Rafal Mantiuk
- "qpfstmo", by Nicholas Phillips.

Without their contribution all of this would have not been possible.

Dependencies
------------
Please, refer to the [INSTALL.md](INSTALL.md) file

Compiling the sources
---------------------
Please, refer to the [INSTALL.md](INSTALL.md) file

Contact and Links
-----------------
All comments and suggestions concerning this package or the implementation of particular algorithms are welcome.

For bugs, issues or feature requests, please refer to the [BUGS](BUGS) file

See also:
* [PFStools](http://www.mpii.mpg.de/resources/pfstools/)
* [PFStmo (a tone mapping library)](http://www.mpii.mpg.de/resources/tmo/)
* [PFScalibration](http://www.mpii.mpg.de/resources/hdr/calibration/pfs.html)
