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

#include <QReadWriteLock>

#include "Common/config.h"
#include "Fileformat/pfsoutldrimage.h"
#include "Mantiuk06Thread.h"

void pfstmo_mantiuk06 (pfs::Frame*,float,float,float,bool,ProgressHelper *);

static QReadWriteLock lock;	

Mantiuk06Thread::Mantiuk06Thread(pfs::Frame *frame, const TonemappingOptions &opts) : 
	TMOThread(frame, opts) {
}

void Mantiuk06Thread::run() {
	connect(ph, SIGNAL(valueChanged(int)), this, SIGNAL(setValue(int)));
	emit setMaximumSteps(100);
	try {
		// pfstmo_mantiuk06 not reentrant
		lock.lockForWrite();
		pfstmo_mantiuk06(workingframe,
		opts.operator_options.mantiuk06options.contrastfactor,
		opts.operator_options.mantiuk06options.saturationfactor,
		opts.operator_options.mantiuk06options.detailfactor,
		opts.operator_options.mantiuk06options.contrastequalization,ph);
		lock.unlock();
	}
	catch(...) {
		lock.unlock();
		emit tmo_error("Failed to tonemap image");
		emit deleteMe(this);
		return;
	}
	
	finalize();
} // run()

void Mantiuk06Thread::startTonemapping() {
    // Use this to circumvent a bug in GCC > 4.2 on Windows:
    // the usage of OpenMP pragmas lets the program crash with a
    // segmentation fault in libgomp.dll, since the OpenMP blocks
    // are not run in the context of the main thread (id = 0).
    // Most probably due to a mismatch of
    // PThreads (OpenMP) vs Windows threads (Qt).
    #ifdef WIN32
        run();
        return;
    #endif

    start();
}
