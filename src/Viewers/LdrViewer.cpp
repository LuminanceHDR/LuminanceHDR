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

#include <QDebug>
#include <QScopedPointer>
#include <QString>
#include <QByteArray>
#include <QMessageBox>
#include <QFile>

#include "Viewers/LdrViewer.h"
#include "Viewers/IGraphicsPixmapItem.h"
#include "Core/TonemappingOptions.h"
#include "Libpfs/frame.h"
#include "Fileformat/pfsoutldrimage.h"
#include "Common/LuminanceOptions.h"
#include "Common/ResourceHandlerLcms.h"

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

QImage* doCMSTransform(QImage* input_qimage, bool doProof, bool doGamutCheck)
{
    LuminanceOptions luminance_opts;
    QString monitor_fname = luminance_opts.getMonitorProfileFileName();
    qDebug() << "Monitor profile: " << monitor_fname;
    QString printer_fname = luminance_opts.getPrinterProfileFileName();
    qDebug() << "Printer profile: " << printer_fname;

    // CHECK MONITOR PROFILE NAME
    // TODO: Check inside LuminanceOptions whether the file is on the filesystem
    // or it has been removed!
    if ( monitor_fname.isEmpty() ) return NULL;

    qDebug() << "Transform to Monitor Profile";
    QByteArray iccProfileMonitor( QFile::encodeName( monitor_fname ) );

    QScopedPointer<QImage> out_qimage( new QImage(input_qimage->width(), input_qimage->height(), QImage::Format_RGB32) );

    ScopedCmsProfile hsRGB( cmsCreate_sRGBProfile() );
    ScopedCmsProfile hOut( cmsOpenProfileFromFile(iccProfileMonitor.constData(), "r") );

    ScopedCmsProfile hProof;
    ScopedCmsTransform xform;

    // Check whether the output profile is open
    if ( !hOut )
    {
        QMessageBox::warning(0,
                             QObject::tr("Warning"),
                             QObject::tr("I cannot open monitor profile. Please select a different one."),
                             QMessageBox::Ok, QMessageBox::NoButton);

        return NULL;
    }

    //
    if (doProof && !printer_fname.isEmpty())
    {
        QByteArray iccProfilePrinter( QFile::encodeName( printer_fname ) );
        hProof.reset( cmsOpenProfileFromFile(iccProfilePrinter.constData(), "r") );
        if ( !hProof )
        {
            QMessageBox::warning(0,
                                 QObject::tr("Warning"),
                                 QObject::tr("I cannot open printer profile. Please select a different one."),
                                 QMessageBox::Ok, QMessageBox::NoButton);
            doProof = false;
        }
    }
    else if (doProof)
    {
        QMessageBox::warning(0,
                             QObject::tr("Warning"),
                             QObject::tr("Please select a printer profile ."),
                             QMessageBox::Ok, QMessageBox::NoButton);
        doProof = false;
    }

    if (doProof)
    {
        cmsUInt32Number dwFlags = doGamutCheck ? cmsFLAGS_SOFTPROOFING | cmsFLAGS_GAMUTCHECK : cmsFLAGS_SOFTPROOFING;
        cmsUInt16Number alarmCodes[cmsMAXCHANNELS] = { 0 };
        alarmCodes[1] = 0xFFFF;
        cmsSetAlarmCodes(alarmCodes);
        xform.reset( cmsCreateProofingTransform(hsRGB.data(), TYPE_RGBA_8,
                                                hOut.data(), TYPE_RGBA_8,
                                                hProof.data(), INTENT_PERCEPTUAL, INTENT_ABSOLUTE_COLORIMETRIC, dwFlags) );
    }
    else
    {
        xform.reset( cmsCreateTransform(hsRGB.data(), TYPE_RGBA_8,
                                        hOut.data(), TYPE_RGBA_8, INTENT_PERCEPTUAL, 0) );
    }

    if ( !xform )
    {
        QMessageBox::warning(0,
                             QObject::tr("Warning"),
                             QObject::tr("I cannot perform the color transform. Please select a different monitor profile."),
                             QMessageBox::Ok, QMessageBox::NoButton);

        return NULL;
    }

    cmsDoTransform(xform.data(),
                   input_qimage->bits(),
                   out_qimage->bits(),
                   (input_qimage->width() * input_qimage->height()) );

    return out_qimage.take();
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

    QScopedPointer<QImage> temp_qimage( fromLDRPFStoQImage(getFrame()) );
    QScopedPointer<QImage> xformed_qimage( doCMSTransform(temp_qimage.data(), false, false) );

    if (xformed_qimage == NULL)
    {
        setQImage(*temp_qimage);
    }
    else
    {
        setQImage(*xformed_qimage);
    }

    updateView();
    retranslateUi();
}

LdrViewer::~LdrViewer()
{
#ifdef QT_DEBUG
    qDebug() << "LdrViewer::~LdrViewer()";
#endif
	delete mTonemappingOptions;
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

    QScopedPointer<QImage> temp_qimage( fromLDRPFStoQImage(getFrame()) );
    QScopedPointer<QImage> xformed_qimage( doCMSTransform(temp_qimage.data(), false, false) );

    if ( !xformed_qimage )
    {
        mPixmap->setPixmap(QPixmap::fromImage(*temp_qimage));
    }
    else
    {
        mPixmap->setPixmap(QPixmap::fromImage(*xformed_qimage));
    }

    informativeLabel->setText( tr("LDR image [%1 x %2]").arg(getWidth()).arg(getHeight()) );
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

void LdrViewer::doSoftProofing(bool doGamutCheck)
{
    QScopedPointer<QImage> src_image( fromLDRPFStoQImage(getFrame()) );
    QScopedPointer<QImage> image( doCMSTransform(src_image.data(), true, doGamutCheck) );
    if (image.data() != NULL)
    {
        mPixmap->setPixmap(QPixmap::fromImage(*image));
    }
}

void LdrViewer::undoSoftProofing()
{
    QScopedPointer<QImage> src_image( fromLDRPFStoQImage(getFrame()) );
    QScopedPointer<QImage> image( doCMSTransform(src_image.data(), false, false) );
    if (image.data() != NULL)
    {
        mPixmap->setPixmap(QPixmap::fromImage(*image));
    }
}
