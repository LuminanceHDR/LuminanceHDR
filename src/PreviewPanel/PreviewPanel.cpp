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

#include "PreviewPanel.h"
#include "Threads/TMOFactory.h"
#include "Threads/TMOThread.h"
#include "Filter/pfssize.h"

#define PREVIEW_WIDTH 128

PreviewPanel::PreviewPanel(QWidget *parent):
        QWidget(parent)
{
    setupUi(this);

	luminance_options=LuminanceOptions::getInstance();

    labelMantiuk06 = new PreviewLabel(frameMantiuk06, 0);
    labelMantiuk06->setText("Mantiuk '06");

    labelMantiuk08 = new PreviewLabel(frameMantiuk08, 1);
    labelMantiuk08->setText("Mantiuk '08");

    labelFattal = new PreviewLabel(frameFattal, 2);
    labelFattal->setText("Fattal");

    labelDrago = new PreviewLabel(frameDrago, 3);
    labelDrago->setText("Drago");

    labelDurand = new PreviewLabel(frameDurand, 4);
    labelDurand->setText("Durand");

    labelReinhard02= new PreviewLabel(frameReinhard02, 5);
    labelReinhard02->setText("Reinhard '02");

    labelReinhard05 = new PreviewLabel(frameReinhard05, 6);
    labelReinhard05->setText("Reinhard '05");

    labelAshikhmin = new PreviewLabel(frameAshikhmin, 7);
    labelAshikhmin->setText("Ashikhmin");

    labelPattanaik = new PreviewLabel(framePattanaik, 8);
    labelPattanaik->setText("Pattanaik");

    setupConnections();
    buildPreviewsDataStructure();

    is_frame_set = false;
    current_frame = NULL;
    opts = new TonemappingOptions;
    original_width_frame = 0;
}

PreviewPanel::~PreviewPanel()
{
    qDebug() << "PreviewPanel::~PreviewPanel()";
    if (is_frame_set)
        delete current_frame;
}

void PreviewPanel::buildPreviewsDataStructure()
{
    //id_tm_operator.insert()
}

void PreviewPanel::setupConnections()
{
    connect(labelMantiuk06, SIGNAL(clicked(int)), this, SLOT(tonemapPreview(int)));
    connect(labelMantiuk08, SIGNAL(clicked(int)), this, SLOT(tonemapPreview(int)));
    connect(labelFattal, SIGNAL(clicked(int)), this, SLOT(tonemapPreview(int)));
    connect(labelDrago, SIGNAL(clicked(int)), this, SLOT(tonemapPreview(int)));
    connect(labelDurand, SIGNAL(clicked(int)), this, SLOT(tonemapPreview(int)));
    connect(labelReinhard02, SIGNAL(clicked(int)), this, SLOT(tonemapPreview(int)));
    connect(labelReinhard05, SIGNAL(clicked(int)), this, SLOT(tonemapPreview(int)));
    connect(labelAshikhmin, SIGNAL(clicked(int)), this, SLOT(tonemapPreview(int)));
    connect(labelPattanaik, SIGNAL(clicked(int)), this, SLOT(tonemapPreview(int)));
}

void PreviewPanel::setPixmap(const QPixmap &p, int n)
{
    switch (n) {
    case 0:
        labelMantiuk06->setPixmap(p);
        labelMantiuk06->adjustSize();
        break;
    case 1:
        labelMantiuk08->setPixmap(p);
        labelMantiuk08->adjustSize();
        break;
    case 2:
        labelFattal->setPixmap(p);
        labelFattal->adjustSize();
        break;
    case 3:
        labelDrago->setPixmap(p);
        labelDrago->adjustSize();
        break;
    case 4:
        labelDurand->setPixmap(p);
        labelDurand->adjustSize();
        break;
    case 5:
        labelReinhard02->setPixmap(p);
        labelReinhard02->adjustSize();
        break;
    case 6:
        labelReinhard05->setPixmap(p);
        labelReinhard05->adjustSize();
        break;
    case 7:
        labelAshikhmin->setPixmap(p);
        labelAshikhmin->adjustSize();
        break;
    case 8:
        labelPattanaik->setPixmap(p);
        labelPattanaik->adjustSize();
        break;
    }
}

