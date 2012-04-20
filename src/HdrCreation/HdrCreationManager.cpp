/**
 * This file is a part of Luminance HDR package
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
 * 
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ---------------------------------------------------------------------- 
 *
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <QDebug>

#include <QApplication>
#include <QFileInfo>

#include "Libpfs/domio.h"
#include "Fileformat/pfstiff.h"
#include "Fileformat/pfsouthdrimage.h"
#include "Filter/pfscut.h"
#include "Exif/ExifOperations.h"
#include "Threads/HdrInputLoader.h"
#include "mtb_alignment.h"
#include "HdrCreationManager.h"
#include "arch/math.h"

pfs::Array2D *shiftPfsArray2D(pfs::Array2D *in, int dx, int dy)
{
	int width = in->getCols();
	int height = in->getRows();

	pfs::Array2D *temp = new pfs::Array2D(width, height);	
	pfs::Array2D *out = new pfs::Array2D(width, height);	
	
	for (int j = 0; j < height; j++) 
        for (int i = 0; i < width; i++) 
			(*temp)(i, j) = 0;

	// x-shift
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			if ((i+dx) < 0)
				continue;
			if ((i+dx) >= width)
				break;
			if ((*in)(i+dx, j) > 65535)
				(*temp)(i, j) = 65535;
			else if ((*in)(i+dx, j) < 0)
				(*temp)(i, j) = 0;
			else
				(*temp)(i, j) = (*in)(i+dx, j);
		}
	}
	// y-shift
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			if ((j+dy) < 0)
				continue;
			if ((j+dy) >= height)
				break;
			if ((*temp)(i, j+dy) > 65535)
				(*out)(i, j) = 65535;
			else if ((*temp)(i, j+dy) < 0)
				(*out)(i, j) = 0;
			else
				(*out)(i, j) = (*temp)(i, j+dy);
		}
	}
	return out;
}

HdrCreationManager::HdrCreationManager(bool fromCommandLine): m_shift(0), fromCommandLine(fromCommandLine) {
	ais = NULL;
	chosen_config = predef_confs[0];
	inputType = UNKNOWN_INPUT_TYPE;
}

void HdrCreationManager::setConfig(config_triple &c) {
	chosen_config = c;
}

void HdrCreationManager::setFileList(QStringList &l) {
	processedFiles = m_shift;
	runningThreads = 0;
	loadingError = false;

	fileList.append(l);

	expotimes.resize(fileList.count());
	filesToRemove.resize(fileList.count());

	//add default values
	for (int i = 0; i < l.count(); i++) {
		//time equivalents of EV values
		expotimes[m_shift + i] = -1;
		//i-th==true means we started a thread to load the i-th file
		startedProcessing.append(false);
		//tiffLdrList contains false by default
		tiffLdrList.append(false);
		//ldr payloads
		ldrImagesList.append(NULL);
		//mdr payloads
		listmdrR.push_back(NULL);
		listmdrG.push_back(NULL);
		listmdrB.push_back(NULL);
	}
}

void HdrCreationManager::loadInputFiles() {
	//find first not started processing.
	int firstNotStarted = -1;
	for (int i = 0; i < fileList.size(); i++) {
		if (!startedProcessing.at(i)) {
			firstNotStarted = i;
			//qDebug("HCM: loadInputFiles: found not startedProcessing: %d",i);
			break;
		}
	}

	//we can end up in this function "conditionalLoadInput" many times, called in a queued way by newResult(...).
	if (firstNotStarted == -1) {
		if (processedFiles == fileList.size()) { //then it's really over
			if (filesLackingExif.size() == 0) {
				//give an offset to the EV values if they are outside of the -10..10 range.
				checkEVvalues();
			}
			emit finishedLoadingInputFiles(filesLackingExif);
		} //all files loaded
		
		//return when list is over but some threads are still running.
		return;
	} //if all files already started processing
	else { //if we still have to start processing some file
		while (runningThreads < m_luminance_options.getNumThreads() && firstNotStarted < startedProcessing.size()) {
			//qDebug("HCM: Creating loadinput thread on %s",qPrintable(fileList[firstNotStarted]));
			startedProcessing[firstNotStarted] = true;
			HdrInputLoader *thread = new HdrInputLoader(fileList[firstNotStarted],firstNotStarted);
			if (thread == NULL)
				exit(1); // TODO: show an error message and exit gracefully
			connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
			connect(thread, SIGNAL(loadFailed(QString,int)), this, SLOT(loadFailed(QString,int)));
			connect(thread, SIGNAL(ldrReady(QImage *, int, float, QString, bool)), this, SLOT(ldrReady(QImage *, int, float, QString, bool)));
			connect(thread, SIGNAL(mdrReady(pfs::Frame *, int, float, QString)), this, SLOT(mdrReady(pfs::Frame *, int, float, QString)));
			connect(thread, SIGNAL(maximumValue(int)), this, SIGNAL(maximumValue(int)));
			connect(thread, SIGNAL(nextstep(int)), this, SIGNAL(nextstep(int)));
			thread->start();
			firstNotStarted++;
			runningThreads++;
		}
	}
}

void HdrCreationManager::loadFailed(QString message, int /*index*/) {
	//check for correct image size: update list that will be sent once all is over.
	//qDebug("HCM: failed loading file: %s.", qPrintable(message));
	loadingError = true;
	emit errorWhileLoading(message);
}

