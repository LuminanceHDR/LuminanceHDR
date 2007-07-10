/**
 * This file is a part of Qtpfsgui package.
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
#include <QMessageBox>
#include "io_threads.h"
#include "../Fileformat/pfstiff.h"
#include "../Fileformat/pfsindcraw.h"
pfs::Frame* readEXRfile  (const char * filename);
pfs::Frame* readRGBEfile (const char * filename);

LoadHdrThread::LoadHdrThread(QString fname, QString RecentDirHDRSetting, qtpfsgui_opts *opts) : QThread(0), fname(fname),RecentDirHDRSetting(RecentDirHDRSetting), qtpfsgui_options(opts) {
}

LoadHdrThread::~LoadHdrThread() {
	wait();
// 	qDebug("~LoadHdrThread");
}

void LoadHdrThread::run() {
	if( !fname.isEmpty() ) {
		QFileInfo qfi(fname);
		if (!qfi.isReadable()) {
			qDebug("file not readable %s", fname.toAscii().constData());
			emit load_failed(fname);
			return;
		}
		// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings.
		if (RecentDirHDRSetting != qfi.path() ) {
			emit updateRecentDirHDRSetting(qfi.path());
		}
		pfs::Frame* hdrpfsframe = NULL;
		QStringList rawextensions;
		rawextensions << "CRW" << "CR2" << "NEF" << "DNG" << "MRW" << "ORF" << "KDC" << "DCR" << "ARW" << "RAF" << "PTX" << "PEF" << "X3F" << "RAW";
		QString extension = qfi.suffix().toUpper();
		bool rawinput = (rawextensions.indexOf(extension)!=-1);
		try {
			if (extension=="EXR") {
				hdrpfsframe = readEXRfile(qfi.filePath().toUtf8().constData());
			} else if (extension=="HDR") {
				hdrpfsframe = readRGBEfile(qfi.filePath().toUtf8().constData());
			} else if (extension=="PFS") {
				pfs::DOMIO pfsio;
				hdrpfsframe=pfsio.readFrame(qfi.filePath());
				hdrpfsframe->convertXYZChannelsToRGB();
			} else if (extension.startsWith("TIF")) {
				TiffReader reader(qfi.filePath().toUtf8().constData());
				hdrpfsframe = reader.readIntoPfsFrame(); //from 8,16,32,logluv to pfs::Frame
			} 
			else if (rawinput) {
				qDebug("TH: raw file");
				hdrpfsframe = readRAWfile(qfi.filePath().toUtf8().constData(), &(qtpfsgui_options->dcraw_options));
			}
			else {
				qDebug("TH: unknown ext");
				emit load_failed(fname);
				return;
			}
		} catch (...) {
			qDebug("TH: catched exception");
			emit load_failed(fname);
			return;
		}
		assert(hdrpfsframe!=NULL);
		emit hdr_ready(hdrpfsframe,fname);
	}
}
