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

#include <QDir>

#include "tonemapperThread.h"
#include "Common/config.h"
#include "Filter/pfscut.h"
#include "Common/ProgressHelper.h"
#include "Fileformat/pfsoutldrimage.h"

#include <iostream>

static ProgressHelper drago03_ph(0), 
					  durand02_ph(0),
					  fattal02_ph(0),
					  mantiuk06_ph(0),
					  mantiuk08_ph(0);

/* Return codes for the progress_callback */
#define PFSTMO_CB_CONTINUE 1
#define PFSTMO_CB_ABORT -1

/**
 **  This functions is called from a tone-mapper to report current progress.
 **
 **  @param progress the current progress in percent (grows from 0 to 100)
 **  @return return PFSTMO_CB_CONTINUE to continue tone-mapping or
 **  PFSTMO_CB_ABORT to request to stop. In some cases tone-mapping may
 **  not stop immediately, even if PFSTMO_CB_ABORT was returned.
 **
 **/

int drago03_ph_wrapper(int progress) {
	drago03_ph.newValue(progress);
	if (drago03_ph.isTerminationRequested()) 
		return PFSTMO_CB_ABORT;
	else
		return PFSTMO_CB_CONTINUE;
}

int durand02_ph_wrapper(int progress) {
	durand02_ph.newValue(progress);
	if (durand02_ph.isTerminationRequested()) 
		return PFSTMO_CB_ABORT;
	else
		return PFSTMO_CB_CONTINUE;
}

int fattal02_ph_wrapper(int progress) {
	fattal02_ph.newValue(progress);
	if (fattal02_ph.isTerminationRequested()) 
		return PFSTMO_CB_ABORT;
	else
		return PFSTMO_CB_CONTINUE;
}

int mantiuk06_ph_wrapper(int progress) {
	mantiuk06_ph.newValue(progress);
	if (mantiuk06_ph.isTerminationRequested())
		return PFSTMO_CB_ABORT;
	else
		return PFSTMO_CB_CONTINUE;
}

int mantiuk08_ph_wrapper(int progress) {
	mantiuk08_ph.newValue(progress);
	if (mantiuk08_ph.isTerminationRequested())
		return PFSTMO_CB_ABORT;
	else
		return PFSTMO_CB_CONTINUE;
}

typedef int(*pfstmo_progress_callback)(int progress);

pfs::Frame* resizeFrame(pfs::Frame* inpfsframe, int _xSize);
void applyGammaOnFrame( pfs::Frame*, const float);
//void pfstmo_ashikhmin02 (pfs::Frame*,bool,float,int);
//void pfstmo_drago03 (pfs::Frame *, float,pfstmo_progress_callback);
//void pfstmo_durand02 (pfs::Frame*,float,float,float,pfstmo_progress_callback);
//void pfstmo_fattal02 (pfs::Frame*,float,float,float,float,bool,pfstmo_progress_callback);
//void pfstmo_mantiuk06(pfs::Frame*,float,float,float,bool,pfstmo_progress_callback);
//void pfstmo_mantiuk08(pfs::Frame*,float,float,float,bool,pfstmo_progress_callback);
//void pfstmo_pattanaik00 (pfs::Frame*,bool,float,float,float,bool);
//void pfstmo_reinhard02 (pfs::Frame*,float,float,int,int,int,bool);
//void pfstmo_reinhard05 (pfs::Frame *,float,float,float);

QReadWriteLock alock;	

int TonemapperThread::counter = 0;

