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
#include "Fileformat/pfstiff.h"
#include "HdrInputLoader.h"

#include <iostream>

HdrInputLoader::HdrInputLoader(QString filename, int image_idx, QStringList dcrawOpts) : QThread(0), image_idx(image_idx), fname(filename), dcrawOpts(dcrawOpts) {
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
			//
			// Extract thumbnail on disk	
			//
			QProcess *extract_thumbnail = new QProcess(0);

			#ifdef WIN32
			QString separator(";");
			#else
			QString separator(":");
			#endif
			
			QStringList env = QProcess::systemEnvironment();
			env.replaceInStrings(QRegExp("^PATH=(.*)", Qt::CaseInsensitive), "PATH=\\1"+separator+QCoreApplication::applicationDirPath());

			extract_thumbnail->setEnvironment(env);

		    QStringList parameters;
		    parameters << "-e " << fname;
			
			#ifdef Q_WS_MAC
			extract_thumbnail->start(QCoreApplication::applicationDirPath()+"/dcraw", parameters);
			#elif defined(Q_WS_WIN)
			extract_thumbnail->start(QCoreApplication::applicationDirPath()+"/dcraw.exe", parameters);
			#else
			extract_thumbnail->start("dcraw", parameters);
			#endif
			
			if(!extract_thumbnail->waitForStarted(10000)) {
				emit loadFailed(tr("ERROR: Cannot start dcraw to create thumbnail of file: %1").arg(qfi.fileName()),image_idx);
				//QApplication::restoreOverrideCursor();
				return;
			}
			
			//blocking, timeout of 5mins
			if(!extract_thumbnail->waitForFinished(300000)) {
				emit loadFailed(tr("ERROR: Error or timeout occured, dcraw on file: %1").arg(qfi.fileName()),image_idx);
				//QApplication::restoreOverrideCursor();
				return;
			}

			//Start raw conversion

			QProcess *rawconversion = new QProcess(0);

			rawconversion->setEnvironment(env);
			
			QStringList params = dcrawOpts;
			params << fname;
			
			#ifdef Q_WS_MAC
			rawconversion->start(QCoreApplication::applicationDirPath()+"/dcraw", params);
			#elif defined(Q_WS_WIN)
			rawconversion->start(QCoreApplication::applicationDirPath()+"/dcraw.exe", params);
			#else
			rawconversion->start("dcraw", params);
			#endif

			//blocking, timeout of 10 sec
			if(!rawconversion->waitForStarted(10000)) {
				emit loadFailed(tr("ERROR: Cannot start dcraw on file: %1").arg(qfi.fileName()),image_idx);
				//QApplication::restoreOverrideCursor();
				return;
			}
		
			//blocking, timeout of 5mins
			if(!rawconversion->waitForFinished(300000)) {
				emit loadFailed(tr("ERROR: Error or timeout occured while executing dcraw on file: %1").arg(qfi.fileName()),image_idx);
				//QApplication::restoreOverrideCursor();
				return;
			}

			QString outfname = QString(qfi.path() + "/"+qfi.completeBaseName()+".tiff");
			qDebug("TH: Loading back file name=%s", qPrintable(outfname));
			TiffReader reader(QFile::encodeName(outfname).constData());
			//if 8bit ldr tiff
			if (reader.is8bitTiff()) {
				qDebug("raw -> 8bit tiff");
				QImage *newimage=reader.readIntoQImage();
				emit ldrReady(newimage, image_idx, expotime, outfname, true);
				//QApplication::restoreOverrideCursor();
				return;
			}
			//if 16bit (tiff) treat as hdr
			else if (reader.is16bitTiff()) {
				qDebug("raw -> 16bit tiff");
				pfs::Frame *frame=reader.readIntoPfsFrame();
				emit mdrReady(frame, image_idx, expotime, outfname);
				//QApplication::restoreOverrideCursor();
				return;
			}
			//error if other tiff type
			else {
				emit loadFailed(QString(tr("ERROR: The file<br>%1<br> is not a 8 bit or 16 bit tiff.")).arg(qfi.fileName()),image_idx);
				//QApplication::restoreOverrideCursor();
				return;
			}
			//now do not remove tiff file, it might be required by align_image_stack

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
