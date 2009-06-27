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

#include "tonemapperThread.h"
#include <QDir>
#include "../Common/config.h"
#include "../Filter/pfscut.h"
#include "../Common/progressHelper.h"

static ProgressHelper durand02_ph(0),
			mantiuk06_ph(0),
			mantiuk08_ph(0);


int durand02_ph_wrapper(int progress) {
	durand02_ph.newValue(progress);
	return 1;
}

int mantiuk06_ph_wrapper(int progress) {
	mantiuk06_ph.newValue(progress);
	return 1;
}

int mantiuk08_ph_wrapper(int progress) {
	mantiuk08_ph.newValue(progress);
	return 1;
}

typedef int(*pfstmo_progress_callback)(int progress);

pfs::Frame* resizeFrame(pfs::Frame* inpfsframe, int _xSize);
void applyGammaOnFrame( pfs::Frame*, const float);
//pfs::Frame* pfstmo_ashikhmin02 (pfs::Frame*,bool,float,int);
pfs::Frame* pfstmo_drago03 (pfs::Frame *, float);
pfs::Frame* pfstmo_fattal02 (pfs::Frame*,float,float,float,float,bool);
pfs::Frame* pfstmo_durand02 (pfs::Frame*,float,float,float,pfstmo_progress_callback);
pfs::Frame* pfstmo_pattanaik00 (pfs::Frame*,bool,float,float,float,bool);
pfs::Frame* pfstmo_reinhard02 (pfs::Frame*,float,float,int,int,int,bool);
pfs::Frame* pfstmo_reinhard05 (pfs::Frame *,float,float,float);
pfs::Frame* pfstmo_mantiuk06(pfs::Frame*,float,float,float,bool,pfstmo_progress_callback);
pfs::Frame* pfstmo_mantiuk08(pfs::Frame*,float,float,float,bool,pfstmo_progress_callback);

QReadWriteLock lock;	

int TonemapperThread::counter = 0;

TonemapperThread::TonemapperThread(pfs::Frame *frame, int xorigsize, const tonemapping_options opts) : QThread(0), originalxsize(xorigsize), opts(opts) {

	workingframe = pfscopy(frame);

	counter++;
	
	// Convert to CS_XYZ: tm operator now use this colorspace
	pfs::Channel *X, *Y, *Z;
	workingframe->getXYZChannels( X, Y, Z );
	pfs::transformColorSpace( pfs::CS_RGB, X, Y, Z, pfs::CS_XYZ, X, Y, Z );	

	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
	setTerminationEnabled(true);
	forciblyTerminated=false;
}

TonemapperThread::~TonemapperThread() {
	if (!forciblyTerminated) {
		wait();
	}
}

