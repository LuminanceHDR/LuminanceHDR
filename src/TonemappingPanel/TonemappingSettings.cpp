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
 *
 */

#include <QScrollArea>
#include <QSqlRecord>

#include <algorithm>

#include "Core/TonemappingOptions.h"
#include "PreviewPanel/PreviewLabel.h"
#include "TonemappingSettings.h"
#include "ui_TonemappingSettings.h"

bool compareByComment(PreviewLabel *l1, PreviewLabel *l2)
{
    QString s1 = l1->getComment(), s2 = l2->getComment();

    return s1 < s2;
}

bool compareByOperator(PreviewLabel *l1, PreviewLabel *l2)
{
    TonemappingOptions *opts1 = l1->getTonemappingOptions(), *opts2 = l2->getTonemappingOptions();
    TMOperator op1 = opts1->tmoperator, op2 = opts2->tmoperator;
    QString s1, s2;
    switch (op1) {
        case ashikhmin:
            s1 = "ashikmin";
        break;
        case drago:
            s1 = "drago";
        break;
        case durand:
            s1 = "durand";
        break;
        case fattal:
            s1 = "fattal";
        break;
        case mantiuk06:
            s1 = "mantiuk06";
        break;
        case mantiuk08:
            s1 = "mantiuk08";
        break;
        case pattanaik:
            s1 = "pattanaik";
        break;
        case reinhard02:
            s1 = "reinhard02";
        break;
        case reinhard05:
            s1 = "reinhard05";
        break;
    }
    switch (op2) {
        case ashikhmin:
            s2 = "ashikmin";
        break;
        case drago:
            s2 = "drago";
        break;
        case durand:
            s2 = "durand";
        break;
        case fattal:
            s2 = "fattal";
        break;
        case mantiuk06:
            s2 = "mantiuk06";
        break;
        case mantiuk08:
            s2 = "mantiuk08";
        break;
        case pattanaik:
            s2 = "pattanaik";
        break;
        case reinhard02:
            s2 = "reinhard02";
        break;
        case reinhard05:
            s2 = "reinhard05";
        break;
    }
    return s1 < s2;
}

TonemappingSettings::TonemappingSettings(QWidget *parent, pfs::Frame *frame) :
    QDialog(parent),
    m_frame(frame),
    m_modelPreviews(new QSqlQueryModel()),
    m_Ui(new Ui::TonemappingSettings)
{
    m_Ui->setupUi(this);
    
    m_Ui->splitter->setStretchFactor(0,1);
    m_Ui->splitter->setStretchFactor(1,5);

    m_previewSettings = new PreviewSettings(m_Ui->scrollArea);

    m_Ui->scrollArea->setWidgetResizable(true);

    m_Ui->scrollArea->setWidget(m_previewSettings);

    fillPreviews();

    if (m_Ui->listWidget->count() != 0) {    
        m_currentIndex = 0;
        m_Ui->listWidget->setCurrentRow(0);
        m_previewSettings->selectLabel(0);
    }
    else
        m_Ui->applyButton->setDisabled(true);

    sortPreviews(0); // by comment

    connect(m_Ui->listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(listWidgetChanged(int)));
    connect(m_Ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(sortPreviews(int)));
    connect(m_previewSettings, SIGNAL(triggered()), this, SLOT(accept()));
}

TonemappingSettings::~TonemappingSettings()
{
}

