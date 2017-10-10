/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Franco Comida
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

#include <QAction>
#include <QDebug>
#include <QSharedPointer>
#include <QtConcurrentMap>

#include "PreviewSettings.h"

#include "Libpfs/frame.h"
#include "Libpfs/manip/copy.h"
#include "Libpfs/manip/cut.h"
#include "Libpfs/manip/resize.h"
#include "Libpfs/progress.h"
#include "Libpfs/tm/TonemapOperator.h"

#include "Common/LuminanceOptions.h"
#include "Core/TMWorker.h"
#include "Fileformat/pfsoutldrimage.h"
#include "PreviewPanel/PreviewLabel.h"

namespace  // anoymous namespace
{
const int PREVIEW_WIDTH = 120;
const int PREVIEW_HEIGHT = 100;

//! \note It is not the most efficient way to do this thing, but I will fix it
//! later
//! this function get calls multiple time
void resetTonemappingOptions(TonemappingOptions *tm_options,
                             const pfs::Frame *frame) {
    tm_options->origxsize = frame->getWidth();
    tm_options->xsize = frame->getWidth();
    tm_options->tonemapSelection = false;
}

class PreviewLabelUpdater {
   public:
    explicit PreviewLabelUpdater(QSharedPointer<pfs::Frame> reference_frame)
        : m_ReferenceFrame(reference_frame) {}

    //! \brief QRunnable::run() definition
    //! \caption I use shared pointer in this function, so I don't have to worry
    //! about memory allocation
    //! in case something wrong happens, it shouldn't leak
    void operator()(PreviewLabel *to_update) {
#ifdef QT_DEBUG
// qDebug() << QThread::currentThread() << "running...";
#endif

        // retrieve TM parameters
        TonemappingOptions *tm_options = to_update->getTonemappingOptions();
        resetTonemappingOptions(tm_options, m_ReferenceFrame.data());

        if (m_ReferenceFrame.isNull()) {
#ifdef QT_DEBUG
            qDebug() << "operator()() for TM"
                     << static_cast<int>(tm_options->tmoperator)
                     << " received a NULL pointer";
            return;
#endif
        }

        pfs::Progress fake_progress;

        // Copy Reference Frame
        QSharedPointer<pfs::Frame> temp_frame(
            pfs::copy(m_ReferenceFrame.data()));

        // Tone Mapping
        QScopedPointer<TonemapOperator> tm_operator(
            TonemapOperator::getTonemapOperator(tm_options->tmoperator));
        tm_operator->tonemapFrame(*temp_frame, tm_options, fake_progress);

        // Create QImage from pfs::Frame into QSharedPointer, and I give it to
        // the
        // preview panel
        QSharedPointer<QImage> qimage(fromLDRPFStoQImage(temp_frame.data()));

        //! \note I cannot use these 2 functions, because setPixmap must run in
        //! the
        //! GUI thread
        // m_PreviewLabel->setPixmap( QPixmap::fromImage(*qimage) );
        // m_PreviewLabel->adjustSize();
        //! \note So I queue a SLOT request on the m_PreviewSettings
        QMetaObject::invokeMethod(to_update, "assignNewQImage",
                                  Qt::QueuedConnection,
                                  Q_ARG(QSharedPointer<QImage>, qimage));

#ifdef QT_DEBUG
// qDebug() << QThread::currentThread() << "done!";
#endif
    }

   private:
    QSharedPointer<pfs::Frame> m_ReferenceFrame;
};
}

PreviewSettings::PreviewSettings(QWidget *parent)
    : QWidget(parent), m_original_width_frame(0) {
    //! \note I need to register the new object to pass this class as parameter
    //! inside invokeMethod()
    //! see run() inside PreviewLabelUpdater
    qRegisterMetaType<QSharedPointer<QImage>>("QSharedPointer<QImage>");

    m_flowLayout = new FlowLayout;

    setLayout(m_flowLayout);
}

PreviewSettings::~PreviewSettings() {
#ifdef QT_DEBUG
    qDebug() << "PreviewSettings::~PreviewSettings()";
#endif
}

void PreviewSettings::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) {
        // m_Ui->retranslateUi(this);
    }

    QWidget::changeEvent(event);
}

void PreviewSettings::updatePreviews(pfs::Frame *frame) {
    if (frame == NULL) return;

    m_original_width_frame = frame->getWidth();

    int frame_width = frame->getWidth();
    int frame_height = frame->getHeight();

    int resized_width = PREVIEW_WIDTH;
    if (frame_height > frame_width) {
        float ratio = ((float)frame_width) / frame_height;
        resized_width = PREVIEW_HEIGHT * ratio;
    }
    // 1. make a resized copy
    QSharedPointer<pfs::Frame> current_frame(
        pfs::resize(frame, resized_width, BilinearInterp));

    // 2. (non concurrent) for each PreviewLabel, call
    // PreviewLabelUpdater::operator()
    foreach (PreviewLabel *current_label, m_ListPreviewLabel) {
        PreviewLabelUpdater updater(current_frame);
        updater(current_label);
    }
    // 2. (concurrent) for each PreviewLabel, call
    // PreviewLabelUpdater::operator()
    // QtConcurrent::map (m_ListPreviewLabel, PreviewLabelUpdater(current_frame)
    // );
}

void PreviewSettings::tonemapPreview(TonemappingOptions *opts) {
#ifdef QT_DEBUG
    qDebug() << "void PreviewSettings::tonemapPreview()";
#endif

    opts->xsize = LuminanceOptions().getPreviewWidth();
    opts->origxsize = m_original_width_frame;

    emit startTonemapping(opts);
}

QSize PreviewSettings::getLabelSize() {
    return m_ListPreviewLabel.at(0)->pixmap()->size();
}

void PreviewSettings::addPreviewLabel(PreviewLabel *label) {
    TonemappingOptions *opts = label->getTonemappingOptions();
    QString text = opts->getCaption(true, QStringLiteral("\n"));

    if (label->actions().isEmpty()) {
        QAction *pAction = new QAction(tr("Load settings"), label);
        label->addAction(pAction);
        connect(pAction, &QAction::triggered, this,
                &PreviewSettings::triggered);
    }

    label->setToolTip(text);

    m_ListPreviewLabel.append(label);
    label->setFrameStyle(QFrame::Box);
    m_flowLayout->addWidget(label);
}

void PreviewSettings::selectLabel(int index) {
    for (int i = 0; i < m_flowLayout->getSize(); i++) {
        QWidget *w = m_flowLayout->itemAt(i)->widget();
        QLabel *l = static_cast<QLabel *>(w);
        l->setLineWidth((i == index) ? 3 : 1);
    }
}

PreviewLabel *PreviewSettings::getPreviewLabel(int index) {
    QWidget *w = m_flowLayout->itemAt(index)->widget();
    return static_cast<PreviewLabel *>(w);
}

void PreviewSettings::clear() {
    int size = m_flowLayout->getSize();
    for (int i = 0; i < size; i++) {
        m_flowLayout->takeAt(i);
    }
    m_ListPreviewLabel.clear();
}