void HdrCreationManager::mdrReady(pfs::Frame *newFrame, int index, float expotime, QString newfname) {
	if (loadingError) {
		//qDebug("HCM: loadingError, bailing out.");
		emit processed();
		return;
	}
	//newFrame is in CS_RGB but channel names remained X Y Z
	pfs::Channel *R, *G, *B;
	newFrame->getXYZChannels( R, G, B);

	if (inputType == LDR_INPUT_TYPE) {
		//qDebug("HCM: wrong format, bailing out.");
		loadingError=true;
		emit errorWhileLoading(tr("The image %1 is an 8 bit format (LDR) while the previous ones are not.").arg(newfname));
		return;
	}
	inputType = MDR_INPUT_TYPE;
	if (!mdrsHaveSameSize(R->getWidth(),R->getHeight())) {
		//qDebug("HCM: wrong size, bailing out.");
		loadingError = true;
		emit errorWhileLoading(tr("The image %1 has an invalid size.").arg(newfname));
		return;
	}
	if (!fromCommandLine)
		mdrImagesList.append(fromHDRPFStoQImage(newFrame));
	m_mdrWidth = R->getWidth();
	m_mdrHeight = R->getHeight();
	// fill with image data
	listmdrR[index] = R->getChannelData();
	listmdrG[index] = G->getChannelData();
	listmdrB[index] = B->getChannelData();
	//perform some housekeeping
		//pfs::DOMIO pfsio;
	//pfsio.freeFrame(newFrame);
	newResult(index,expotime,newfname);
	//continue with the loading process
	loadInputFiles();
}

void HdrCreationManager::ldrReady(QImage *newImage, int index, float expotime, QString newfname, bool ldrtiff) {
	//qDebug("HCM: ldrReady");
	if (loadingError) {
		//qDebug("HCM: loadingError, bailing out.");
		emit processed();
		return;
	}
	if (inputType==MDR_INPUT_TYPE) {
		//qDebug("HCM: wrong format, bailing out.");
		loadingError = true;
		emit errorWhileLoading(tr("The image %1 is an 16 bit format while the previous ones are not.").arg(newfname));
		return;
	}
	inputType=LDR_INPUT_TYPE;
	if (!ldrsHaveSameSize(newImage->width(),newImage->height())) {
		//qDebug("HCM: wrong size, bailing out.");
		loadingError = true;
		emit errorWhileLoading(tr("The image %1 has an invalid size.").arg(newfname));
		return;
	}

	// fill with image data
	ldrImagesList[index] = newImage;
	//check if ldr tiff
	if (ldrtiff)
		tiffLdrList[index] = true;
	//perform some housekeeping
	newResult(index,expotime,newfname);
	//continue with the loading process
	loadInputFiles();
}

void HdrCreationManager::newResult(int index, float expotime, QString newfname) {
	runningThreads--;
	processedFiles++;

	//update filesToRemove
	if ( fileList.at(index) != newfname ) {
		qDebug() << "Files to remove " << index << " " << newfname;
		filesToRemove[index] = newfname;
	}

	//update expotimes[i]
	expotimes[index] = expotime;

	QFileInfo qfi(fileList[index]);
	//check for invalid exif: update list that will be sent once all is over.
	if (expotimes[index] == -1) {
		filesLackingExif << "<li>"+qfi.fileName()+"</li>";
		//qDebug("HCM: new invalid exif. elements: %d", filesLackingExif.size());
	}

	emit fileLoaded(index,fileList[index],expotimes[index]);
	emit processed();
}

