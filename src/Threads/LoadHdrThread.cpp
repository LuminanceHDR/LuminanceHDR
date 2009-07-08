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
#include <QProcess>
#include <QCoreApplication>

#include "LoadHdrThread.h"
#include "Fileformat/pfstiff.h"

pfs::Frame* readEXRfile  (const char * filename);
pfs::Frame* readRGBEfile (const char * filename);

LoadHdrThread::LoadHdrThread(QString fname, QString RecentDirHDRSetting) : QThread(0), fname(fname),RecentDirHDRSetting(RecentDirHDRSetting), progress(NULL) {
	qtpfsgui_options=QtpfsguiOptions::getInstance();
}

LoadHdrThread::~LoadHdrThread() {
	wait();
}

void LoadHdrThread::run() {
	if( fname.isEmpty() )
		return;

	QFileInfo qfi(fname);
	if (!qfi.isReadable()) {
		qDebug("File %s is not readable.", fname.toAscii().constData());
		emit load_failed(tr("ERROR: The following file is not readable: %1").arg(fname));
		return;
	}
	// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings.
	if (RecentDirHDRSetting != qfi.path() ) {
		emit updateRecentDirHDRSetting(qfi.path());
	}
	pfs::Frame* hdrpfsframe = NULL;
	QStringList rawextensions;
	rawextensions << "CRW" << "CR2" << "NEF" << "DNG" << "MRW" << "ORF" << "KDC" << "DCR" << "ARW" << "RAF" << "PTX" << "PEF" << "X3F" << "RAW" << "SR2";
	QString extension = qfi.suffix().toUpper();
	bool rawinput = (rawextensions.indexOf(extension)!=-1);
	try {
		char* encodedFileName=strdup(QFile::encodeName(qfi.filePath()).constData());
		if (extension=="EXR") {
			hdrpfsframe = readEXRfile(encodedFileName);
		} else if (extension=="HDR") {
			hdrpfsframe = readRGBEfile(encodedFileName);
		} else if (extension=="PFS") {
			//TODO
			const char *fname = encodedFileName;
			FILE *fd = fopen(fname, "rb");
			if (!fd) {
				emit load_failed(tr("ERROR: Cannot open file: %1").arg(fname));
				return;
			}
			pfs::DOMIO pfsio;
			hdrpfsframe=pfsio.readFrame( fd);
			fclose(fd);
		} else if (extension.startsWith("TIF")) {	
			TiffReader reader(encodedFileName);
			connect(&reader, SIGNAL(maximumValue(int)), this, SIGNAL(maximumValue(int)));
			connect(&reader, SIGNAL(nextstep(int)), this, SIGNAL(nextstep(int)));
			hdrpfsframe = reader.readIntoPfsFrame(); //from 8,16,32,logluv to pfs::Frame
		}
		else if (rawinput) {
			qDebug("TH: raw file");
			QProcess *rawconversion = new QProcess(0);
			rawconversion->setWorkingDirectory(qtpfsgui_options->tempfilespath);
			#ifdef WIN32
			QString separator(";");
			#else
			QString separator(":");
			#endif
			QStringList env = QProcess::systemEnvironment();
			env.replaceInStrings(QRegExp("^PATH=(.*)", Qt::CaseInsensitive), "PATH=\\1"+separator+QCoreApplication::applicationDirPath());
			rawconversion->setEnvironment(env);
			
			QStringList params = qtpfsgui_options->dcraw_options;
			params << fname;
			
			#ifdef Q_WS_MAC
			rawconversion->start(QCoreApplication::applicationDirPath()+"/dcraw", params);
			#elifdef Q_WS_WIN
			rawconversion->start(QCoreApplication::applicationDirPath()+"/dcraw.exe", params);
			#else
			rawconversion->start("dcraw", params);
			#endif

			//blocking, timeout of 10 sec
			if(!rawconversion->waitForStarted(10000)) {
				qDebug("Cannot start dcraw on file: %s", qPrintable(fname));
				emit load_failed(tr("ERROR: Cannot start dcraw on file: %1").arg(fname));
				return;
			}
			
			//blocking, timeout of 5mins
			if(!rawconversion->waitForFinished(300000)) {
				qDebug("Error or timeout occured while executing dcraw on file: %s", qPrintable(fname));
				emit load_failed(tr("ERROR: Error or timeout occured while executing dcraw on file: %1").arg(fname));
				return;
			}

			QString outfname = QString(qfi.path() + "/"+qfi.completeBaseName()+".tiff");
			qDebug("TH: Loading back file name=%s", qPrintable(outfname));
			TiffReader reader(QFile::encodeName(outfname).constData());
			hdrpfsframe = reader.readIntoPfsFrame(); //from 8,16,32,logluv to pfs::Frame

			QFile::remove(outfname);
		} //raw file detected
		else {
			qDebug("TH: File %s has unsupported extension.", qPrintable(fname));
			emit load_failed(tr("ERROR: File %1 has unsupported extension.").arg(fname));
			return;
		}
		free(encodedFileName);
#if 0
		pfs::Channel *R,*G,*B;
		hdrpfsframe->getRGBChannels( R, G, B );
		float maxYval=-1; float minYval=1e6;
		float maxRval=-1; float minRval=1e6;
		float maxGval=-1; float minGval=1e6;
		float maxBval=-1; float minBval=1e6;
		for (int i=0; i<hdrpfsframe->getHeight()*hdrpfsframe->getWidth(); i++) {
			float yval = 0.212656f*(*R)(i)+0.715158f*(*G)(i)+0.072186f*(*B)(i);
			maxYval = (yval>maxYval) ? yval : maxYval;
			minYval = (yval<minYval) ? yval : minYval;
			maxRval = ((*R)(i)>maxRval) ? (*R)(i) : maxRval;
			minRval = ((*R)(i)<minRval) ? (*R)(i) : minRval;
			maxGval = ((*G)(i)>maxGval) ? (*G)(i) : maxGval;
			minGval = ((*G)(i)<minGval) ? (*G)(i) : minGval;
			maxBval = ((*B)(i)>maxBval) ? (*B)(i) : maxBval;
			minBval = ((*B)(i)<minBval) ? (*B)(i) : minBval;
		}
		qDebug("minYval=%f, maxYval=%f",minYval,maxYval);
		qDebug("minRval=%f, maxRval=%f",minRval,maxRval);
		qDebug("minGval=%f, maxGval=%f",minGval,maxGval);
		qDebug("minBval=%f, maxBval=%f",minBval,maxBval);
#endif
		if (hdrpfsframe == NULL)
			throw "Error loading file";
	}
	catch(pfs::Exception e) {
		emit load_failed(tr("ERROR: %1").arg(e.getMessage()));
		return;
	}
	catch (...) {
		qDebug("TH: catched exception");
		emit load_failed(tr("ERROR: Failed loading file: %1").arg(fname));
		return;
	}
	emit hdr_ready(hdrpfsframe,fname);
}
