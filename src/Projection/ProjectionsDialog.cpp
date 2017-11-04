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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */

#include <QtConcurrentRun>
#include <boost/bind.hpp>

#include "Projection/ProjectionsDialog.h"
#include "Projection/ui_ProjectionsDialog.h"

#include "Libpfs/frame.h"
#include "Libpfs/manip/projection.h"

static void worker(pfs::Frame *original, pfs::Frame *transformed, int xSize,
                   int ySize, TransformInfo *transforminfo) {
    const pfs::ChannelContainer &channels = original->getChannels();

    for (pfs::ChannelContainer::const_iterator it = channels.begin();
         it != channels.end(); ++it) {
        pfs::Channel *newCh = transformed->createChannel((*it)->getName());

        transformArray(*it, newCh, transforminfo);
    }

    pfs::copyTags(original, transformed);
}

ProjectionsDialog::ProjectionsDialog(QWidget *parent, pfs::Frame *orig)
    : QDialog(parent),
      original(orig),
      transformed(NULL),
      m_Ui(new Ui::ProjectionsDialog) {
    m_Ui->setupUi(this);

    m_Ui->progressBar->hide();

    projectionList.append(&(PolarProjection::singleton));
    projectionList.append(&(AngularProjection::singleton));
    projectionList.append(&(CylindricalProjection::singleton));
    projectionList.append(&(MirrorBallProjection::singleton));
    transforminfo = new TransformInfo();
    transforminfo->srcProjection = projectionList.at(0);
    transforminfo->dstProjection = projectionList.at(0);

    connect(m_Ui->okButton, &QAbstractButton::clicked, this,
            &ProjectionsDialog::okClicked);
    connect(m_Ui->sourceProjection, SIGNAL(activated(int)), this,
            SLOT(srcProjActivated(int)));
    connect(m_Ui->destProjection, SIGNAL(activated(int)), this,
            SLOT(dstProjActivated(int)));
    connect(m_Ui->bilinearCheckBox, &QAbstractButton::toggled, this,
            &ProjectionsDialog::bilinearToggled);
    connect(m_Ui->oversampleSpinBox, SIGNAL(valueChanged(int)), this,
            SLOT(oversampleChanged(int)));
    connect(m_Ui->XrotSpinBox, SIGNAL(valueChanged(int)), this,
            SLOT(XRotChanged(int)));
    connect(m_Ui->YrotSpinBox, SIGNAL(valueChanged(int)), this,
            SLOT(YRotChanged(int)));
    connect(m_Ui->ZrotSpinBox, SIGNAL(valueChanged(int)), this,
            SLOT(ZRotChanged(int)));
    connect(m_Ui->anglesSpinBox, SIGNAL(valueChanged(int)), this,
            SLOT(anglesAngularDestinationProj(int)));

    connect(&m_futureWatcher, &QFutureWatcherBase::finished, this,
            &ProjectionsDialog::projectionFinished, Qt::DirectConnection);
}

ProjectionsDialog::~ProjectionsDialog() { delete transforminfo; }

void ProjectionsDialog::XRotChanged(int v) { transforminfo->xRotate = v; }
void ProjectionsDialog::YRotChanged(int v) { transforminfo->yRotate = v; }
void ProjectionsDialog::ZRotChanged(int v) { transforminfo->zRotate = v; }
void ProjectionsDialog::oversampleChanged(int v) {
    transforminfo->oversampleFactor = v;
}
void ProjectionsDialog::bilinearToggled(bool v) {
    transforminfo->interpolate = v;
}
void ProjectionsDialog::dstProjActivated(int gui_index) {
    transforminfo->dstProjection = projectionList.at(gui_index);
    bool transformIsAngular =
        transforminfo->dstProjection == &(AngularProjection::singleton);
    m_Ui->labelAngles->setEnabled(transformIsAngular);
    m_Ui->anglesSpinBox->setEnabled(transformIsAngular);
    if (transformIsAngular)
        ((AngularProjection *)(transforminfo->dstProjection))
            ->setAngle(m_Ui->anglesSpinBox->value());
}
void ProjectionsDialog::srcProjActivated(int gui_index) {
    transforminfo->srcProjection = projectionList.at(gui_index);
}

void ProjectionsDialog::anglesAngularDestinationProj(int v) {
    ((AngularProjection *)(transforminfo->dstProjection))->setAngle(v);
}

void ProjectionsDialog::okClicked() {
    qDebug("Projective Transformation from %s to %s",
           transforminfo->srcProjection->getName(),
           transforminfo->dstProjection->getName());

    m_Ui->progressBar->setMaximum(0);
    m_Ui->progressBar->setMinimum(0);
    m_Ui->progressBar->setValue(-1);
    m_Ui->progressBar->show();
    m_Ui->cancelButton->setDisabled(true);
    m_Ui->okButton->setDisabled(true);

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    int xSize = original->getWidth();
    int ySize =
        static_cast<int>(xSize / transforminfo->dstProjection->getSizeRatio());
    transformed = new pfs::Frame(xSize, ySize);

    m_future = QtConcurrent::run(boost::bind(&worker, original, transformed,
                                             xSize, ySize, transforminfo));

    m_futureWatcher.setFuture(m_future);
}

void ProjectionsDialog::projectionFinished() {
    QApplication::restoreOverrideCursor();
    emit accept();
}