bool HdrCreationManager::ldrsHaveSameSize(int currentWidth, int currentHeight) {
	for (int i = 0; i < ldrImagesList.size(); i++) {
		const QImage* imagepointer = ldrImagesList.at(i);
		if (imagepointer != NULL) {
			if (imagepointer->width() != currentWidth || imagepointer->height() != currentHeight) {
				return false;
			}
		}
	}
	return true;
}

bool HdrCreationManager::mdrsHaveSameSize(int currentWidth, int currentHeight) {
	for (unsigned int i = 0; i < listmdrR.size(); i++) {
		const pfs::Array2D* Rpointer = listmdrR.at(i);
		const pfs::Array2D* Gpointer = listmdrG.at(i);
		const pfs::Array2D* Bpointer = listmdrB.at(i);
		if (Rpointer != NULL && Gpointer != NULL && Bpointer != NULL) {
			if (Rpointer->getCols() != currentWidth || Rpointer->getRows() != currentHeight
			 || Gpointer->getCols() != currentWidth || Gpointer->getRows() != currentHeight
			 || Bpointer->getCols() != currentWidth || Bpointer->getRows() != currentHeight ) {
				return false;
			}
		}
	}
	return true;
}

void HdrCreationManager::align_with_mtb() {
	mtb_alignment(ldrImagesList,tiffLdrList);
	emit finishedAligning(0);
}

void HdrCreationManager::align_with_ais() {
	ais = new QProcess(this);
	if (ais == NULL) //TODO: exit gracefully
		exit(1);
	if (!fromCommandLine)
		ais->setWorkingDirectory(m_luminance_options.getTempDir());
	QStringList env = QProcess::systemEnvironment();
	#ifdef WIN32
	QString separator(";");
	#else
	QString separator(":");
	#endif
	env.replaceInStrings(QRegExp("^PATH=(.*)", Qt::CaseInsensitive), "PATH=\\1"+separator+QCoreApplication::applicationDirPath());
	ais->setEnvironment(env);
	connect(ais, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(ais_finished(int,QProcess::ExitStatus)));
	connect(ais, SIGNAL(error(QProcess::ProcessError)), this, SIGNAL(ais_failed(QProcess::ProcessError)));
	connect(ais, SIGNAL(error(QProcess::ProcessError)), this, SLOT(ais_failed_slot(QProcess::ProcessError)));
	connect(ais, SIGNAL(readyRead()), this, SLOT(readData()));
	
	QStringList ais_parameters = m_luminance_options.getAlignImageStackOptions();
	if (filesToRemove[0] == "") {
		ais_parameters << fileList;
	}
	else {
		foreach(QString fname, filesToRemove) 
			ais_parameters << fname;	
	}
	qDebug() << "ais_parameters " << ais_parameters;
	#ifdef Q_WS_MAC
	ais->start(QCoreApplication::applicationDirPath()+"/align_image_stack", ais_parameters );
	#else
	ais->start("align_image_stack", ais_parameters );
	#endif
	qDebug() << "ais started";
}

void HdrCreationManager::ais_finished(int exitcode, QProcess::ExitStatus exitstatus) {
	if (exitstatus != QProcess::NormalExit) {
		qDebug() << "ais failed";
		//emit ais_failed(QProcess::Crashed);
		return;
	}
	if (exitcode == 0) {
		//TODO: try-catch 
		//qDebug("HCM: align_image_stack successfully terminated");
		clearlists(false);
		for (int i = 0; i < fileList.size(); i++) {
			//align_image_stack can only output tiff files
			QString filename;
			if (!fromCommandLine)
				filename = QString(m_luminance_options.getTempDir() + "/aligned_" + QString("%1").arg(i,4,10,QChar('0'))+".tif");
			else
				filename = QString("aligned_" + QString("%1").arg(i,4,10,QChar('0'))+".tif");
			QByteArray fname = QFile::encodeName(filename);
			//qDebug("HCM: Loading back file name=%s", fname);
			TiffReader reader(fname, "", false);
			//if 8bit ldr tiff
			if (reader.is8bitTiff()) {
				QImage* resultImage = reader.readIntoQImage();
				QImage* oldImage = resultImage;
				HdrInputLoader::conditionallyRotateImage(QFileInfo(fileList[0]), &resultImage);

				ldrImagesList.append( resultImage );
				tiffLdrList.append(oldImage == resultImage);
			}
			//if 16bit (tiff) treat as hdr
			else if (reader.is16bitTiff()) {
				//TODO: get a 16bit TIFF image and test it
				pfs::Frame *newFrame=reader.readIntoPfsFrame();
				pfs::Channel *R, *G, *B;
				R = newFrame->getChannel("X");
				G = newFrame->getChannel("Y");
				B = newFrame->getChannel("Z");
				listmdrR.push_back(R->getChannelData());
				listmdrG.push_back(G->getChannelData());
				listmdrB.push_back(B->getChannelData());
					//pfs::DOMIO pfsio;
				//pfsio.freeFrame(newFrame);
			}
			qDebug() << "void HdrCreationManager::ais_finished: remove " << fname;
			QFile::remove(fname);
		}
		QFile::remove(m_luminance_options.getTempDir() + "/hugin_debug_optim_results.txt");
		emit finishedAligning(exitcode);
	}
	else {
		qDebug() << "align_image_stack exited with exit code " << exitcode;
		emit finishedAligning(exitcode);
	}
}

