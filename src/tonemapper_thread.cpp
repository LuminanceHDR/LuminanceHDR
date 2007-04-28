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

#include "tonemapper_thread.h"
#include <QProgressBar>
pfs::Frame* resizeFrame(pfs::Frame* inpfsframe, int _xSize);
void applyGammaFrame( pfs::Frame*, const float);
pfs::Frame* pfstmo_ashikhmin02 (pfs::Frame*,bool,float,int);
pfs::Frame* pfstmo_drago03 (pfs::Frame *, float);
pfs::Frame* pfstmo_fattal02 (pfs::Frame*,float,float,float);
pfs::Frame* pfstmo_durand02 (pfs::Frame*,float,float,float);
pfs::Frame* pfstmo_pattanaik00 (pfs::Frame*,bool,float,float,float,bool);
pfs::Frame* pfstmo_reinhard02 (pfs::Frame*,float,float,int,int,int,bool);
pfs::Frame* pfstmo_reinhard04 (pfs::Frame *,float,float);
QImage* fromLDRPFStoQImage(pfs::Frame*);

// width of the pfs:frame written on disk during resize operation
int xsize=-1; //-1 means nothing has been computed yet
// gamma of the pfs:frame written on disk after resize
float pregamma=-1; //-1 means NOT VALID (size has changed/is changing)<
//pregamma!=-1 means that the pregamma frame has the xsize as width
QReadWriteLock lock;


TonemapperThread::TonemapperThread(int xorigsize, QString cachepath, QProgressBar* b) : QThread(0), bar(b), originalxsize(xorigsize), cachepath(cachepath){
	colorspaceconversion=false;
}

TonemapperThread::~TonemapperThread() {
// 	qDebug("~TonemapperThread()");
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
// 	dumpOpts();
// 	qDebug("::::::begin thread, size=%d, gamma=%f",xsize, pregamma);
	lock.lockForRead();
	if (opt.xsize==originalxsize && opt.pregamma==1.0f) {
		//original goes into tone mapping
		qDebug("::::::original goes into tone mapping");
		fetch("/original.pfs");
		status=from_tm;
		colorspaceconversion=true;
		bar->setMaximum(2);
		bar->setValue(1);
	} else if (opt.xsize==xsize && opt.pregamma==1.0f) {
		//resized goes into tone mapping
		qDebug("::::::resized goes into tone mapping");
		fetch("/after_resize.pfs");
		status=from_tm;
		colorspaceconversion=true;
		bar->setMaximum(2);
		bar->setValue(1);
	} else if ( (opt.xsize==xsize && opt.pregamma==pregamma) || (opt.xsize==originalxsize && opt.pregamma==pregamma) ) {
		//after_pregamma goes into tone mapping
		qDebug("::::::after_pregamma goes into tone mapping");
		fetch("/after_pregamma.pfs");
		status=from_tm;
		bar->setMaximum(2);
		bar->setValue(1);
	} else if (opt.xsize==xsize) {
		//resized goes into pregamma
		qDebug("::::::resized goes into pregamma");
		fetch("/after_resize.pfs");
		status=from_pregamma;
		bar->setMaximum(3);
		bar->setValue(1);
	} else if (opt.xsize==originalxsize) {
		//original goes into pregamma
		qDebug("::::::original goes into pregamma");
		fetch("/original.pfs");
		status=from_pregamma;
		bar->setMaximum(3);
		bar->setValue(1);
	} else {
		//original goes into resize
		qDebug("::::::original goes into resize");
		fetch("/original.pfs");
		status=from_resize;
		bar->setMaximum(4);
		bar->setValue(1);
	}
	lock.unlock();


	if (status==from_resize) {
		qDebug("executing resize step");
		pfs::Frame *resized=resizeFrame(workingframe, opt.xsize);
		lock.lockForWrite();
		swap(resized,"/after_resize.pfs");
		xsize=opt.xsize;
		pregamma=-1;
		lock.unlock();
		delete workingframe;
		workingframe=resized;
		status=from_pregamma;
		bar->setValue(bar->value()+1);
	}
	if (status==from_pregamma) {
		qDebug("executing pregamma step");
		applyGammaFrame( workingframe, opt.pregamma );
		lock.lockForWrite();
		swap(workingframe,"/after_pregamma.pfs");
		pregamma=opt.pregamma;
		if (opt.xsize==originalxsize)
			xsize=-1;
		else
			xsize=opt.xsize;
		lock.unlock();
		status=from_tm;
		bar->setValue(bar->value()+1);
	}
	if (status==from_tm) {
		qDebug("executing tone mapping step");
		if (colorspaceconversion)
			workingframe->convertRGBChannelsToXYZ();
		pfs::Frame *result=NULL;
		switch (opt.tmoperator) {
		case fattal:
			//fattal is NOT even reentrant! (problem in PDE solving)
			//therefore I need to use a mutex here
			lock.lockForWrite();
			result=pfstmo_fattal02(workingframe,
			opt.operator_options.fattaloptions.alpha,
			opt.operator_options.fattaloptions.beta,
			opt.operator_options.fattaloptions.color);
			lock.unlock();
		break;
		case ashikhmin:
			result=pfstmo_ashikhmin02(workingframe,
			opt.operator_options.ashikhminoptions.simple,
			opt.operator_options.ashikhminoptions.lct,
			opt.operator_options.ashikhminoptions.eq2 ? 2 : 4);
		break;
		case durand:
			//even durand seems to be not reentrant
			lock.lockForWrite();
			result=pfstmo_durand02(workingframe,
			opt.operator_options.durandoptions.spatial,
			opt.operator_options.durandoptions.range,
			opt.operator_options.durandoptions.base);
			lock.unlock();
		break;
		case drago:
			result=pfstmo_drago03(workingframe, opt.operator_options.dragooptions.bias);
		break;
		case pattanaik:
			result=pfstmo_pattanaik00(workingframe,
			opt.operator_options.pattanaikoptions.local,
			opt.operator_options.pattanaikoptions.multiplier,
			opt.operator_options.pattanaikoptions.cone,
			opt.operator_options.pattanaikoptions.rod,
			opt.operator_options.pattanaikoptions.autolum);
		break;
		case reinhard02:
			result=pfstmo_reinhard02(workingframe,
			opt.operator_options.reinhard02options.key,
			opt.operator_options.reinhard02options.phi,
			opt.operator_options.reinhard02options.range,
			opt.operator_options.reinhard02options.lower,
			opt.operator_options.reinhard02options.upper,
			opt.operator_options.reinhard02options.scales);
		break;
		case reinhard04:
			result=pfstmo_reinhard04(workingframe,
			opt.operator_options.reinhard04options.brightness,
			opt.operator_options.reinhard04options.saturation);
		break;
		}
	bar->setValue(bar->value()+1);
	assert(result!=NULL);
	delete workingframe;
	QImage* res=fromLDRPFStoQImage(result);
	delete result;
	emit ImageComputed(res,&opt);
	emit removeProgressBar(bar);
	}
}

