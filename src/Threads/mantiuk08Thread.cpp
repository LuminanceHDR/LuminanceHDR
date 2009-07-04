/*
 * This file is a part of Qtpfsgui package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2008,2007 Giuseppe Rota
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

#include "mantiuk08Thread.h"
#include "../Common/config.h"
#include "../Filter/pfscut.h"
#include "../Fileformat/pfsoutldrimage.h"

pfs::Frame* resizeFrame(pfs::Frame* inpfsframe, int xSize);
void applyGammaOnFrame( pfs::Frame*, const float);
void pfstmo_mantiuk08 (pfs::Frame*,float,float,float,bool,ProgressHelper *);

int Mantiuk08Thread::counter = 0;

Mantiuk08Thread::Mantiuk08Thread(pfs::Frame *frame, int xorigsize, const tonemapping_options opts) : 
	QThread(0), originalxsize(xorigsize), opts(opts) {

	workingframe = pfscopy(frame);

	counter++;
	ph = new ProgressHelper(0);

	// Convert to CS_XYZ: tm operator now use this colorspace
	pfs::Channel *X, *Y, *Z;
	workingframe->getXYZChannels( X, Y, Z );
	pfs::transformColorSpace( pfs::CS_RGB, X, Y, Z, pfs::CS_XYZ, X, Y, Z );	
}

Mantiuk08Thread::~Mantiuk08Thread() {
	delete ph;
}

void Mantiuk08Thread::run() {
	pfs::DOMIO pfsio;
	if (opts.pregamma != 1.0f) { 
		applyGammaOnFrame( workingframe, opts.pregamma );
	}

	if (opts.xsize != originalxsize) {
		pfs::Frame *resized = resizeFrame(workingframe, opts.xsize);
		pfsio.freeFrame(workingframe);
		workingframe = resized;
	}
	
	connect(ph, SIGNAL(valueChanged(int)), this, SIGNAL(setValue(int)));
	emit setMaximumSteps(100);
	try {
		pfstmo_mantiuk08(workingframe,
		opts.operator_options.mantiuk08options.colorsaturation,
		opts.operator_options.mantiuk08options.contrastenhancement,
		opts.operator_options.mantiuk08options.luminancelevel,
		opts.operator_options.mantiuk08options.setluminance,ph);
	}
	catch(pfs::Exception e) {
		pfsio.freeFrame(workingframe);
		if (strcmp("failed to analyse the image", e.getMessage())) 
				return;
		emit tmo_error("Failed to tonemap image");
		return;
	}
	catch(...) {
		pfsio.freeFrame(workingframe);
		emit tmo_error("Failed to tonemap image");
		return;
	}
	const QImage& res = fromLDRPFStoQImage(workingframe);
	
	pfsio.freeFrame(workingframe);
	if (!(ph->isTerminationRequested())) {
		emit imageComputed(res, &opts);
		emit finished();
	}
}
//
// run()
//

void Mantiuk08Thread::terminateRequested() {
	//TODO
	ph->terminate(true);
}

