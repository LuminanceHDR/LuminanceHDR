/**
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ---------------------------------------------------------------------- 
 *
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */

#include <QFileInfo>
#include <QProcess>
#include <QApplication>
#include <QCoreApplication>

#include "Exif/ExifOperations.h"
#include "Fileformat/pfs_file_format.h"
#include "HdrInputLoader.h"

#include <iostream>

HdrInputLoader::HdrInputLoader(QString filename, int image_idx) : QThread(0), image_idx(image_idx), fname(filename) {
	luminance_options=LuminanceOptions::getInstance();
}

HdrInputLoader::~HdrInputLoader() {
	wait();
}

void HdrInputLoader::run() {
	try {
		//QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		QFileInfo qfi(fname);

		//get exposure time, -1 is error
		//ExifOperations methods want a std::string, we need to use the QFile::encodeName(QString).constData() trick to cope with local 8-bit encoding determined by the user's locale.
		float expotime = ExifOperations::obtain_avg_lum( QFile::encodeName(qfi.filePath()).constData() );

		QString extension=qfi.suffix().toUpper(); //get filename extension
		//now go and fill the list of image data (real payload)
		// check for extension: if JPEG:
		if (extension.startsWith("JP")) {
			QImage *newimage=new QImage(qfi.filePath());
			if (newimage->isNull())
				throw "Failed Loading Image";
			emit ldrReady(newimage, image_idx, expotime, fname, false);
			//QApplication::restoreOverrideCursor();
			return;
		}
		//if tiff
		else if(extension.startsWith("TIF")) {
			TiffReader reader(QFile::encodeName(qfi.filePath()).constData());
			//if 8bit ldr tiff
			if (reader.is8bitTiff()) {
				QImage *newimage=reader.readIntoQImage();
				if (newimage->isNull())
					throw "Failed Loading Image";
				emit ldrReady(newimage, image_idx, expotime, fname, true);
				//QApplication::restoreOverrideCursor();
				return;
			}
			//if 16bit (tiff) treat as hdr
			else if (reader.is16bitTiff()) {
				pfs::Frame *frame = reader.readIntoPfsFrame();
				if (frame == NULL)
					throw "Failed Loading Image";
				emit mdrReady(frame, image_idx, expotime, fname);
				//QApplication::restoreOverrideCursor();
				return;
			}
			//error if other tiff type
			else {
				emit loadFailed(tr("ERROR: The file<br>%1<br> is not a 8 bit or 16 bit tiff.").arg(qfi.fileName()),image_idx);
				//QApplication::restoreOverrideCursor();
				return;
			}
		//not a jpeg of tiff file, so it's raw input (hdr)
		} else {
			pfs::Frame* frame = readRawIntoPfsFrame(fname.toAscii().constData(),(luminance_options->tempfilespath).toAscii().constData(),luminance_options, true);
			if (frame == NULL)
				throw "Failed Loading Image";

			QString outfname = QString(luminance_options->tempfilespath + "/" + qfi.completeBaseName() + ".tiff");
			emit mdrReady(frame, image_idx, expotime, outfname);
		}
	}
	catch(pfs::Exception e) {
		emit loadFailed(QString(tr("ERROR: %1")).arg(e.getMessage()),image_idx);
		//QApplication::restoreOverrideCursor();
		return;
	}
	catch (...) {
		qDebug("LIT: catched exception");
		emit loadFailed(QString(tr("ERROR: Failed Loading file: %1")).arg(fname),image_idx);
		//QApplication::restoreOverrideCursor();
		return;
	}
}