void PreviewPanel::updatePreviews(pfs::Frame* frame)
{

    // I get as a parameter a pfs::Frame
    if (is_frame_set)
        delete current_frame;

    original_width_frame = frame->getWidth();
    current_frame = pfs::resizeFrame(frame, PREVIEW_WIDTH);

    // TODO : implementation using Concurrent::map
    generatePreviews();
    // 1. make a resized copy
    // 2. set again all the parameters
    // 3. call a function that
    // for each TM operator does:
    // a/ copy of the reference frame
    // b/ tm
    // c/ updates the previewer
}

void PreviewPanel::generatePreviews()
{
    qDebug() << "MainWindow::generatePreviews()";

    if (current_frame == NULL)
        return;

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    opts->origxsize = PREVIEW_WIDTH;
    opts->xsize = PREVIEW_WIDTH;
    opts->pregamma = 1.0;
    opts->tonemapSelection = false;
    opts->tonemapOriginal = false;

    opts->tmoperator = mantiuk06;
    opts->tmoperator_str = "Mantiuk '06";

    TMOThread *thread = TMOFactory::getTMOThread(opts->tmoperator, current_frame, opts);
    thread->set_image_number(0);
    thread->set_mode(TMO_PREVIEW);
    connect(thread, SIGNAL(imageComputed(QImage*, int)), this, SLOT(addSmallPreviewResult(QImage*, int)));
    connect(thread, SIGNAL(tmo_error(const char *)), this, SLOT(showError(const char *)));
    connect(thread, SIGNAL(deleteMe(TMOThread *)), this, SLOT(deleteTMOThread(TMOThread *)));
    thread->start();

    opts->tmoperator = mantiuk08;
    opts->tmoperator_str = "Mantiuk '08";

    thread = TMOFactory::getTMOThread(opts->tmoperator, current_frame, opts);
    thread->set_image_number(1);
    thread->set_mode(TMO_PREVIEW);
    connect(thread, SIGNAL(imageComputed(QImage*, int)), this, SLOT(addSmallPreviewResult(QImage*, int)));
    connect(thread, SIGNAL(tmo_error(const char *)), this, SLOT(showError(const char *)));
    connect(thread, SIGNAL(deleteMe(TMOThread *)), this, SLOT(deleteTMOThread(TMOThread *)));
    thread->start();

    opts->tmoperator = fattal;
    opts->tmoperator_str = "Fattal";

    thread = TMOFactory::getTMOThread(opts->tmoperator, current_frame, opts);
    thread->set_image_number(2);
    thread->set_mode(TMO_PREVIEW);
    connect(thread, SIGNAL(imageComputed(QImage*, int)), this, SLOT(addSmallPreviewResult(QImage*, int)));
    connect(thread, SIGNAL(tmo_error(const char *)), this, SLOT(showError(const char *)));
    connect(thread, SIGNAL(deleteMe(TMOThread *)), this, SLOT(deleteTMOThread(TMOThread *)));
    thread->start();

    opts->tmoperator = drago;
    opts->tmoperator_str = "Drago";

    thread = TMOFactory::getTMOThread(opts->tmoperator, current_frame, opts);
    thread->set_image_number(3);
    thread->set_mode(TMO_PREVIEW);
    connect(thread, SIGNAL(imageComputed(QImage*, int)), this, SLOT(addSmallPreviewResult(QImage*, int)));
    connect(thread, SIGNAL(tmo_error(const char *)), this, SLOT(showError(const char *)));
    connect(thread, SIGNAL(deleteMe(TMOThread *)), this, SLOT(deleteTMOThread(TMOThread *)));
    thread->start();

    opts->tmoperator = durand;
    opts->tmoperator_str = "Durand";

    thread = TMOFactory::getTMOThread(opts->tmoperator, current_frame, opts);
    thread->set_image_number(4);
    thread->set_mode(TMO_PREVIEW);
    connect(thread, SIGNAL(imageComputed(QImage*, int)), this, SLOT(addSmallPreviewResult(QImage*, int)));
    connect(thread, SIGNAL(tmo_error(const char *)), this, SLOT(showError(const char *)));
    connect(thread, SIGNAL(deleteMe(TMOThread *)), this, SLOT(deleteTMOThread(TMOThread *)));
    thread->start();

    opts->tmoperator = reinhard02;
    opts->tmoperator_str = "Reinhard '02";

    thread = TMOFactory::getTMOThread(opts->tmoperator, current_frame, opts);
    thread->set_image_number(5);
    thread->set_mode(TMO_PREVIEW);
    connect(thread, SIGNAL(imageComputed(QImage*, int)), this, SLOT(addSmallPreviewResult(QImage*, int)));
    connect(thread, SIGNAL(tmo_error(const char *)), this, SLOT(showError(const char *)));
    connect(thread, SIGNAL(deleteMe(TMOThread *)), this, SLOT(deleteTMOThread(TMOThread *)));
    thread->start();

    opts->tmoperator = reinhard05;
    opts->tmoperator_str = "Reinhard '05";

    thread = TMOFactory::getTMOThread(opts->tmoperator, current_frame, opts);
    thread->set_image_number(6);
    thread->set_mode(TMO_PREVIEW);
    connect(thread, SIGNAL(imageComputed(QImage*, int)), this, SLOT(addSmallPreviewResult(QImage*, int)));
    connect(thread, SIGNAL(tmo_error(const char *)), this, SLOT(showError(const char *)));
    connect(thread, SIGNAL(deleteMe(TMOThread *)), this, SLOT(deleteTMOThread(TMOThread *)));
    thread->start();

    opts->tmoperator = ashikhmin;
    opts->tmoperator_str = "Ashikhmin";

    thread = TMOFactory::getTMOThread(opts->tmoperator, current_frame, opts);
    thread->set_image_number(7);
    thread->set_mode(TMO_PREVIEW);
    connect(thread, SIGNAL(imageComputed(QImage*, int)), this, SLOT(addSmallPreviewResult(QImage*, int)));
    connect(thread, SIGNAL(tmo_error(const char *)), this, SLOT(showError(const char *)));
    connect(thread, SIGNAL(deleteMe(TMOThread *)), this, SLOT(deleteTMOThread(TMOThread *)));
    thread->start();

    opts->tmoperator = pattanaik;
    opts->tmoperator_str = "Pattanaik";

    thread = TMOFactory::getTMOThread(opts->tmoperator, current_frame, opts);
    thread->set_image_number(8);
    thread->set_mode(TMO_PREVIEW);
    connect(thread, SIGNAL(imageComputed(QImage*, int)), this, SLOT(addSmallPreviewResult(QImage*, int)));
    connect(thread, SIGNAL(tmo_error(const char *)), this, SLOT(showError(const char *)));
    connect(thread, SIGNAL(deleteMe(TMOThread *)), this, SLOT(deleteTMOThread(TMOThread *)));
    thread->start();

    QApplication::restoreOverrideCursor();
}