void HdrCreationManager::ais_failed_slot(QProcess::ProcessError error)
{
	qDebug() <<	"align_image_stack failed";
}

void HdrCreationManager::removeTempFiles()
{
	foreach (QString tempfname, filesToRemove)
	{
		qDebug() << "void HdrCreationManager::removeTempFiles(): " << qPrintable(tempfname);
		if (!tempfname.isEmpty())
		{
			QFile::remove(tempfname);
		}
	}
	filesToRemove.clear();
}

void HdrCreationManager::checkEVvalues() {
	//qDebug("HCM::checkEVvalues");
	float max=-20, min=+20;
	for (int i = 0; i < fileList.size(); i++) {
		float ev_val = log2f(expotimes[i]);
		if (ev_val > max)
			max = ev_val;
		if (ev_val < min)
			min = ev_val;
	}
	//now if values are out of bounds, add an offset to them.
	if (max > 10) {
		for (int i = 0; i < fileList.size(); i++) {
			float new_ev = log2f(expotimes[i]) - (max - 10);
			expotimes[i] = exp2f(new_ev);
			emit expotimeValueChanged(exp2f(new_ev), i);
		}
	} else if (min < -10) {
		for (int i = 0; i < fileList.size(); i++) {
			float new_ev = log2f(expotimes[i]) - (min + 10);
			expotimes[i] = exp2f(new_ev);
			emit expotimeValueChanged(exp2f(new_ev), i);
		}
	}
	//qDebug("HCM::END checkEVvalues");
}

void HdrCreationManager::setEV(float new_ev, int image_idx) {
	//qDebug("HCM::setEV image_idx=%d",image_idx);
	if (expotimes[image_idx] == -1) {
		//remove always the first one
		//after the initial printing we only need to know the size of the list
		filesLackingExif.removeAt(0);
		//qDebug("HCM: fixed a expotime");
	}
	//qDebug("HCM: setting expotimes[%d]=%f EV=%f", image_idx, exp2f(new_ev), new_ev);
	expotimes[image_idx] = exp2f(new_ev);
	emit expotimeValueChanged(exp2f(new_ev), image_idx);
}

pfs::Frame* HdrCreationManager::createHdr(bool ag, int iterations) {
	//CREATE THE HDR
	if (inputType == LDR_INPUT_TYPE)
		return createHDR(expotimes.data(), &chosen_config, ag, iterations, true, &ldrImagesList );
	else
		return createHDR(expotimes.data(), &chosen_config, ag, iterations, false, &listmdrR, &listmdrG, &listmdrB );
}

HdrCreationManager::~HdrCreationManager() {
	if (ais != NULL && ais->state() != QProcess::NotRunning) {
		ais->kill();
	}
	clearlists(true);
}