void TonemapperThread::run() {
	pfs::DOMIO pfsio;
	if (opts.pregamma != 1.0f) { 
		applyGammaOnFrame( workingframe, opts.pregamma );
	}

	if (opts.xsize != originalxsize) {
		pfs::Frame *resized = resizeFrame(workingframe, opts.xsize);
		pfsio.freeFrame(workingframe);
		workingframe = resized;
	}
	
	qDebug("TMthread:: executing tone mapping step");
	switch (opts.tmoperator) {
		case mantiuk06:
			connect(&mantiuk06_ph, SIGNAL(emitValue(int)), 
				this, SIGNAL(advanceCurrentProgress(int)));
			emit setMaximumSteps(100);
			// pfstmo_mantiuk06 not reentrant
			lock.lockForWrite();
			pfstmo_mantiuk06(workingframe,
			opts.operator_options.mantiuk06options.contrastfactor,
			opts.operator_options.mantiuk06options.saturationfactor,
			opts.operator_options.mantiuk06options.detailfactor,
			opts.operator_options.mantiuk06options.contrastequalization,
			mantiuk06_ph_wrapper);
			lock.unlock();
		break;
		case mantiuk08:
			try {
				connect(&mantiuk08_ph, SIGNAL(emitValue(int)), 
					this, SIGNAL(advanceCurrentProgress(int)));
				emit setMaximumSteps(100);
				pfstmo_mantiuk08(workingframe,
				opts.operator_options.mantiuk08options.colorsaturation,
				opts.operator_options.mantiuk08options.contrastenhancement,
				opts.operator_options.mantiuk08options.luminancelevel,
				opts.operator_options.mantiuk08options.setluminance,mantiuk08_ph_wrapper);
			}
			catch(...) {
				pfsio.freeFrame(workingframe);
				emit finished();
				return;
			}
		break;
		case fattal:
			emit setMaximumSteps(0);
			try {
				// pfstmo_fattal02 not reentrant
				lock.lockForWrite();
				pfstmo_fattal02(workingframe,
				opts.operator_options.fattaloptions.alpha,
				opts.operator_options.fattaloptions.beta,
				opts.operator_options.fattaloptions.color,
				opts.operator_options.fattaloptions.noiseredux,
				opts.operator_options.fattaloptions.newfattal);
				lock.unlock();
			}
			catch(...) {
				lock.unlock();
				pfsio.freeFrame(workingframe);
				emit finished();
				return;
			}
		break;
		//case ashikhmin:
		//	lock.lockForWrite();
		//	pfstmo_ashikhmin02(workingframe,
		//	opts.operator_options.ashikhminoptions.simple,
		//	opts.operator_options.ashikhminoptions.lct,
		//	opts.operator_options.ashikhminoptions.eq2 ? 2 : 4);
		//	lock.unlock();
		//break;
		case durand:
			connect(&durand02_ph, SIGNAL(emitValue(int)), 
				this, SIGNAL(advanceCurrentProgress(int)));
			emit setMaximumSteps(100);
			// pfstmo_durand02 not reentrant
			lock.lockForWrite();
			pfstmo_durand02(workingframe,
			opts.operator_options.durandoptions.spatial,
			opts.operator_options.durandoptions.range,
			opts.operator_options.durandoptions.base, durand02_ph_wrapper);
			lock.unlock();
		break;
		case drago:
			emit setMaximumSteps(0);
			try {
				pfstmo_drago03(workingframe, opts.operator_options.dragooptions.bias);
			}
			catch(...) {
				lock.unlock();
				pfsio.freeFrame(workingframe);
				emit finished();
				return;
			}
		break;
		case pattanaik:
			emit setMaximumSteps(0);
			try {
				pfstmo_pattanaik00(workingframe,
				opts.operator_options.pattanaikoptions.local,
				opts.operator_options.pattanaikoptions.multiplier,
				opts.operator_options.pattanaikoptions.cone,
				opts.operator_options.pattanaikoptions.rod,
				opts.operator_options.pattanaikoptions.autolum);
			}
			catch(...) {
				pfsio.freeFrame(workingframe);
				emit finished();
				return;
			}
		break;
		case reinhard02:
			emit setMaximumSteps(0);
			try {
				pfstmo_reinhard02(workingframe,
				opts.operator_options.reinhard02options.key,
				opts.operator_options.reinhard02options.phi,
				opts.operator_options.reinhard02options.range,
				opts.operator_options.reinhard02options.lower,
				opts.operator_options.reinhard02options.upper,
				opts.operator_options.reinhard02options.scales);
			}
			catch(...) {
				pfsio.freeFrame(workingframe);
				emit finished();
				return;
			}
		break;
		case reinhard05:
			emit setMaximumSteps(0);
			try {
				pfstmo_reinhard05(workingframe,
				opts.operator_options.reinhard05options.brightness,
				opts.operator_options.reinhard05options.chromaticAdaptation,
				opts.operator_options.reinhard05options.lightAdaptation);
			}
			catch(...) {
				pfsio.freeFrame(workingframe);
				emit finished();
				return;
			}
		break;
	} //switch (opts.tmoperator)
	const QImage& res = fromLDRPFStoQImage(workingframe);
	pfsio.freeFrame(workingframe);
	emit imageComputed(res, &opts);
}
//
// run()
//

void TonemapperThread::terminateRequested() {
	//TODO
	pfs::DOMIO pfsio;
	if (forciblyTerminated)
		return;
	forciblyTerminated=true;
	//HACK oddly enough for the other operators we cannot emit finished (it segfaults)
	if (opts.tmoperator==mantiuk06 || opts.tmoperator==ashikhmin || opts.tmoperator==pattanaik )
		emit finished();
	pfsio.freeFrame(workingframe);
	terminate();
}

