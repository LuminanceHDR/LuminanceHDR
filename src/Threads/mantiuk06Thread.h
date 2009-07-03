/*
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

#ifndef MANTIUK06THREAD_H
#define MANTIUK06THREAD_H

#include <QThread>
#include <QImage>
#include "../Common/options.h"
#include "../Common/global.h"
#include "../Libpfs/pfs.h"
#include "../Common/progressHelper.h"

class Mantiuk06Thread : public QThread {
Q_OBJECT

public:
	//tonemapping_options passed by value, bit-copy should be enough
	Mantiuk06Thread(pfs::Frame *frame, int origsize, const tonemapping_options opts);
	~Mantiuk06Thread();
public slots:
	void terminateRequested();
signals:
	void imageComputed(const QImage&, tonemapping_options *);
	void setMaximumSteps(int);
	void advanceCurrentProgress(int);
	void advanceCurrentProgress();
	void tmo_error(const char *);
protected:
	void run();
private:
	pfs::Frame *workingframe;
	int originalxsize;
	int ldr_output_cs;
	bool colorspaceconversion;
	tonemapping_options opts;
	ProgressHelper *ph;
	static int counter;
};

#endif