TonemapperThread::TonemapperThread(pfs::Frame *frame, int xorigsize, const TonemappingOptions opts) : QThread(0), originalxsize(xorigsize), opts(opts) {

	workingframe = pfscopy(frame);

	counter++;
	
	// Convert to CS_XYZ: tm operator now use this colorspace
	pfs::Channel *X, *Y, *Z;
	workingframe->getXYZChannels( X, Y, Z );
	pfs::transformColorSpace( pfs::CS_RGB, X, Y, Z, pfs::CS_XYZ, X, Y, Z );	

	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

TonemapperThread::~TonemapperThread() {
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
			/*
			connect(&mantiuk06_ph, SIGNAL(emitValue(int)), 
				this, SIGNAL(advanceCurrentProgress(int)));
			emit setMaximumSteps(100);
			// pfstmo_mantiuk06 not reentrant
			alock.alockForWrite();
			pfstmo_mantiuk06(workingframe,
			opts.operator_options.mantiuk06options.contrastfactor,
			opts.operator_options.mantiuk06options.saturationfactor,
			opts.operator_options.mantiuk06options.detailfactor,
			opts.operator_options.mantiuk06options.contrastequalization,
			mantiuk06_ph_wrapper);
			alock.unlock();*/
		break;
		case mantiuk08:
		/*
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
			catch(pfs::Exception e) {
				pfsio.freeFrame(workingframe);
				emit tmo_error(e.getMessage());
				return;
			}
			catch(...) {
				pfsio.freeFrame(workingframe);
				emit tmo_error("Failed to tonemap image");
				return;
			}*/
		break;
		case fattal:
			/*
			connect(&fattal02_ph, SIGNAL(emitValue(int)), 
					this, SIGNAL(advanceCurrentProgress(int)));
			emit setMaximumSteps(100);
			try {
				// pfstmo_fattal02 not reentrant
				alock.lockForWrite();
				pfstmo_fattal02(workingframe,
				opts.operator_options.fattaloptions.alpha,
				opts.operator_options.fattaloptions.beta,
				opts.operator_options.fattaloptions.color,
				opts.operator_options.fattaloptions.noiseredux,
				opts.operator_options.fattaloptions.newfattal,fattal02_ph_wrapper);
				alock.unlock();
			}
			catch(pfs::Exception e) {
				alock.unlock();
				pfsio.freeFrame(workingframe);
				emit tmo_error(e.getMessage());
				return;
			}
			catch(...) {
				alock.unlock();
				pfsio.freeFrame(workingframe);
				emit tmo_error("Failed to tonemap image");
				return;
			}*/
		break;
		case ashikhmin:
		/*
			emit setMaximumSteps(0);
			pfstmo_ashikhmin02(workingframe,
			opts.operator_options.ashikhminoptions.simple,
			opts.operator_options.ashikhminoptions.lct,
			opts.operator_options.ashikhminoptions.eq2 ? 2 : 4);*/
		break;
		case durand:
		/*
			try {
				connect(&durand02_ph, SIGNAL(emitValue(int)), 
					this, SIGNAL(advanceCurrentProgress(int)));
				emit setMaximumSteps(100);
				// pfstmo_durand02 not reentrant
				alock.alockForWrite();
				pfstmo_durand02(workingframe,
				opts.operator_options.durandoptions.spatial,
				opts.operator_options.durandoptions.range,
				opts.operator_options.durandoptions.base, durand02_ph_wrapper);
				alock.unalock();
			}
			catch(...) {
				alock.unalock();
				pfsio.freeFrame(workingframe);
				emit tmo_error("Failed to tonemap image");
				return;
			}*/
		break;
		case drago:
			/*
			connect(&drago03_ph, SIGNAL(emitValue(int)), 
					this, SIGNAL(advanceCurrentProgress(int)));
			emit setMaximumSteps(100);
			try {
				pfstmo_drago03(workingframe, opts.operator_options.dragooptions.bias,drago03_ph_wrapper);
			}
			catch(pfs::Exception e) {
				pfsio.freeFrame(workingframe);
				emit tmo_error(e.getMessage());
				return;
			}
			catch(...) {
				pfsio.freeFrame(workingframe);
				emit tmo_error("Failed to tonemap image");
				return;
			}*/
		break;
		case pattanaik:
		/*
			emit setMaximumSteps(0);
			try {
				pfstmo_pattanaik00(workingframe,
				opts.operator_options.pattanaikoptions.local,
				opts.operator_options.pattanaikoptions.multiplier,
				opts.operator_options.pattanaikoptions.cone,
				opts.operator_options.pattanaikoptions.rod,
				opts.operator_options.pattanaikoptions.autolum);
			}
			catch(pfs::Exception e) {
				pfsio.freeFrame(workingframe);
				emit tmo_error(e.getMessage());
				return;
			}
			catch(...) {
				pfsio.freeFrame(workingframe);
				emit tmo_error("Failed to tonemap image");
				return;
			}*/
		break;
		case reinhard02:
		/*
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
			catch(pfs::Exception e) {
				pfsio.freeFrame(workingframe);
				emit tmo_error(e.getMessage());
				return;
			}
			catch(...) {
				pfsio.freeFrame(workingframe);
				emit tmo_error("Failed to tonemap image");
				return;
			}*/
		break;
		case reinhard05:
		/*
			emit setMaximumSteps(0);
			try {
				pfstmo_reinhard05(workingframe,
				opts.operator_options.reinhard05options.brightness,
				opts.operator_options.reinhard05options.chromaticAdaptation,
				opts.operator_options.reinhard05options.lightAdaptation);
			}
			catch(pfs::Exception e) {
				pfsio.freeFrame(workingframe);
				emit tmo_error(e.getMessage());
				return;
			}
			catch(...) {
				pfsio.freeFrame(workingframe);
				emit tmo_error("Failed to tonemap image");
				return;
			}*/
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
}