void HdrCreationManager::clearlists(bool deleteExpotimeAsWell) {
	startedProcessing.clear();
	filesLackingExif.clear();

	if (deleteExpotimeAsWell) {
		fileList.clear();
		expotimes.clear();
	}
	if (ldrImagesList.size() != 0) {
		//qDebug("HCM: clearlists: cleaning LDR exposures list");
		for(int i = 0 ; i < ldrImagesList.size(); i++) {
			if (tiffLdrList[i]) {
				//qDebug("HCM: clearlists: freeing ldr tiffs' payload.");
				delete [] ldrImagesList[i]->bits();
			}
		}
		qDeleteAll(ldrImagesList);
		ldrImagesList.clear();
		tiffLdrList.clear();
	}
	if (listmdrR.size()!=0 && listmdrG.size()!=0 && listmdrB.size()!=0) {
		//qDebug("HCM: cleaning HDR exposures list");
		Array2DList::iterator itR=listmdrR.begin(), itG=listmdrG.begin(), itB=listmdrB.begin();
		for (; itR!=listmdrR.end(); itR++,itG++,itB++ ){
			delete *itR; delete *itG; delete *itB;
		}
		listmdrR.clear(); listmdrG.clear(); listmdrB.clear();
		qDeleteAll(mdrImagesList);
		mdrImagesList.clear();
		qDeleteAll(mdrImagesToRemove);
		mdrImagesToRemove.clear();
	}
}

void HdrCreationManager::makeSureLDRsHaveAlpha() {
	if (ldrImagesList.at(0)->format()==QImage::Format_RGB32) {
		int origlistsize = ldrImagesList.size();
		for (int image_idx = 0; image_idx < origlistsize; image_idx++) {
			QImage *newimage = new QImage(ldrImagesList.at(0)->convertToFormat(QImage::Format_ARGB32));
			if (newimage == NULL)
				exit(1); // TODO: exit gracefully;
			ldrImagesList.append(newimage);
			if (tiffLdrList[0]) {
				delete [] ldrImagesList[0]->bits();
			}
			delete ldrImagesList.takeAt(0);
			tiffLdrList.removeAt(0);
			tiffLdrList.append(false);
		}
	}
}

void HdrCreationManager::applyShiftsToImageStack(QList< QPair<int,int> > HV_offsets) {
	int originalsize = ldrImagesList.count();
	//shift the images
	for (int i = 0; i < originalsize; i++) {
		//qDebug("%d,%d",HV_offsets[i].first,HV_offsets[i].second);
		if (HV_offsets[i].first == HV_offsets[i].second && HV_offsets[i].first == 0)
			continue;
		//qDebug("shifting image %d of (%d,%d)",i, HV_offsets[i].first, HV_offsets[i].second);
		QImage *shifted = shiftQImage(ldrImagesList[i], HV_offsets[i].first, HV_offsets[i].second);
		if (tiffLdrList[i]) {
			delete [] ldrImagesList[i]->bits();
		}
		delete ldrImagesList.takeAt(i);
		ldrImagesList.insert(i, shifted);
		tiffLdrList.removeAt(i);
		tiffLdrList.insert(i,false);
	}
}

void HdrCreationManager::applyShiftsToMdrImageStack(QList< QPair<int,int> > HV_offsets) {
	qDebug() << "HdrCreationManager::applyShiftsToMdrImageStack";
	int originalsize = mdrImagesList.count();
	for (int i = 0; i < originalsize; i++) {
		if (HV_offsets[i].first == HV_offsets[i].second && HV_offsets[i].first == 0)
			continue;
		pfs::Array2D *shiftedR = shiftPfsArray2D(listmdrR[i], HV_offsets[i].first, HV_offsets[i].second);
		pfs::Array2D *shiftedG = shiftPfsArray2D(listmdrG[i], HV_offsets[i].first, HV_offsets[i].second);
		pfs::Array2D *shiftedB = shiftPfsArray2D(listmdrB[i], HV_offsets[i].first, HV_offsets[i].second);
		delete listmdrR[i];
		delete listmdrG[i];
		delete listmdrB[i];
		listmdrR[i] = shiftedR;
		listmdrG[i] = shiftedG;
		listmdrB[i] = shiftedB;
	}
}


void HdrCreationManager::cropLDR (QRect ca) {
	//qDebug("cropping left,top=(%d,%d) %dx%d",ca.left(),ca.top(),ca.width(),ca.height());
	//crop all the images
	int origlistsize = ldrImagesList.size();
	for (int image_idx = 0; image_idx < origlistsize; image_idx++) {
		QImage *newimage = new QImage(ldrImagesList.at(0)->copy(ca));
		if (newimage == NULL)
			exit(1); // TODO: exit gracefully
		ldrImagesList.append(newimage);
		if (tiffLdrList[0])
			delete [] ldrImagesList[0]->bits();
		delete ldrImagesList.takeAt(0);
		tiffLdrList.removeAt(0);
		tiffLdrList.append(false);
	}
}