void TonemappingSettings::fillPreviews()
{
    int index = 0;
    QString sqlQuery;
    int origxsize = m_frame->getWidth();
    float pregamma;
    QString comment;

    sqlQuery = "SELECT *, 'ashikhmin' AS operator FROM ashikhmin";
    m_modelPreviews->setQuery(sqlQuery);

    bool simple;
    bool eq2;
    float lct;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoAshikhmin = new TonemappingOptions;
        comment = m_modelPreviews->record(selectedRow).value("comment").toString();
        simple = m_modelPreviews->record(selectedRow).value("simple").toBool();
        eq2 = m_modelPreviews->record(selectedRow).value("eq2").toBool();
        lct = m_modelPreviews->record(selectedRow).value("lct").toFloat();
        pregamma = m_modelPreviews->record(selectedRow).value("pregamma").toFloat();

        tmoAshikhmin->origxsize = origxsize;
        tmoAshikhmin->xsize = 120;
        tmoAshikhmin->pregamma = pregamma;
        tmoAshikhmin->tmoperator = ashikhmin;
        tmoAshikhmin->operator_options.ashikhminoptions.simple = simple;
        tmoAshikhmin->operator_options.ashikhminoptions.eq2 = eq2;
        tmoAshikhmin->operator_options.ashikhminoptions.lct = lct;

        PreviewLabel *previewLabelAshikhmin = new PreviewLabel(0, tmoAshikhmin, index++);
        previewLabelAshikhmin->setComment(comment);
        connect(previewLabelAshikhmin, SIGNAL(clicked(int)), m_previewSettings, SLOT(selectLabel(int)));
        connect(previewLabelAshikhmin, SIGNAL(clicked(int)), this, SLOT(updateListView(int)));
        m_previewSettings->addPreviewLabel(previewLabelAshikhmin);
        m_Ui->listWidget->addItem(comment);
    }

    sqlQuery = "SELECT *, 'drago' AS operator FROM drago";
    m_modelPreviews->setQuery(sqlQuery);

    float bias;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoDrago = new TonemappingOptions;
        comment = m_modelPreviews->record(selectedRow).value("comment").toString();
		bias = m_modelPreviews->record(selectedRow).value("bias").toFloat();
		pregamma = m_modelPreviews->record(selectedRow).value("pregamma").toFloat();

        tmoDrago->origxsize = origxsize;
        tmoDrago->xsize = 120;
        tmoDrago->pregamma = pregamma;
        tmoDrago->tmoperator = drago;
        tmoDrago->operator_options.dragooptions.bias = bias;

        PreviewLabel *previewLabelDrago = new PreviewLabel(0, tmoDrago, index++);
        previewLabelDrago->setComment(comment);
        connect(previewLabelDrago, SIGNAL(clicked(int)), m_previewSettings, SLOT(selectLabel(int)));
        connect(previewLabelDrago, SIGNAL(clicked(int)), this, SLOT(updateListView(int)));
        m_previewSettings->addPreviewLabel(previewLabelDrago);
        m_Ui->listWidget->addItem(comment);
    }

    sqlQuery = "SELECT *, 'durand' AS operator FROM durand";
    m_modelPreviews->setQuery(sqlQuery);

    float spatial,
            range,
             base;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoDurand = new TonemappingOptions;
        comment = m_modelPreviews->record(selectedRow).value("comment").toString();
		spatial  = m_modelPreviews->record(selectedRow).value("spatial").toFloat();
		range    = m_modelPreviews->record(selectedRow).value("range").toFloat();
		base     = m_modelPreviews->record(selectedRow).value("base").toFloat();
		pregamma = m_modelPreviews->record(selectedRow).value("pregamma").toFloat();

        tmoDurand->origxsize = origxsize;
        tmoDurand->xsize = 120;
        tmoDurand->pregamma = pregamma;
        tmoDurand->tmoperator = durand;
        tmoDurand->operator_options.durandoptions.spatial = spatial;
        tmoDurand->operator_options.durandoptions.range = range;
        tmoDurand->operator_options.durandoptions.base = base;

        PreviewLabel *previewLabelDurand = new PreviewLabel(0, tmoDurand, index++);
        previewLabelDurand->setComment(comment);
        connect(previewLabelDurand, SIGNAL(clicked(int)), m_previewSettings, SLOT(selectLabel(int)));
        connect(previewLabelDurand, SIGNAL(clicked(int)), this, SLOT(updateListView(int)));
        m_previewSettings->addPreviewLabel(previewLabelDurand);
        m_Ui->listWidget->addItem(comment);
    }

    sqlQuery = "SELECT *, 'fattal' AS operator FROM fattal";
    m_modelPreviews->setQuery(sqlQuery);

    float alpha,
           beta,
        colorSat,
        noiseReduction;
    bool fftsolver;
    
    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoFattal = new TonemappingOptions;
        comment = m_modelPreviews->record(selectedRow).value("comment").toString();
		alpha          = m_modelPreviews->record(selectedRow).value("alpha").toFloat();
		beta           = m_modelPreviews->record(selectedRow).value("beta").toFloat();
		colorSat       = m_modelPreviews->record(selectedRow).value("colorSaturation").toFloat();
		noiseReduction = m_modelPreviews->record(selectedRow).value("noiseReduction").toFloat();
		fftsolver      = !m_modelPreviews->record(selectedRow).value("oldFattal").toBool();
		pregamma       = m_modelPreviews->record(selectedRow).value("pregamma").toFloat();

        tmoFattal->origxsize = origxsize;
        tmoFattal->xsize = 120;
        tmoFattal->pregamma = pregamma;
        tmoFattal->tmoperator = fattal;
        tmoFattal->operator_options.fattaloptions.alpha = alpha;
        tmoFattal->operator_options.fattaloptions.beta = beta;
        tmoFattal->operator_options.fattaloptions.color = colorSat;
        tmoFattal->operator_options.fattaloptions.noiseredux = noiseReduction;
        tmoFattal->operator_options.fattaloptions.fftsolver = fftsolver;

        PreviewLabel *previewLabelFattal = new PreviewLabel(0, tmoFattal, index++);
        previewLabelFattal->setComment(comment);
        connect(previewLabelFattal, SIGNAL(clicked(int)), m_previewSettings, SLOT(selectLabel(int)));
        connect(previewLabelFattal, SIGNAL(clicked(int)), this, SLOT(updateListView(int)));
        m_previewSettings->addPreviewLabel(previewLabelFattal);
        m_Ui->listWidget->addItem(comment);
    }

    sqlQuery = "SELECT *, 'mantiuk06' AS operator FROM mantiuk06";
    m_modelPreviews->setQuery(sqlQuery);

    bool contrastEqualization;
    float contrastFactor;
    float saturationFactor;
    float detailFactor;
    
    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoMantiuk06 = new TonemappingOptions;
        comment = m_modelPreviews->record(selectedRow).value("comment").toString();
		contrastEqualization = m_modelPreviews->record(selectedRow).value("contrastEqualization").toBool();
		contrastFactor       = m_modelPreviews->record(selectedRow).value("contrastFactor").toFloat();
		saturationFactor     = m_modelPreviews->record(selectedRow).value("saturationFactor").toFloat();
		detailFactor         = m_modelPreviews->record(selectedRow).value("detailFactor").toFloat();
		pregamma             = m_modelPreviews->record(selectedRow).value("pregamma").toFloat();

        tmoMantiuk06->origxsize = origxsize;
        tmoMantiuk06->xsize = 120;
        tmoMantiuk06->pregamma = pregamma;
        tmoMantiuk06->tmoperator = mantiuk06;
        tmoMantiuk06->operator_options.mantiuk06options.contrastfactor = contrastFactor;
        tmoMantiuk06->operator_options.mantiuk06options.saturationfactor = saturationFactor;
        tmoMantiuk06->operator_options.mantiuk06options.detailfactor = detailFactor;
        tmoMantiuk06->operator_options.mantiuk06options.contrastequalization = contrastEqualization;

        PreviewLabel *previewLabelMantiuk06 = new PreviewLabel(0, tmoMantiuk06, index++);
        previewLabelMantiuk06->setComment(comment);
        connect(previewLabelMantiuk06, SIGNAL(clicked(int)), m_previewSettings, SLOT(selectLabel(int)));
        connect(previewLabelMantiuk06, SIGNAL(clicked(int)), this, SLOT(updateListView(int)));
        m_previewSettings->addPreviewLabel(previewLabelMantiuk06);
        m_Ui->listWidget->addItem(comment);
    }

    sqlQuery = "SELECT *, 'mantiuk08' AS operator FROM mantiuk08";
    m_modelPreviews->setQuery(sqlQuery);

    float colorSaturation,
          contrastEnhancement,
          luminanceLevel;
    bool manualLuminanceLevel;
    
    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoMantiuk08 = new TonemappingOptions;
        comment = m_modelPreviews->record(selectedRow).value("comment").toString();
		colorSaturation      = m_modelPreviews->record(selectedRow).value("colorSaturation").toFloat();
		contrastEnhancement  = m_modelPreviews->record(selectedRow).value("contrastEnhancement").toFloat();
		luminanceLevel       = m_modelPreviews->record(selectedRow).value("luminanceLevel").toFloat();
		manualLuminanceLevel = m_modelPreviews->record(selectedRow).value("manualLuminanceLevel").toBool();
		pregamma             = m_modelPreviews->record(selectedRow).value("pregamma").toFloat();

        tmoMantiuk08->origxsize = origxsize;
        tmoMantiuk08->xsize = 120;
        tmoMantiuk08->pregamma = pregamma;
        tmoMantiuk08->tmoperator = mantiuk08;
        tmoMantiuk08->operator_options.mantiuk08options.colorsaturation = colorSaturation;
        tmoMantiuk08->operator_options.mantiuk08options.contrastenhancement = contrastEnhancement;
        tmoMantiuk08->operator_options.mantiuk08options.luminancelevel = luminanceLevel;
        tmoMantiuk08->operator_options.mantiuk08options.setluminance = manualLuminanceLevel;

        PreviewLabel *previewLabelMantiuk08 = new PreviewLabel(0, tmoMantiuk08, index++);
        previewLabelMantiuk08->setComment(comment);
        connect(previewLabelMantiuk08, SIGNAL(clicked(int)), m_previewSettings, SLOT(selectLabel(int)));
        connect(previewLabelMantiuk08, SIGNAL(clicked(int)), this, SLOT(updateListView(int)));
        m_previewSettings->addPreviewLabel(previewLabelMantiuk08);
        m_Ui->listWidget->addItem(comment);
    }

    sqlQuery = "SELECT *, 'pattanaik' AS operator FROM pattanaik";
    m_modelPreviews->setQuery(sqlQuery);

    float multiplier,
          rod,
          cone;
    bool autolum,
         local;
    
    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoPattanaik = new TonemappingOptions;
        comment = m_modelPreviews->record(selectedRow).value("comment").toString();
		multiplier = m_modelPreviews->record(selectedRow).value("multiplier").toFloat();
		rod        = m_modelPreviews->record(selectedRow).value("rod").toFloat();
		cone       = m_modelPreviews->record(selectedRow).value("cone").toFloat();
		autolum    = m_modelPreviews->record(selectedRow).value("autolum").toBool();
		local      = m_modelPreviews->record(selectedRow).value("local").toBool();
		pregamma   = m_modelPreviews->record(selectedRow).value("pregamma").toFloat();

        tmoPattanaik->origxsize = origxsize;
        tmoPattanaik->xsize = 120;
        tmoPattanaik->pregamma = pregamma;
        tmoPattanaik->tmoperator = pattanaik;
        tmoPattanaik->operator_options.pattanaikoptions.autolum = autolum;
        tmoPattanaik->operator_options.pattanaikoptions.local = local;
        tmoPattanaik->operator_options.pattanaikoptions.cone = cone;
        tmoPattanaik->operator_options.pattanaikoptions.rod = rod;
        tmoPattanaik->operator_options.pattanaikoptions.multiplier = multiplier;

        PreviewLabel *previewLabelPattanaik = new PreviewLabel(0, tmoPattanaik, index++);
        previewLabelPattanaik->setComment(comment);
        connect(previewLabelPattanaik, SIGNAL(clicked(int)), m_previewSettings, SLOT(selectLabel(int)));
        connect(previewLabelPattanaik, SIGNAL(clicked(int)), this, SLOT(updateListView(int)));
        m_previewSettings->addPreviewLabel(previewLabelPattanaik);
        m_Ui->listWidget->addItem(comment);
    }

    sqlQuery = "SELECT *, 'reinhard02' AS operator FROM reinhard02";
    m_modelPreviews->setQuery(sqlQuery);

    bool scales;
    float key,
          phi;
    int irange,
        lower,
        upper;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoReinhard02 = new TonemappingOptions;
        comment  = m_modelPreviews->record(selectedRow).value("comment").toString();
		scales   = m_modelPreviews->record(selectedRow).value("scales").toBool();
		key      = m_modelPreviews->record(selectedRow).value("key").toFloat();
		phi      = m_modelPreviews->record(selectedRow).value("phi").toFloat();
		irange   = m_modelPreviews->record(selectedRow).value("range").toInt();
		lower    = m_modelPreviews->record(selectedRow).value("lower").toInt();
		upper    = m_modelPreviews->record(selectedRow).value("upper").toInt();
		pregamma = m_modelPreviews->record(selectedRow).value("pregamma").toFloat();

        tmoReinhard02->origxsize = origxsize;
        tmoReinhard02->xsize = 120;
        tmoReinhard02->pregamma = pregamma;
        tmoReinhard02->tmoperator = reinhard02;
        tmoReinhard02->operator_options.reinhard02options.scales = scales;
        tmoReinhard02->operator_options.reinhard02options.key = key;
        tmoReinhard02->operator_options.reinhard02options.phi = phi;
        tmoReinhard02->operator_options.reinhard02options.range = irange;
        tmoReinhard02->operator_options.reinhard02options.lower = lower;
        tmoReinhard02->operator_options.reinhard02options.upper = upper;

        PreviewLabel *previewLabelReinhard02 = new PreviewLabel(0, tmoReinhard02, index++);
        previewLabelReinhard02->setComment(comment);
        connect(previewLabelReinhard02, SIGNAL(clicked(int)), m_previewSettings, SLOT(selectLabel(int)));
        connect(previewLabelReinhard02, SIGNAL(clicked(int)), this, SLOT(updateListView(int)));
        m_previewSettings->addPreviewLabel(previewLabelReinhard02);
        m_Ui->listWidget->addItem(comment);
    }

    sqlQuery = "SELECT *, 'reinhard05' AS operator FROM reinhard05";
    m_modelPreviews->setQuery(sqlQuery);

    float brightness,
          chromaticAdaptation,
          lightAdaptation;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoReinhard05 = new TonemappingOptions;
        comment             = m_modelPreviews->record(selectedRow).value("comment").toString();
		brightness          = m_modelPreviews->record(selectedRow).value("brightness").toFloat();
		chromaticAdaptation = m_modelPreviews->record(selectedRow).value("chromaticAdaptation").toFloat();
		lightAdaptation     = m_modelPreviews->record(selectedRow).value("lightAdaptation").toFloat();
		pregamma            = m_modelPreviews->record(selectedRow).value("pregamma").toFloat();

        tmoReinhard05->origxsize = origxsize;
        tmoReinhard05->xsize = 120;
        tmoReinhard05->pregamma = pregamma;
        tmoReinhard05->tmoperator = reinhard05;
        tmoReinhard05->operator_options.reinhard05options.brightness = brightness;
        tmoReinhard05->operator_options.reinhard05options.chromaticAdaptation = chromaticAdaptation;
        tmoReinhard05->operator_options.reinhard05options.lightAdaptation = lightAdaptation;

        PreviewLabel *previewLabelReinhard05 = new PreviewLabel(0, tmoReinhard05, index++);
        previewLabelReinhard05->setComment(comment);
        connect(previewLabelReinhard05, SIGNAL(clicked(int)), m_previewSettings, SLOT(selectLabel(int)));
        connect(previewLabelReinhard05, SIGNAL(clicked(int)), this, SLOT(updateListView(int)));
        m_previewSettings->addPreviewLabel(previewLabelReinhard05);
        m_Ui->listWidget->addItem(comment);
    }
    m_previewSettings->updatePreviews(m_frame);
}

