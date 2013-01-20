/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2011 Franco Comida
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
 * @author Franco Comida <fcomida@users.sourceforge.net>
 */

#include <QDebug>
#include <QtConcurrentMap>
#include <QSharedPointer>

#include "PreviewPanel.h"

#include "Libpfs/frame.h"
#include "Libpfs/domio.h"
#include "Libpfs/manip/copy.h"
#include "Libpfs/manip/resize.h"

#include "Core/TMWorker.h"
#include "Libpfs/tm/TonemapOperator.h"

#include "Fileformat/pfsoutldrimage.h"
#include "PreviewPanel/PreviewLabel.h"

#include "Common/LuminanceOptions.h"
#include "UI/FlowLayout.h"

namespace // anoymous namespace
{
const int PREVIEW_WIDTH = 120;
const int PREVIEW_HEIGHT = 100;

//! \note It is not the most efficient way to do this thing, but I will fix it later
//! this function get calls multiple time
void resetTonemappingOptions(TonemappingOptions* tm_options, const pfs::Frame* frame)
{
    tm_options->origxsize          = frame->getWidth();
    tm_options->xsize              = frame->getWidth();
    //tm_options->pregamma           = 1.0f;  //TODO check this
    tm_options->tonemapSelection   = false;
}

class PreviewLabelUpdater
{
public:
    PreviewLabelUpdater(QSharedPointer<pfs::Frame> reference_frame):
        m_ReferenceFrame(reference_frame)
    {}

    //! \brief QRunnable::run() definition
    //! \caption I use shared pointer in this function, so I don't have to worry about memory allocation
    //! in case something wrong happens, it shouldn't leak
    void operator()(PreviewLabel* to_update)
    {
#ifdef QT_DEBUG
        //qDebug() << QThread::currentThread() << "running...";
#endif

        // retrieve TM parameters
        TonemappingOptions* tm_options = to_update->getTonemappingOptions();
        resetTonemappingOptions(tm_options, m_ReferenceFrame.data());

        if ( m_ReferenceFrame.isNull() )
        {
#ifdef QT_DEBUG
            qDebug() << "operator()() for TM" << static_cast<int>(tm_options->tmoperator) << " received a NULL pointer";
            return;
#endif
        }

        //ProgressHelper fake_progress_helper;

        // Copy Reference Frame
        QSharedPointer<pfs::Frame> temp_frame( pfs::copy(m_ReferenceFrame.data()) );

        // Tone Mapping
        //QScopedPointer<TonemapOperator> tm_operator( TonemapOperator::getTonemapOperator(tm_options->tmoperator));
        //tm_operator->tonemapFrame(temp_frame.data(), tm_options, fake_progress_helper);
        try {
            QScopedPointer<TMWorker> tmWorker(new TMWorker);
            QSharedPointer<pfs::Frame> frame (tmWorker->computeTonemap(temp_frame.data(), tm_options));
        
            // Create QImage from pfs::Frame into QSharedPointer, and I give it to the preview panel
            //QSharedPointer<QImage> qimage(fromLDRPFStoQImage(temp_frame.data()));
            QSharedPointer<QImage> qimage(fromLDRPFStoQImage(frame.data()));

            //! \note I cannot use these 2 functions, because setPixmap must run in the GUI thread
            //m_PreviewLabel->setPixmap( QPixmap::fromImage(*qimage) );
            //m_PreviewLabel->adjustSize();
            //! \note So I queue a SLOT request on the m_PreviewPanel
            QMetaObject::invokeMethod(to_update, "assignNewQImage", Qt::QueuedConnection,
                                      Q_ARG(QSharedPointer<QImage>, qimage));
        }
        catch (...) {
            qDebug() << "PreviewLabelUpdater: caught exception";
            QSharedPointer<QImage> qimage(new QImage);
            QMetaObject::invokeMethod(to_update, "assignNewQImage", Qt::QueuedConnection,
                                      Q_ARG(QSharedPointer<QImage>, qimage));
        }

#ifdef QT_DEBUG
        //qDebug() << QThread::currentThread() << "done!";
#endif
    }

private:
    QSharedPointer<pfs::Frame> m_ReferenceFrame;
};

}

