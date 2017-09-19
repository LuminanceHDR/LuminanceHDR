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

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QScopedPointer>
#include <QString>

#include "Common/LuminanceOptions.h"
#include "Core/TonemappingOptions.h"
#include "Fileformat/pfsoutldrimage.h"
#include "Viewers/IGraphicsPixmapItem.h"
#include "Viewers/LdrViewer.h"

#include <Libpfs/frame.h>
#include <Libpfs/utils/resourcehandlerlcms.h>

using namespace pfs;

namespace {
void parseOptions(TonemappingOptions *opts, QString &caption) {
    if (opts == NULL) {
        caption.clear();
    } else {
        TMOptionsOperations tmopts(opts);
        // postfix = opts->getPostfix();
        caption = opts->getCaption();
        // exif_comment = tmopts.getExifComment();
    }
}

bool doCMSTransform(QImage &qImage, bool doProof, bool doGamutCheck) {
    LuminanceOptions luminance_opts;
    QString monitor_fname = luminance_opts.getMonitorProfileFileName();
    qDebug() << "Monitor profile: " << monitor_fname;
    QString printer_fname = luminance_opts.getPrinterProfileFileName();
    qDebug() << "Printer profile: " << printer_fname;

    // Check Monitor Profile
    if (monitor_fname.isEmpty()) {
        return false;
    }

    utils::ScopedCmsProfile hsRGB(cmsCreate_sRGBProfile());
    utils::ScopedCmsProfile hOut(cmsOpenProfileFromFile(
        QFile::encodeName(monitor_fname).constData(), "r"));

    utils::ScopedCmsProfile hProof;
    utils::ScopedCmsTransform xform;

    // Check whether the output profile is open
    if (!hOut) {
        QMessageBox::warning(0, QObject::tr("Warning"),
                             QObject::tr("I cannot open monitor profile. "
                                         "Please select a different one."),
                             QMessageBox::Ok, QMessageBox::NoButton);

        return false;
    }

    //
    if (doProof && !printer_fname.isEmpty()) {
        hProof.reset(cmsOpenProfileFromFile(
            QFile::encodeName(printer_fname).constData(), "r"));
        if (!hProof) {
            QMessageBox::warning(0, QObject::tr("Warning"),
                                 QObject::tr("I cannot open printer profile. "
                                             "Please select a different one."),
                                 QMessageBox::Ok, QMessageBox::NoButton);
            doProof = false;
        }
    } else if (doProof) {
        QMessageBox::warning(0, QObject::tr("Warning"),
                             QObject::tr("Please select a printer profile ."),
                             QMessageBox::Ok, QMessageBox::NoButton);
        doProof = false;
    }

    if (doProof) {
        cmsUInt32Number dwFlags =
            doGamutCheck ? cmsFLAGS_SOFTPROOFING | cmsFLAGS_GAMUTCHECK
                         : cmsFLAGS_SOFTPROOFING;
        cmsUInt16Number alarmCodes[cmsMAXCHANNELS] = {0};
        alarmCodes[1] = 0xFFFF;
        cmsSetAlarmCodes(alarmCodes);
        xform.reset(cmsCreateProofingTransform(
            hsRGB.data(), TYPE_BGRA_8,  // TYPE_RGBA_8,
            hOut.data(), TYPE_BGRA_8,   // TYPE_RGBA_8,
            hProof.data(), INTENT_PERCEPTUAL, INTENT_ABSOLUTE_COLORIMETRIC,
            dwFlags));
    } else {
        xform.reset(cmsCreateTransform(hsRGB.data(),
                                       TYPE_BGRA_8,  // TYPE_RGBA_8,
                                       hOut.data(),
                                       TYPE_BGRA_8,  // TYPE_RGBA_8,
                                       INTENT_PERCEPTUAL, 0));
    }

    if (!xform) {
        QMessageBox::warning(
            0, QObject::tr("Warning"),
            QObject::tr("I cannot perform the color transform. Please select a "
                        "different monitor profile."),
            QMessageBox::Ok, QMessageBox::NoButton);

        return false;
    }

    cmsDoTransform(xform.data(), qImage.bits(), qImage.bits(),
                   qImage.width() * qImage.height());

    return true;
}
}

