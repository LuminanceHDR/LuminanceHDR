/*
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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing 
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef TMOTHREAD_H
#define TMOTHREAD_H

#include <QThread>
#include <QImage>

#include "Common/options.h"
#include "Common/global.h"
#include "Libpfs/pfs.h"
#include "Common/ProgressHelper.h"

class TMOThread : public QThread {
Q_OBJECT

public:
	TMOThread(pfs::Frame *frame, const TonemappingOptions &opts);
	virtual ~TMOThread();
        virtual void startTonemapping();

public slots:
	virtual void terminateRequested();
signals:
	virtual void imageComputed(const QImage&);
	virtual void setMaximumSteps(int) ;
	virtual void setValue(int);
	virtual void setValue();
	virtual void finished();
	virtual void deleteMe(TMOThread *);
	virtual void tmo_error(const char *);
protected:
	virtual void run() = 0;
	pfs::Frame *workingframe;
	const TonemappingOptions &opts;
	ProgressHelper *ph;
};

#endif
