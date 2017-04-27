
#http://dev.exiv2.org/projects/exiv2/repository/
SET(EXIV2_VERSION 0.25)
SET(EXIV2_URI www.exiv2.org/exiv2-${EXIV2_VERSION}.tar.gz)
set(EXIV2_HASH 4753)

#http://github.com/libjpeg-turbo/libjpeg-turbo
SET(LIBJPEG-TURBO_VERSION 1.5.1)
SET(LIBJPEG-TURBO_URI https://sourceforge.net/projects/libjpeg-turbo/files/${LIBJPEG-TURBO_VERSION}/libjpeg-turbo-${LIBJPEG-TURBO_VERSION}.tar.gz/download)
set(LIBJPEG-TURBO_HASH da2a27ef056a0179cbd80f9146e58b89403d9933)

#https://github.com/madler/zlib/commits
SET(ZLIB_version 1.2.11)
set(ZLIB_URI https://netcologne.dl.sourceforge.net/project/libpng/zlib/${ZLIB_VERSION}/zlib-${ZLIB_VERSION}.tar.gz)
set(ZLIB_HASH cacf7f1d4e3d44d871b605da3b647f07d718623f)

#https://github.com/openexr/openexr
set(ILMBASE_VERSION 2.2.0)
set(ILMBASE_URI http://download.savannah.nongnu.org/releases/openexr/ilmbase-${ILMBASE_VERSION}.tar.gz)
set(ILMBASE_HASH 20d043d017d4b752356bb76946ffdffaa9c15c72)

#http://www.boost.org/
set(BOOST_VERSION 1.63.0)
set(BOOST_VERSION_NODOTS 1_63_0)
set(BOOST_URI http://sourceforge.net/projects/boost/files/boost/${BOOST_VERSION}/boost_${BOOST_VERSION_NODOTS}.tar.bz2/download)
set(BOOST_MD5 65a840e1a0b13a558ff19eeb2c4f0cbe)

#ftp://ftp.fftw.org/pub/fftw/
set(FFTW_VERSION 3.3.4)
set(FFTW_URI http://www.fftw.org/fftw-${FFTW_VERSION}.tar.gz)
set(FFTW_HASH f8048af3e30cb3f65befd0aa2f3d16de3eeb5583)

#https://github.com/mm2/Little-CMS
set(LCMS_VERSION 2.8)
set(LCMS_URI https://github.com/mm2/Little-CMS/archive/lcms${LCMS_VERSION}.zip)
set(LCMS_HASH f8048af3e30cb3f65befd0aa2f3d16de3eeb5583)

#https://github.com/ampl/gsl
set(GSL_VERSION 2.2.1)
set(GSL_URI https://github.com/ampl/gsl/archive/v${GSL_VERSION}.zip)
set(GSL_HASH bdb1bb249094975822c034694425dba2014b4b18)

#https://github.com/LibRaw/LibRaw
SET LIBRAW_COMMIT_LONG=d7c3d2cb460be10a3ea7b32e9443a83c243b2251
SET LIBRAW_DEMOS2_COMMIT_LONG=194f592e205990ea8fce72b6c571c14350aca716
SET LIBRAW_DEMOS3_COMMIT_LONG=f0895891fdaa775255af02275fce426a5bf5c9fc

#ftp://sourceware.org/pub/pthreads-win32/
set(PTHREADS_VERSION 2-9-1)
set(PTHREADS_URI ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-${PTHREADS_VERSION}-release.tar.gz)
set(PTHREADS_SHA512 9c06e85310766834370c3dceb83faafd397da18a32411ca7645c8eb6b9495fea54ca2872f4a3e8d83cb5fdc5dea7f3f0464be5bb9af3222a6534574a184bd551)

#http://heasarc.gsfc.nasa.gov/FTP/software/fitsio/c
#broken 3370
set(FITSIO_VERSION 3.360)
set(FITSIO_VERSION_NODOTS 3360)
set(FITSIO_URI heasarc.gsfc.nasa.gov/FTP/software/fitsio/c/cfit${FITSIO_VERSION_NODOTS}.zip)
set(FITSIO_HASH 3360)

#Unused?
#REM Internal version number for  http://qtpfsgui.sourceforge.net/win/hugin-*
#SET HUGIN_VER=201600

#http://download.osgeo.org/libtiff/
set(TIFF_VERSION 4.0.7)
set(TIFF_URI http://download.osgeo.org/libtiff/tiff-${TIFF_VERSION}.tar.gz)
set(TIFF_HASH b28076b056eba9d665881bab139d21b21137fd2d)