// This constructor is a bit of a mess!
LdrViewer::LdrViewer(pfs::Frame *frame, TonemappingOptions *opts,
                     QWidget *parent, bool ns, const float devicePixelRatio)
    : GenericViewer(frame, parent, ns),
      informativeLabel(new QLabel(mToolBar)),
      mTonemappingOptions(opts) {
    informativeLabel->setSizePolicy(QSizePolicy::Preferred,
                                    QSizePolicy::Preferred);
    informativeLabel->setMinimumSize(QSize(200, 36));
    mToolBar->addWidget(informativeLabel);

    setDevicePixelRatio(devicePixelRatio);

    mPixmap->disableSelectionTool();  // disable by default crop functionalities

    // I shouldn't call a virtual function straight from the constructor,
    // but specifing correctly which version of this virtual function I want to
    // call,
    // I am safe
    LdrViewer::setTonemappingOptions(opts);

    QScopedPointer<QImage> temp_qimage(fromLDRPFStoQImage(getFrame()));
    doCMSTransform(*temp_qimage, false, false);
    setQImage(*temp_qimage);

    updateView();
    retranslateUi();
}

LdrViewer::~LdrViewer() {
#ifdef QT_DEBUG
    qDebug() << "LdrViewer::~LdrViewer()";
#endif
}

void LdrViewer::retranslateUi() {
    parseOptions(mTonemappingOptions, caption);
    informativeLabel->setText(tr("LDR image [%1 x %2]: %3")
                                  .arg(getWidth())
                                  .arg(getHeight())
                                  .arg(caption));

    GenericViewer::retranslateUi();
}

QString LdrViewer::getFileNamePostFix() {
    return mTonemappingOptions ? mTonemappingOptions->getPostfix() : QString();
}

QString LdrViewer::getExifComment() {
    if (mTonemappingOptions) {
        TMOptionsOperations tm_ops(mTonemappingOptions);
        return tm_ops.getExifComment();
    } else {
        return QString();
    }
}

void LdrViewer::updatePixmap() {
#ifdef QT_DEBUG
    qDebug() << "void LdrViewer::updatePixmap()";
#endif

    QScopedPointer<QImage> temp_qimage(fromLDRPFStoQImage(getFrame()));

    doCMSTransform(*temp_qimage, false, false);
    mPixmap->setPixmap(QPixmap::fromImage(*temp_qimage));

    parseOptions(mTonemappingOptions, caption);
    informativeLabel->setText(tr("LDR image [%1 x %2]: %3")
                                  .arg(getWidth())
                                  .arg(getHeight())
                                  .arg(caption));
}

void LdrViewer::setTonemappingOptions(TonemappingOptions *tmopts) {
    mTonemappingOptions = tmopts;

    parseOptions(tmopts, caption);
    setWindowTitle(caption);
    // setToolTip(caption);
    informativeLabel->setText(tr("LDR image [%1 x %2]: %3")
                                  .arg(getWidth())
                                  .arg(getHeight())
                                  .arg(caption));
}

TonemappingOptions *LdrViewer::getTonemappingOptions() {
    return mTonemappingOptions;
}

//! \brief returns max value of the handled frame
float LdrViewer::getMaxLuminanceValue() { return 1.0f; }

//! \brief returns min value of the handled frame
float LdrViewer::getMinLuminanceValue() { return 0.0f; }

void LdrViewer::doSoftProofing(bool doGamutCheck) {
    QScopedPointer<QImage> src_image(fromLDRPFStoQImage(getFrame()));
    if (doCMSTransform(*src_image, true, doGamutCheck)) {
        mPixmap->setPixmap(QPixmap::fromImage(*src_image));
    }
}

void LdrViewer::undoSoftProofing() {
    QScopedPointer<QImage> src_image(fromLDRPFStoQImage(getFrame()));
    if (doCMSTransform(*src_image, false, false)) {
        mPixmap->setPixmap(QPixmap::fromImage(*src_image));
    }
}
