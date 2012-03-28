/**
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

#include <iostream>
#include <QDebug>
#include <QScopedPointer>
#include <QString>
#include <QByteArray>
#include <QMessageBox>

#include "Viewers/LdrViewer.h"
#include "Viewers/IGraphicsPixmapItem.h"
#include "Core/TonemappingOptions.h"
#include "Libpfs/frame.h"
#include "Fileformat/pfsoutldrimage.h"
#include "Common/LuminanceOptions.h"


namespace
{
void parseOptions(const TonemappingOptions *opts, QString& caption)
{
    if (opts == NULL)
    {
        caption.clear();
    }
    else
    {
        TMOptionsOperations tmopts(opts);
        //postfix = tmopts.getPostfix();
        caption = tmopts.getCaption();
        //exif_comment = tmopts.getExifComment();
    }
}
}


LdrViewer::LdrViewer(pfs::Frame* frame, TonemappingOptions* opts, QWidget *parent, bool ns):
    GenericViewer(frame, parent, ns),
    informativeLabel(new QLabel( mToolBar)),
    mTonemappingOptions(opts)
{
    mToolBar->addWidget(informativeLabel);

    mPixmap->disableSelectionTool(); // disable by default crop functionalities

    // I shouldn't call a virtual function straight from the constructor,
    // but specifing correctly which version of this virtual function I want to call,
    // I am safe
    LdrViewer::setTonemappingOptions(opts);

	LuminanceOptions luminance_opts; 

	storedPixmap = new IGraphicsPixmapItem();

    //QScopedPointer<QImage> temp_qimage(fromLDRPFStoQImage(getFrame()));
	QImage *temp_qimage(fromLDRPFStoQImage(getFrame()));	

	QImage *xformed_qimage = doCMSTransform(temp_qimage, false, false);

	if (xformed_qimage == NULL)
		setQImage(*temp_qimage);
	else {
		setQImage(*xformed_qimage);
		delete xformed_qimage;
	}

    updateView();

    retranslateUi();
	
	delete temp_qimage;
}

LdrViewer::~LdrViewer()
{
#ifdef QT_DEBUG
    std::cout << "LdrViewer::~LdrViewer()" << std::endl;
#endif
    delete storedPixmap;
}

void LdrViewer::retranslateUi()
{
    informativeLabel->setText( tr("LDR image [%1 x %2]").arg(getWidth()).arg(getHeight()) );

	GenericViewer::retranslateUi();
}

QString LdrViewer::getFileNamePostFix()
{
    if ( mTonemappingOptions )
    {
        TMOptionsOperations tm_ops(mTonemappingOptions);
        return tm_ops.getPostfix();
    } else
        return QString();
}

QString LdrViewer::getExifComment()
{
    if ( mTonemappingOptions )
    {
        TMOptionsOperations tm_ops(mTonemappingOptions);
        return tm_ops.getExifComment();
    } else
        return QString();
}

void LdrViewer::updatePixmap()
{
#ifdef QT_DEBUG
    qDebug() << "void LdrViewer::updatePixmap()";
#endif

    //QScopedPointer<QImage> temp_qimage(fromLDRPFStoQImage(getFrame()));
	QImage *temp_qimage(fromLDRPFStoQImage(getFrame()));

	QImage *xformed_qimage = doCMSTransform(temp_qimage, false, false);

	if (xformed_qimage == NULL) 
    	mPixmap->setPixmap(QPixmap::fromImage(*temp_qimage));
	else {
    	mPixmap->setPixmap(QPixmap::fromImage(*xformed_qimage));
		delete xformed_qimage;
	}

    informativeLabel->setText( tr("LDR image [%1 x %2]").arg(getWidth()).arg(getHeight()) );
	delete temp_qimage;
}

void LdrViewer::setTonemappingOptions(TonemappingOptions* tmopts)
{
    mTonemappingOptions = tmopts;

    parseOptions(tmopts, caption);
    setWindowTitle(caption);
    setToolTip(caption);
}

TonemappingOptions* LdrViewer::getTonemappingOptions()
{
    return mTonemappingOptions;
}

//! \brief returns max value of the handled frame
float LdrViewer::getMaxLuminanceValue()
{
    return 1.0f;
}

//! \brief returns min value of the handled frame
float LdrViewer::getMinLuminanceValue()
{
    return 0.0f;
}

QImage *LdrViewer::doCMSTransform(QImage *input_qimage, bool doProof, bool doGamutCheck)
{
	LuminanceOptions luminance_opts;
	QString monitor_fname = luminance_opts.getMonitorProfileFileName();
	qDebug() << "Monitor profile: " << monitor_fname;
	QString printer_fname = luminance_opts.getPrinterProfileFileName();
	qDebug() << "Printer profile: " << printer_fname;
	
	QImage *out_qimage = NULL;

	if (!monitor_fname.isEmpty()) {
		qDebug() << "Transform to Monitor Profile";
		QByteArray ba = monitor_fname.toUtf8();

		out_qimage = new QImage(input_qimage->width(), input_qimage->height(), QImage::Format_RGB32);		

		cmsHPROFILE hsRGB, hOut, hProof;
		cmsHTRANSFORM xform;

		#ifndef USE_LCMS2
			cmsErrorAction(LCMS_ERROR_SHOW);
		#endif

		hsRGB = cmsCreate_sRGBProfile();
		hOut = cmsOpenProfileFromFile(ba.data(), "r");
		
		if (hOut == NULL) {
			QMessageBox::warning(0,tr("Warning"), tr("I cannot open monitor profile. Please select a different one."), QMessageBox::Ok, QMessageBox::NoButton);
			return NULL; 
		}

		if (doProof && !printer_fname.isEmpty()) {
			QByteArray ba = printer_fname.toUtf8();
			hProof = cmsOpenProfileFromFile(ba.data(), "r");
			if (hProof == NULL) {
				QMessageBox::warning(0,tr("Warning"), tr("I cannot open printer profile. Please select a different one."), QMessageBox::Ok, QMessageBox::NoButton);
				doProof = false;
			}
		}
		else if (doProof) {
			QMessageBox::warning(0,tr("Warning"), tr("Please select a printer profile ."), QMessageBox::Ok, QMessageBox::NoButton);
			doProof = false;
		}
					
		if (doProof) {
			quint32 dwFlags = doGamutCheck ? cmsFLAGS_SOFTPROOFING | cmsFLAGS_GAMUTCHECK : cmsFLAGS_SOFTPROOFING;
			cmsSetAlarmCodes(255, 0, 0);
			xform = cmsCreateProofingTransform(hsRGB, TYPE_RGBA_8, hOut, TYPE_RGBA_8, hProof, INTENT_PERCEPTUAL, INTENT_ABSOLUTE_COLORIMETRIC, dwFlags);
		}
		else
			xform = cmsCreateTransform(hsRGB, TYPE_RGBA_8, hOut, TYPE_RGBA_8, INTENT_PERCEPTUAL, 0);

		if (xform == NULL) {
			QMessageBox::warning(0,tr("Warning"), tr("I cannot perform the color transform. Please select a different monitor profile."), QMessageBox::Ok, QMessageBox::NoButton);
			cmsCloseProfile(hOut);
			return NULL;		
		}

		cmsDoTransform(xform, input_qimage->bits(), out_qimage->bits(), input_qimage->width() * input_qimage->height());

		cmsCloseProfile(hOut);
		if (doProof)
			cmsCloseProfile(hProof);

		cmsDeleteTransform(xform);
	}

	return out_qimage;
}

void LdrViewer::doSoftProofing(bool doGamutCheck)
{
 	QImage *image = doCMSTransform(&mPixmap->pixmap().toImage(), true, doGamutCheck);	
	if (image == NULL)
		return;
	storedPixmap->setPixmap(mPixmap->pixmap());
	mPixmap->setPixmap(QPixmap::fromImage(*image));	
	delete image;
}

void LdrViewer::undoSoftProofing()
{
	mPixmap->setPixmap(storedPixmap->pixmap());
}