void HdrCreationManager::cropMDR (QRect ca) {
	//qDebug("cropping left,top=(%d,%d) %dx%d",ca.left(),ca.top(),ca.width(),ca.height());
	//crop all the images
	pfs::DOMIO pfsio;
	int origlistsize = listmdrR.size();
	for (int idx = 0; idx < origlistsize; idx++) {
		pfs::Frame *frame = pfsio.createFrame( m_mdrWidth, m_mdrHeight );
		pfs::Channel *Xc, *Yc, *Zc;
		frame->createXYZChannels( Xc, Yc, Zc );
		Xc->setChannelData(listmdrR[idx]);	
		Yc->setChannelData(listmdrG[idx]);	
		Zc->setChannelData(listmdrB[idx]);	
		int x_ul, y_ul, x_br, y_br;
		ca.getCoords(&x_ul, &y_ul, &x_br, &y_br);
		pfs::Frame *cropped_frame = pfs::pfscut(frame, x_ul, y_ul, x_br, y_br);
		pfsio.freeFrame(frame);
		pfs::Channel *R, *G, *B;
		cropped_frame->getXYZChannels( R, G, B);
		listmdrR[idx] = R->getChannelData();
		listmdrG[idx] = G->getChannelData();
		listmdrB[idx] = B->getChannelData();
		QImage *newimage = new QImage(mdrImagesList.at(0)->copy(ca));
		if (newimage == NULL)
			exit(1); // TODO: exit gracefully
		mdrImagesList.append(newimage);
		mdrImagesToRemove.append(mdrImagesList.takeAt(0));
	}
}

void HdrCreationManager::reset() {
	ais = NULL;
	m_shift = 0;
	chosen_config = predef_confs[0];
	inputType = UNKNOWN_INPUT_TYPE;
	fileList.clear();
	clearlists(true);
	removeTempFiles();
}

void HdrCreationManager::remove(int index) {
	switch (inputType) {
	case LDR_INPUT_TYPE:
	{
		if (tiffLdrList[index]) {
			delete [] ldrImagesList[index]->bits();
		}				
		ldrImagesList.removeAt(index);			
		tiffLdrList.removeAt(index);
	}
		break;
	case MDR_INPUT_TYPE:
	{
			Array2DList::iterator itR = listmdrR.begin() + index;
			delete *itR;
			listmdrR.erase(itR);

			Array2DList::iterator itG = listmdrG.begin() + index;
			delete *itG;
			listmdrG.erase(itG);

			Array2DList::iterator itB = listmdrB.begin() + index;
			delete *itB;
			listmdrB.erase(itB);
			
			delete mdrImagesList[index];			
			mdrImagesList.removeAt(index);			
			
			QString fname = filesToRemove.at(index);
			qDebug() << "void HdrCreationManager::remove(int index): filename " << fname;
			QFile::remove(fname);
	}
		break;
		// ...in this case, do nothing!
	case UNKNOWN_INPUT_TYPE:
	default:{}
		break;
	}
	fileList.removeAt(index);
	filesToRemove.remove(index);
	expotimes.remove(index);
	startedProcessing.removeAt(index);
}

void HdrCreationManager::readData()
{
	QByteArray data = ais->readAll();
	emit aisDataReady(data);
}

void HdrCreationManager::saveMDRs(QString filename)
{
	qDebug() << "HdrCreationManager::saveMDRs";
	pfs::DOMIO pfsio;
	int origlistsize = listmdrR.size();
	for (int idx = 0; idx < origlistsize; idx++) {
		QString fname = filename + QString("_%1").arg(idx) + ".tiff";
		pfs::Frame *frame = pfsio.createFrame( m_mdrWidth, m_mdrHeight );
		pfs::Channel *Xc, *Yc, *Zc;
		frame->createXYZChannels( Xc, Yc, Zc );
		Xc->setChannelData(listmdrR[idx]);	
		Yc->setChannelData(listmdrG[idx]);	
		Zc->setChannelData(listmdrB[idx]);	
		TiffWriter writer(fname.toLatin1().constData(), frame);
		writer.writePFSFrame16bitTiff();

		QFileInfo qfi(filename);
		QString absoluteFileName = qfi.absoluteFilePath();
		QByteArray encodedName = QFile::encodeName(absoluteFileName + QString("_%1").arg(idx) + ".tiff");
		ExifOperations::writeExifData(encodedName.constData(), "", expotimes[idx]);	
	}
	emit mdrSaved();
}
