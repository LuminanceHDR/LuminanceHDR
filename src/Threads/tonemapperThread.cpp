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

#include "tonemapperThread.h"
#include <QDir>
#include "../Common/config.h"
pfs::Frame* resizeFrame(pfs::Frame* inpfsframe, int _xSize);
void applyGammaFrame( pfs::Frame*, const float);
pfs::Frame* pfstmo_ashikhmin02 (pfs::Frame*,bool,float,int);
pfs::Frame* pfstmo_drago03 (pfs::Frame *, float);
pfs::Frame* pfstmo_fattal02 (pfs::Frame*,float,float,float,float,bool);
pfs::Frame* pfstmo_durand02 (pfs::Frame*,float,float,float);
pfs::Frame* pfstmo_pattanaik00 (pfs::Frame*,bool,float,float,float,bool);
pfs::Frame* pfstmo_reinhard02 (pfs::Frame*,float,float,int,int,int,bool);
pfs::Frame* pfstmo_reinhard05 (pfs::Frame *,float,float,float);
pfs::Frame* pfstmo_mantiuk06(pfs::Frame*,float,float,bool);

// width of the pfs:frame written on disk during resize operation, cannot be the 100% size: originalxsize, because i don't resize to 100% and write to disk.
int xsize=-1; //-1 means nothing has been computed yet
// gamma of the pfs:frame written on disk after resize
float pregamma=-1; //-1 means NOT VALID (size has changed/is changing)
//pregamma!=-1 means that the pregamma frame has the xsize as width
QReadWriteLock lock;


TonemapperThread::TonemapperThread(int xorigsize, const tonemapping_options opts) : QThread(0), originalxsize(xorigsize), opts(opts) {
	colorspaceconversion=false;
	settings.beginGroup(GROUP_TONEMAPPING);
	cachepath=settings.value(KEY_TEMP_RESULT_PATH,QDir::currentPath()).toString();
	settings.endGroup();

	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
	setTerminationEnabled(true);
	forciblyTerminated=false;
}

TonemapperThread::~TonemapperThread() {
	if (!forciblyTerminated) {
		wait();
	}
}

void TonemapperThread::fetch(QString filename) {
	pfs::DOMIO pfsio;
	workingframe=pfsio.readFrame(cachepath+filename);
}

void TonemapperThread::swap(pfs::Frame *frame, QString filename) {
	//swap frame to hd
	pfs::DOMIO pfsio;
	pfsio.writeFrame(frame,cachepath+filename);
}

