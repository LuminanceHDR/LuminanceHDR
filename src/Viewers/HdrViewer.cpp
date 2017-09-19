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

#include "HdrViewer.h"

#include <QDebug>
#include <QFileInfo>

#include <cassert>
#include <cmath>
#include "arch/math.h"

#include "Common/global.h"

#include "Fileformat/pfsoutldrimage.h"
#include "Viewers/IGraphicsPixmapItem.h"
#include "Viewers/LuminanceRangeWidget.h"

#include "Libpfs/array2d.h"
#include "Libpfs/channel.h"
#include "Libpfs/frame.h"
#include "Libpfs/utils/msec_timer.h"
#include "Libpfs/utils/sse.h"

namespace  // anonymous namespace
{
// Implementation details stay inside an anonymous namespace
// In this way, we let know the compiler it can mess up as much as it wants with
// the code,
// because it will only used inside this compilation unit

const pfs::Array2Df *getPrimaryChannel(const pfs::Frame &frame) {
    return frame.getChannel("Y");
}

}  // end anonymous namespace

HdrViewer::HdrViewer(pfs::Frame *frame, QWidget *parent, bool ns)
    : GenericViewer(frame, parent, ns),
      m_mappingMethod(MAP_GAMMA2_2),
      m_minValue(0.f),
      m_maxValue(1.f) {
    initUi();

    // I prefer to do everything by hand, so the flow of the calls is clear
    m_lumRange->blockSignals(true);

    m_lumRange->setHistogramImage(getPrimaryChannel(*getFrame()));
    m_lumRange->fitToDynamicRange();

    m_mappingMethod =
        static_cast<RGBMappingType>(m_mappingMethodCB->currentIndex());
    m_minValue = powf(10.0f, m_lumRange->getRangeWindowMin());
    m_maxValue = powf(10.0f, m_lumRange->getRangeWindowMax());

    QScopedPointer<QImage> qImage(mapFrameToImage(getFrame()));
    mPixmap->setPixmap(QPixmap::fromImage(*qImage));

    updateView();
    m_lumRange->blockSignals(false);
}

void HdrViewer::initUi() {
    m_mappingMethodLabel = new QLabel(mToolBar);
    m_mappingMethodLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_mappingMethodCB = new QComboBox(mToolBar);
    mToolBar->addWidget(m_mappingMethodLabel);
    mToolBar->addWidget(m_mappingMethodCB);
    mToolBar->addSeparator();
    m_mappingMethodLabel->setBuddy(m_mappingMethodCB);

    m_histLabel = new QLabel(mToolBar);
    m_lumRange = new LuminanceRangeWidget(mToolBar);
    mToolBar->addWidget(m_histLabel);
    mToolBar->addWidget(m_lumRange);
    mToolBar->addSeparator();
    connect(m_lumRange, &LuminanceRangeWidget::updateRangeWindow, this,
            &HdrViewer::updateRangeWindow);
    mToolBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    retranslateUi();
}

void HdrViewer::retranslateUi() {
    m_mappingMethodLabel->setText(tr("&Mapping:"));
    m_histLabel->setText(tr("Histogram:"));

    int oldMappingMethodIndex = m_mappingMethodCB->currentIndex();

    disconnect(m_mappingMethodCB,
               static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
               this, &HdrViewer::setLumMappingMethod);
    QStringList methods;
    methods << tr("Linear") << tr("Gamma 1.4") << tr("Gamma 1.8")
            << tr("Gamma 2.2") << tr("Gamma 2.6") << tr("Logarithmic");

    m_mappingMethodCB->clear();
    m_mappingMethodCB->addItems(methods);
    m_mappingMethodCB->setCurrentIndex(
        oldMappingMethodIndex >= 0 ? oldMappingMethodIndex : 3);
    connect(m_mappingMethodCB,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this,
            &HdrViewer::setLumMappingMethod);
    connect(m_mappingMethodCB, SIGNAL(currentIndexChanged(int)),
            m_mappingMethodCB, SLOT(setFocus()));  // TODO convert to new style

    GenericViewer::retranslateUi();
}

void HdrViewer::refreshPixmap() {
    setCursor(Qt::WaitCursor);

    QScopedPointer<QImage> qImage(mapFrameToImage(getFrame()));
    mPixmap->setPixmap(QPixmap::fromImage(*qImage));

    unsetCursor();
}

void HdrViewer::updatePixmap() {
#ifdef QT_DEBUG
    qDebug() << "void HdrViewer::updatePixmap()";
#endif

    m_lumRange->blockSignals(true);

    refreshPixmap();

    // I need to set the histogram again during the setFrame function
    m_lumRange->setHistogramImage(getPrimaryChannel(*getFrame()));
    m_lumRange->fitToDynamicRange();
    m_lumRange->blockSignals(false);
}

LuminanceRangeWidget *HdrViewer::lumRange() { return m_lumRange; }

void HdrViewer::updateRangeWindow() {
    setRangeWindow(powf(10.0f, m_lumRange->getRangeWindowMin()),
                   powf(10.0f, m_lumRange->getRangeWindowMax()));
}

void HdrViewer::setRangeWindow(float min, float max) {
    m_minValue = min;
    m_maxValue = max;

    refreshPixmap();
}

int HdrViewer::getLumMappingMethod() {
    return m_mappingMethodCB->currentIndex();
}

void HdrViewer::setLumMappingMethod(int method) {
    m_mappingMethodCB->setCurrentIndex(method);
    m_mappingMethod = static_cast<RGBMappingType>(method);

    refreshPixmap();
}

//! empty dtor
HdrViewer::~HdrViewer() {}

QString HdrViewer::getFileNamePostFix() {
    return QStringLiteral("_hdr_preview");
}

QString HdrViewer::getExifComment() {
    return QStringLiteral("HDR Created with Luminance HDR");
}

//! \brief returns max value of the handled frame
float HdrViewer::getMaxLuminanceValue() { return m_maxValue; }

//! \brief returns min value of the handled frame
float HdrViewer::getMinLuminanceValue() { return m_minValue; }

RGBMappingType HdrViewer::getLuminanceMappingMethod() {
    return m_mappingMethod;
}

QImage *HdrViewer::mapFrameToImage(pfs::Frame *in_frame) {
    return fromLDRPFStoQImage(in_frame, m_minValue, m_maxValue,
                              m_mappingMethod);
}

void HdrViewer::keyPressEvent(QKeyEvent *event) {
    GenericViewer::keyPressEvent(event);
    if (event->key() == Qt::Key_L) {
        m_lumRange->lowDynamicRange();
    } else if (event->key() == Qt::Key_BracketLeft) {
        m_lumRange->extendRange();
    } else if (event->key() == Qt::Key_BracketRight) {
        m_lumRange->shrinkRange();
    } else if (event->key() == Qt::Key_Backslash) {
        m_lumRange->fitToDynamicRange();
    } else if (event->key() == Qt::Key_0) {
        m_lumRange->decreaseExposure();
    } else if (event->key() == Qt::Key_9) {
        m_lumRange->increaseExposure();
    }
}
