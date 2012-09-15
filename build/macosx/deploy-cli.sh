#!/bin/bash

cp luminance-hdr-cli luminance-hdr.app/Contents/MacOS/

install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change QtWebKit.framework/Versions/4/QtWebKit @executable_path/../Frameworks/QtWebKit.framework/Versions/4/QtWebKit luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change QtSql.framework/Versions/4/QtSql @executable_path/../Frameworks/QtSql.framework/Versions/4/QtSql luminance-hdr.app/Contents/MacOS/luminance-hdr-cli

install_name_tool -change /opt/local/lib/libIlmImf.6.dylib @executable_path/../Frameworks/libIlmImf.6.dylib luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change /opt/local/lib/libHalf.6.dylib @executable_path/../Frameworks/libHalf.6.dylib luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change /opt/local/lib/libIex.6.dylib @executable_path/../Frameworks/libIex.6.dylib luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change /opt/local/lib/libtiff.3.dylib @executable_path/../Frameworks/libtiff.3.dylib luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change /usr/local/lib/libraw_r.5.dylib @executable_path/../Frameworks/libraw_r.5.dylib luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change /opt/local/lib/libfftw3f.3.dylib @executable_path/../Frameworks/libfftw3f.3.dylib luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change /opt/local/lib/libfftw3f_threads.3.dylib @executable_path/../Frameworks/libfftw3f_threads.3.dylib luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change /opt/local/lib/libgsl.0.dylib @executable_path/../Frameworks/libgsl.0.dylib luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change /opt/local/lib/libgslcblas.0.dylib @executable_path/../Frameworks/libgslcblas.0.dylib luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change /opt/local/lib/libexiv2.11.dylib @executable_path/../Frameworks/libexiv2.11.dylib luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change /opt/local/lib/libjpeg.8.dylib @executable_path/../Frameworks/libjpeg.8.dylib luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change /opt/local/lib/liblcms2.2.dylib @executable_path/../Frameworks/liblcms2.2.dylib luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
install_name_tool -change /opt/local/lib/libpng14.14.dylib @executable_path/../Frameworks/libpng14.14.dylib luminance-hdr.app/Contents/MacOS/luminance-hdr-cli