PreviewPanel::PreviewPanel(QWidget *parent):
    QWidget(parent)
{
    //! \note I need to register the new object to pass this class as parameter inside invokeMethod()
    //! see run() inside PreviewLabelUpdater
    qRegisterMetaType< QSharedPointer<QImage> >("QSharedPointer<QImage>");

    PreviewLabel * labelMantiuk06 = new PreviewLabel(this, mantiuk06);
    labelMantiuk06->setText("Mantiuk '06");
    labelMantiuk06->setToolTip("Mantiuk '06");
    labelMantiuk06->setFrameStyle(QFrame::Box);
    m_ListPreviewLabel.push_back(labelMantiuk06);
    connect(labelMantiuk06, SIGNAL(clicked(TonemappingOptions*)), this, SLOT(tonemapPreview(TonemappingOptions*)));

    PreviewLabel * labelMantiuk08 = new PreviewLabel(this, mantiuk08);
    labelMantiuk08->setText("Mantiuk '08");
    labelMantiuk08->setToolTip("Mantiuk '08");
    labelMantiuk08->setFrameStyle(QFrame::Box);
    m_ListPreviewLabel.push_back(labelMantiuk08);
    connect(labelMantiuk08, SIGNAL(clicked(TonemappingOptions*)), this, SLOT(tonemapPreview(TonemappingOptions*)));

    PreviewLabel * labelFattal = new PreviewLabel(this, fattal);
    labelFattal->setText("Fattal");
    labelFattal->setToolTip("Fattal");
    labelFattal->setFrameStyle(QFrame::Box);
    m_ListPreviewLabel.push_back(labelFattal);
    connect(labelFattal, SIGNAL(clicked(TonemappingOptions*)), this, SLOT(tonemapPreview(TonemappingOptions*)));

    PreviewLabel * labelDrago = new PreviewLabel(this, drago);
    labelDrago->setText("Drago");
    labelDrago->setToolTip("Drago");
    labelDrago->setFrameStyle(QFrame::Box);
    m_ListPreviewLabel.push_back(labelDrago);
    connect(labelDrago, SIGNAL(clicked(TonemappingOptions*)), this, SLOT(tonemapPreview(TonemappingOptions*)));

    PreviewLabel * labelDurand = new PreviewLabel(this, durand);
    labelDurand->setText("Durand");
    labelDurand->setToolTip("Durand");
    labelDurand->setFrameStyle(QFrame::Box);
    m_ListPreviewLabel.push_back(labelDurand);
    connect(labelDurand, SIGNAL(clicked(TonemappingOptions*)), this, SLOT(tonemapPreview(TonemappingOptions*)));

    PreviewLabel * labelReinhard02= new PreviewLabel(this, reinhard02);
    labelReinhard02->setText("Reinhard '02");
    labelReinhard02->setToolTip("Reinhard '02");
    labelReinhard02->setFrameStyle(QFrame::Box);
    m_ListPreviewLabel.push_back(labelReinhard02);
    connect(labelReinhard02, SIGNAL(clicked(TonemappingOptions*)), this, SLOT(tonemapPreview(TonemappingOptions*)));

    PreviewLabel * labelReinhard05 = new PreviewLabel(this, reinhard05);
    labelReinhard05->setText("Reinhard '05");
    labelReinhard05->setToolTip("Reinhard '05");
    labelReinhard05->setFrameStyle(QFrame::Box);
    m_ListPreviewLabel.push_back(labelReinhard05);
    connect(labelReinhard05, SIGNAL(clicked(TonemappingOptions*)), this, SLOT(tonemapPreview(TonemappingOptions*)));

    PreviewLabel * labelAshikhmin = new PreviewLabel(this, ashikhmin);
    labelAshikhmin->setText("Ashikhmin");
    labelAshikhmin->setToolTip("Ashikhmin");
    labelAshikhmin->setFrameStyle(QFrame::Box);
    m_ListPreviewLabel.push_back(labelAshikhmin);
    connect(labelAshikhmin, SIGNAL(clicked(TonemappingOptions*)), this, SLOT(tonemapPreview(TonemappingOptions*)));

    PreviewLabel * labelPattanaik = new PreviewLabel(this, pattanaik);
    labelPattanaik->setText("Pattanaik");
    labelPattanaik->setToolTip("Pattanaik");
    labelPattanaik->setFrameStyle(QFrame::Box);
    m_ListPreviewLabel.push_back(labelPattanaik);
    connect(labelPattanaik, SIGNAL(clicked(TonemappingOptions*)), this, SLOT(tonemapPreview(TonemappingOptions*)));

    FlowLayout *flowLayout = new FlowLayout;

    flowLayout->addWidget(labelMantiuk06);
    flowLayout->addWidget(labelMantiuk08);
    flowLayout->addWidget(labelFattal);
    flowLayout->addWidget(labelDrago);
    flowLayout->addWidget(labelDurand);
    flowLayout->addWidget(labelReinhard02);
    flowLayout->addWidget(labelReinhard05);
    flowLayout->addWidget(labelAshikhmin);
    flowLayout->addWidget(labelPattanaik);

    setLayout(flowLayout);
}

PreviewPanel::~PreviewPanel()
{
#ifdef QT_DEBUG
    qDebug() << "PreviewPanel::~PreviewPanel()";
#endif
}

void PreviewPanel::updatePreviews(pfs::Frame* frame, int index)
{
    if ( frame == NULL ) return;

    original_width_frame = frame->getWidth();

    int frame_width = frame->getWidth();
    int frame_height = frame->getHeight();

    int resized_width = PREVIEW_WIDTH;
    if (frame_height > frame_width)
    {
        float ratio = ((float)frame_width)/frame_height;
        resized_width = PREVIEW_HEIGHT*ratio;
    }
    // 1. make a resized copy
    QSharedPointer<pfs::Frame> current_frame( pfs::resize(frame, resized_width));

    // 2. (non concurrent) for each PreviewLabel, call PreviewLabelUpdater::operator()
    if (index == -1) {
        foreach(PreviewLabel* current_label, m_ListPreviewLabel)
        {
            PreviewLabelUpdater updater(current_frame);
            updater(current_label);
        }
    }
    else {
        PreviewLabelUpdater updater(current_frame);
        updater(m_ListPreviewLabel.at(index));
    }
    // 2. (concurrent) for each PreviewLabel, call PreviewLabelUpdater::operator()
    //QtConcurrent::map (m_ListPreviewLabel, PreviewLabelUpdater(current_frame) );
}

void PreviewPanel::tonemapPreview(TonemappingOptions* opts)
{
#ifdef QT_DEBUG
    qDebug() << "void PreviewPanel::tonemapPreview()";
#endif

    opts->xsize = LuminanceOptions().getPreviewWidth();
    opts->origxsize = original_width_frame;

    emit startTonemapping(opts);
}

QSize PreviewPanel::getLabelSize()
{
    return m_ListPreviewLabel.at(0)->pixmap()->size();
}

PreviewLabel *PreviewPanel::getLabel(int index)
{
    return m_ListPreviewLabel.at(index);
}
