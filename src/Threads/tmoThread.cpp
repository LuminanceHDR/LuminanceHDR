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

#include "tmoThread.h"
#include "../Common/config.h"
#include "../Filter/pfscut.h"
#include "../Fileformat/pfsoutldrimage.h"

#include <iostream>

TMOThread::TMOThread(pfs::Frame *frame, const TonemappingOptions &opts) : 
	QThread(0), opts(opts) {

	workingframe = pfscopy(frame);

	ph = new ProgressHelper(0);

	// Convert to CS_XYZ: tm operator now use this colorspace
	pfs::Channel *X, *Y, *Z;
	workingframe->getXYZChannels( X, Y, Z );
	pfs::transformColorSpace( pfs::CS_RGB, X, Y, Z, pfs::CS_XYZ, X, Y, Z );	
}

TMOThread::~TMOThread() {
	wait();
	pfs::DOMIO pfsio;
	pfsio.freeFrame(workingframe);
	delete ph;
	std::cout << "~TMOThread()" << std::endl;
}

void TMOThread::terminateRequested() {
	ph->terminate(true);
}