void TonemappingSettings::listWidgetChanged(int row) {
    m_currentIndex = row;
    m_previewSettings->selectLabel(row);
}

void TonemappingSettings::updateListView(int row) {
    m_currentIndex = row;
    m_Ui->listWidget->setCurrentRow(row);
}

TonemappingOptions *TonemappingSettings::getTonemappingOptions() {
    return m_previewSettings->getPreviewLabel(m_currentIndex)->getTonemappingOptions();
}

void TonemappingSettings::sortPreviews(int index) {
    m_Ui->listWidget->clear();
    QList<PreviewLabel *> l;
    int listSize = m_previewSettings->getSize();
    for (int i = 0; i < listSize; i++) {
        l.append(m_previewSettings->getPreviewLabel(i));
    }
    m_previewSettings->clear();
    std::sort(l.begin(), l.end(), (index == 0) ? compareByComment : compareByOperator);
    for (int i = 0; i < listSize; i++) {
        PreviewLabel *pl = l.at(i);
        pl->setIndex(i);
        m_previewSettings->addPreviewLabel(pl);
        m_Ui->listWidget->addItem(pl->getComment());
    } 
    m_Ui->listWidget->setCurrentRow(0);
    m_previewSettings->selectLabel(0);
    m_currentIndex = 0;
}