void PreviewPanel::deleteTMOThread(TMOThread *th)
{
    delete th;
}

void PreviewPanel::addSmallPreviewResult(QImage *img, int n)
{
    qDebug() << "addSmallPreviewResult(" << n << ")";

    this->setPixmap( QPixmap::fromImage(*img), n);
}

void PreviewPanel::tonemapPreview(int n)
{
    if (current_frame == NULL)
        return;

    opts->origxsize = original_width_frame;
    opts->xsize = luminance_options->previews_width;
    opts->pregamma = 1.0;
    opts->tonemapSelection = false;
    opts->tonemapOriginal = false;

    switch (n) {
    case 0:
        opts->tmoperator = mantiuk06;
        opts->tmoperator_str = "Mantiuk '06";
        emit startTonemapping(opts);
        break;
    case 1:
        opts->tmoperator = mantiuk08;
        opts->tmoperator_str = "Mantiuk '08";
        emit startTonemapping(opts);
        break;
    case 2:
        opts->tmoperator = fattal;
        opts->tmoperator_str = "Fattal";
        emit startTonemapping(opts);
        break;
    case 3:
        opts->tmoperator = drago;
        opts->tmoperator_str = "Drago";
        emit startTonemapping(opts);
        break;
    case 4:
        opts->tmoperator = durand;
        opts->tmoperator_str = "Durand";
        emit startTonemapping(opts);
        break;
    case 5:
        opts->tmoperator = reinhard02;
        opts->tmoperator_str = "Reinhard '02";
        emit startTonemapping(opts);
        break;
    case 6:
        opts->tmoperator = reinhard05;
        opts->tmoperator_str = "Reinhard '05";
        emit startTonemapping(opts);
        break;
    case 7:
        opts->tmoperator = ashikhmin;
        opts->tmoperator_str = "Ashikhmin";
        emit startTonemapping(opts);
        break;
    case 8:
        opts->tmoperator = pattanaik;
        opts->tmoperator_str = "Pattanaik";
        emit startTonemapping(opts);
        break;
    }
}

void PreviewPanel::showError(const char *error)
{
	qDebug() << error;
}