#if 0
void TonemapperThread::dumpOpts() {
	qDebug("dump: opt.xsize=%d",opt.xsize);
	qDebug("dump: opt.pregamma=%f",opt.pregamma);
	switch (opt.tmoperator) {
	case fattal:
		qDebug("dump: fattal");
		qDebug("dump: opt.alpha=%f",opt.operator_options.fattaloptions.alpha);
		qDebug("dump: opt.beta=%f",opt.operator_options.fattaloptions.beta);
		qDebug("dump: opt.color=%f",opt.operator_options.fattaloptions.color);
	break;
	case ashikhmin:
		qDebug("dump: ashikhmin");
		qDebug(opt.operator_options.ashikhminoptions.simple? "dump: opt.simple=true" : "dump: opt.simple=false");
		qDebug("dump: opt.lct=%f",opt.operator_options.ashikhminoptions.lct);
		qDebug("dump: opt.eq=%d",
 opt.operator_options.ashikhminoptions.eq2 ? 2 : 4);
	break;
	case durand:
		qDebug("dump: durand");
		qDebug("dump: opt.spatial=%f",opt.operator_options.durandoptions.spatial);
		qDebug("dump: opt.range=%f",opt.operator_options.durandoptions.range);
		qDebug("dump: opt.base=%f",opt.operator_options.durandoptions.base);
	break;
	case drago:
		qDebug("dump: drago");
		qDebug("dump: opt.bias=%f",opt.operator_options.dragooptions.bias);
	break;
	case pattanaik:
		qDebug("dump: pattanaik");
		qDebug(opt.operator_options.pattanaikoptions.local?"dump: opt.local=true":"dump: opt.local=false");
		qDebug(opt.operator_options.pattanaikoptions.autolum?"dump: opt.autolum=true":"dump: opt.autolum=false");
		qDebug("dump: opt.multiplier=%f",opt.operator_options.pattanaikoptions.multiplier);
		qDebug("dump: opt.cone=%f",opt.operator_options.pattanaikoptions.cone);
		qDebug("dump: opt.rod=%f",opt.operator_options.pattanaikoptions.rod);
	break;
	case reinhard02:
		qDebug("dump: reinhard02");
		qDebug(opt.operator_options.reinhard02options.scales?"dump: opt.scales=true":"dump: opt.scales=false");
		qDebug("dump: opt.key=%f",opt.operator_options.reinhard02options.key);
		qDebug("dump: opt.phi=%f",opt.operator_options.reinhard02options.phi);
		qDebug("dump: opt.range=%d",opt.operator_options.reinhard02options.range);
		qDebug("dump: opt.lower=%d",opt.operator_options.reinhard02options.lower);
		qDebug("dump: opt.upper=%d",opt.operator_options.reinhard02options.upper);
	break;
	case reinhard04:
		qDebug("dump: reinhard04");
		qDebug("dump: opt.brightness=%f",opt.operator_options.reinhard04options.brightness);
		qDebug("dump: opt.saturation=%f",opt.operator_options.reinhard04options.saturation);
	break;
	}
}
#endif

void TonemapperThread::ComputeImage(const tonemapping_options opt) {
	this->opt=opt;
	if (!isRunning()) {
#ifdef _WIN32
		start();
#else
		start(HighestPriority);
#endif
	}
}