void TonemapperThread::run() {
	qDebug("TMthread::begin thread, size=%d, gamma=%f",xsize, pregamma);
	lock.lockForRead();
	if (opts.xsize==originalxsize && opts.pregamma==1.0f) {
		//original goes into tone mapping
		qDebug("TMthread::original goes into tone mapping");
		fetch("/original.pfs");
		status=from_tm;
		colorspaceconversion=true;
		emit setMaximumSteps(2);
	} else if (opts.xsize==xsize && opts.pregamma==1.0f) {
		//resized goes into tone mapping
		qDebug("TMthread::resized goes into tone mapping");
		fetch("/after_resize.pfs");
		status=from_tm;
		colorspaceconversion=true;
		emit setMaximumSteps(2);
	} else if ( (opts.xsize==xsize && opts.pregamma==pregamma) || (opts.xsize==originalxsize && xsize==-1 && opts.pregamma==pregamma) ) {
		//after_pregamma goes into tone mapping
		qDebug("TMthread::after_pregamma goes into tone mapping");
		fetch("/after_pregamma.pfs");
		status=from_tm;
		emit setMaximumSteps(2);
	} else if (opts.xsize==xsize) {
		//resized goes into pregamma
		qDebug("TMthread::resized goes into pregamma");
		fetch("/after_resize.pfs");
		status=from_pregamma;
		emit setMaximumSteps(3);
	} else if (opts.xsize==originalxsize) {
		//original goes into pregamma
		qDebug("TMthread::original goes into pregamma");
		fetch("/original.pfs");
		status=from_pregamma;
		emit setMaximumSteps(3);
	} else {
		//original goes into resize
		qDebug("TMthread::original goes into resize");
		fetch("/original.pfs");
		status=from_resize;
		emit setMaximumSteps(4);
	}
	emit advanceCurrentProgress();
	lock.unlock();


	if (status==from_resize) {
		assert(opts.xsize!=originalxsize);
		qDebug("TMthread:: executing resize step");
		pfs::Frame *resized=resizeFrame(workingframe, opts.xsize);
		lock.lockForWrite();
		swap(resized,"/after_resize.pfs");
		xsize=opts.xsize;
		pregamma=-1;
		lock.unlock();
		delete workingframe;
		workingframe=resized;
		status=from_pregamma;
		emit advanceCurrentProgress();
	}
	if (status==from_pregamma) {
		qDebug("TMthread:: executing pregamma step");
		applyGammaFrame( workingframe, opts.pregamma );
		lock.lockForWrite();
		swap(workingframe,"/after_pregamma.pfs");
		pregamma=opts.pregamma;
		if (opts.xsize==originalxsize)
			xsize=-1;
		else
			xsize=opts.xsize;
		lock.unlock();
		status=from_tm;
		emit advanceCurrentProgress();
	}
	if (status==from_tm) {
		qDebug("TMthread:: executing tone mapping step");
		if (colorspaceconversion)
			workingframe->convertRGBChannelsToXYZ();
		pfs::Frame *result=NULL;
		switch (opts.tmoperator) {
		case mantiuk:
			result=pfstmo_mantiuk06(workingframe,
			opts.operator_options.mantiukoptions.contrastfactor,
			opts.operator_options.mantiukoptions.saturationfactor,
			opts.operator_options.mantiukoptions.contrastequalization);
		break;
		case fattal:
			//fattal is NOT even reentrant! (problem in PDE solving)
			//therefore I need to use a mutex here
			lock.lockForWrite();
			result=pfstmo_fattal02(workingframe,
			opts.operator_options.fattaloptions.alpha,
			opts.operator_options.fattaloptions.beta,
			opts.operator_options.fattaloptions.color,
			opts.operator_options.fattaloptions.noiseredux,
			opts.operator_options.fattaloptions.newfattal);
			lock.unlock();
		break;
		case ashikhmin:
			result=pfstmo_ashikhmin02(workingframe,
			opts.operator_options.ashikhminoptions.simple,
			opts.operator_options.ashikhminoptions.lct,
			opts.operator_options.ashikhminoptions.eq2 ? 2 : 4);
		break;
		case durand:
			//even durand seems to be not reentrant
			lock.lockForWrite();
			result=pfstmo_durand02(workingframe,
			opts.operator_options.durandoptions.spatial,
			opts.operator_options.durandoptions.range,
			opts.operator_options.durandoptions.base);
			lock.unlock();
		break;
		case drago:
			result=pfstmo_drago03(workingframe, opts.operator_options.dragooptions.bias);
		break;
		case pattanaik:
			result=pfstmo_pattanaik00(workingframe,
			opts.operator_options.pattanaikoptions.local,
			opts.operator_options.pattanaikoptions.multiplier,
			opts.operator_options.pattanaikoptions.cone,
			opts.operator_options.pattanaikoptions.rod,
			opts.operator_options.pattanaikoptions.autolum);
		break;
		case reinhard02:
			result=pfstmo_reinhard02(workingframe,
			opts.operator_options.reinhard02options.key,
			opts.operator_options.reinhard02options.phi,
			opts.operator_options.reinhard02options.range,
			opts.operator_options.reinhard02options.lower,
			opts.operator_options.reinhard02options.upper,
			opts.operator_options.reinhard02options.scales);
		break;
		case reinhard05:
			result=pfstmo_reinhard05(workingframe,
			opts.operator_options.reinhard05options.brightness,
			opts.operator_options.reinhard05options.chromaticAdaptation,
			opts.operator_options.reinhard05options.lightAdaptation);
		break;
		} //switch (opts.tmoperator)
		emit advanceCurrentProgress();
		assert(result!=NULL);
		delete workingframe;
		const QImage& res=fromLDRPFStoQImage(result);
		delete result;
		emit ImageComputed(res,&opts);
	} //if (status==from_tm)
// 	emit finished();
}

void TonemapperThread::terminateRequested() {
	if (forciblyTerminated)
		return;
	lock.lockForWrite();
	pregamma=-1;
	xsize=-1;
	lock.unlock();
	forciblyTerminated=true;
	//HACK oddly enough for the other operators we cannot emit finished (it segfaults)
	if (opts.tmoperator==mantiuk || opts.tmoperator==ashikhmin || opts.tmoperator==pattanaik )
		emit finished();
	terminate();
}

