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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing 
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef TONEMAPPERTHREAD_H
#define TONEMAPPERTHREAD_H

#include <QThread>
#include <QReadWriteLock>
#include <QImage>
#include "../Common/options.h"
#include "../Common/global.h"
#include "../Libpfs/pfs.h"
class QProgressBar;

class TonemapperThread : public QThread {
Q_OBJECT

public:
	//tonemapping_options passed by value, bit-copy should be enough
	TonemapperThread(pfs::Frame *frame, int origsize, const tonemapping_options opts);
	~TonemapperThread();
public slots:
	void terminateRequested();
signals:
	void imageComputed(const QImage&, tonemapping_options *);
	void setMaximumSteps(int);
	void advanceCurrentProgress(int);
	void advanceCurrentProgress();
protected:
	void run();
private:
	pfs::Frame *workingframe;
	int originalxsize;
	int ldr_output_cs;
	//QString cachepath;
	bool colorspaceconversion;
	tonemapping_options opts;
	//void fetch(QString);
	//void swap(pfs::Frame *, QString );
	bool forciblyTerminated;
	//void dumpOpts();
	//enum {from_resize,from_pregamma,from_tm} status;
	QImage fromLDRPFStoQImage( pfs::Frame* inpfsframe );
	static int counter;
};

#endif
