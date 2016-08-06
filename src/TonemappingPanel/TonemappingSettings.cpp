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

#include <QDebug>

#include <QScrollArea>
#include <QSqlRecord>

#include <algorithm>

#include "Core/TonemappingOptions.h"
#include "PreviewPanel/PreviewLabel.h"
#include "TonemappingSettings.h"
#include "ui_TonemappingSettings.h"

namespace // anoymous namespace
{
const int PREVIEW_WIDTH = PREVIEW_WIDTH;
const int PREVIEW_HEIGHT = 100;

bool compareByComment(PreviewLabel *l1, PreviewLabel *l2)
{
    QString s1 = l1->getComment(), s2 = l2->getComment();
    return s1 < s2;
}

bool compareByOperator(PreviewLabel *l1, PreviewLabel *l2)
{
    TonemappingOptions *opts1 = l1->getTonemappingOptions(), *opts2 = l2->getTonemappingOptions();
    return opts1->getPostfix() < opts2->getPostfix();
}

bool compareByMostUsefulOperators(PreviewLabel *l1, PreviewLabel *l2)
{
    TonemappingOptions *opts1 = l1->getTonemappingOptions(), *opts2 = l2->getTonemappingOptions();
    return opts1->getRatingForOperator() < opts2->getRatingForOperator();
}


}

TonemappingSettings::TonemappingSettings(QWidget *parent, pfs::Frame *frame) :
    QDialog(parent),
    m_frame(frame),
    m_modelPreviews(new QSqlQueryModel()),
    m_wantsTonemap(false),
    m_Ui(new Ui::TonemappingSettings)
{
    m_Ui->setupUi(this);
    
    m_Ui->splitter->setStretchFactor(0,1);
    m_Ui->splitter->setStretchFactor(1,10);

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
    {
        m_Ui->applyButton->setDisabled(true);
        m_Ui->btnTonemap->setDisabled(true);
    }

    sortPreviews(0); // by comment

    connect(m_Ui->listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(listWidgetChanged(int)));
    connect(m_Ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(sortPreviews(int)));
    connect(m_previewSettings, SIGNAL(triggered()), this, SLOT(accept()));
}

TonemappingSettings::~TonemappingSettings()
{
    qDeleteAll(m_previewLabelList);
}

void TonemappingSettings::fillPreviews()
{
    int index = 0;
    QString sqlQuery;
    int origxsize = m_frame->getWidth();

    sqlQuery = "SELECT *, 'ashikhmin' AS operator FROM ashikhmin";
    m_modelPreviews->setQuery(sqlQuery);

    bool simple;
    bool eq2;
    float lct;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoAshikhmin = new TonemappingOptions;
        simple = m_modelPreviews->record(selectedRow).value("simple").toBool();
        eq2 = m_modelPreviews->record(selectedRow).value("eq2").toBool();
        lct = m_modelPreviews->record(selectedRow).value("lct").toFloat();

		fillCommonValues(tmoAshikhmin, origxsize, PREVIEW_WIDTH, ashikhmin, m_modelPreviews->record(selectedRow));

		tmoAshikhmin->operator_options.ashikhminoptions.simple = simple;
        tmoAshikhmin->operator_options.ashikhminoptions.eq2 = eq2;
        tmoAshikhmin->operator_options.ashikhminoptions.lct = lct;

		addPreview(new PreviewLabel(0, tmoAshikhmin, index++), m_modelPreviews->record(selectedRow));
    }

    sqlQuery = "SELECT *, 'drago' AS operator FROM drago";
    m_modelPreviews->setQuery(sqlQuery);

    float bias;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoDrago = new TonemappingOptions;
		bias = m_modelPreviews->record(selectedRow).value("bias").toFloat();

		fillCommonValues(tmoDrago, origxsize, PREVIEW_WIDTH, drago, m_modelPreviews->record(selectedRow));

		tmoDrago->operator_options.dragooptions.bias = bias;

		addPreview(new PreviewLabel(0, tmoDrago, index++), m_modelPreviews->record(selectedRow));
    }

    sqlQuery = "SELECT *, 'durand' AS operator FROM durand";
    m_modelPreviews->setQuery(sqlQuery);

    float spatial,
            range,
             base;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoDurand = new TonemappingOptions;
		spatial  = m_modelPreviews->record(selectedRow).value("spatial").toFloat();
		range    = m_modelPreviews->record(selectedRow).value("range").toFloat();
		base     = m_modelPreviews->record(selectedRow).value("base").toFloat();

		fillCommonValues(tmoDurand, origxsize, PREVIEW_WIDTH, durand, m_modelPreviews->record(selectedRow));

		tmoDurand->operator_options.durandoptions.spatial = spatial;
        tmoDurand->operator_options.durandoptions.range = range;
        tmoDurand->operator_options.durandoptions.base = base;

		addPreview(new PreviewLabel(0, tmoDurand, index++), m_modelPreviews->record(selectedRow));
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
		alpha          = m_modelPreviews->record(selectedRow).value("alpha").toFloat();
		beta           = m_modelPreviews->record(selectedRow).value("beta").toFloat();
		colorSat       = m_modelPreviews->record(selectedRow).value("colorSaturation").toFloat();
		noiseReduction = m_modelPreviews->record(selectedRow).value("noiseReduction").toFloat();
		fftsolver      = !m_modelPreviews->record(selectedRow).value("oldFattal").toBool();

		fillCommonValues(tmoFattal, origxsize, PREVIEW_WIDTH, fattal, m_modelPreviews->record(selectedRow));

		tmoFattal->operator_options.fattaloptions.alpha = alpha;
        tmoFattal->operator_options.fattaloptions.beta = beta;
        tmoFattal->operator_options.fattaloptions.color = colorSat;
        tmoFattal->operator_options.fattaloptions.noiseredux = noiseReduction;
        tmoFattal->operator_options.fattaloptions.fftsolver = fftsolver;

		addPreview(new PreviewLabel(0, tmoFattal, index++), m_modelPreviews->record(selectedRow));
    }

    sqlQuery = "SELECT *, 'ferradans' AS operator FROM ferradans";
    m_modelPreviews->setQuery(sqlQuery);

    float rho,
           inv_alpha;
    
    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoFerradans = new TonemappingOptions;
		rho          = m_modelPreviews->record(selectedRow).value("rho").toFloat();
		inv_alpha    = m_modelPreviews->record(selectedRow).value("inv_alpha").toFloat();

		fillCommonValues(tmoFerradans, origxsize, PREVIEW_WIDTH, ferradans, m_modelPreviews->record(selectedRow));

		tmoFerradans->operator_options.ferradansoptions.rho = rho;
        tmoFerradans->operator_options.ferradansoptions.inv_alpha = inv_alpha;

		addPreview(new PreviewLabel(0, tmoFerradans, index++), m_modelPreviews->record(selectedRow));
    }

    sqlQuery = "SELECT *, 'mantiuk06' AS operator FROM mantiuk06";
    m_modelPreviews->setQuery(sqlQuery);

    bool contrastEqualization;
    float contrastFactor;
    float saturationFactor;
    float detailFactor;
    
    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoMantiuk06 = new TonemappingOptions;
		contrastEqualization = m_modelPreviews->record(selectedRow).value("contrastEqualization").toBool();
		contrastFactor       = m_modelPreviews->record(selectedRow).value("contrastFactor").toFloat();
		saturationFactor     = m_modelPreviews->record(selectedRow).value("saturationFactor").toFloat();
		detailFactor         = m_modelPreviews->record(selectedRow).value("detailFactor").toFloat();

		fillCommonValues(tmoMantiuk06, origxsize, PREVIEW_WIDTH, mantiuk06, m_modelPreviews->record(selectedRow));

		tmoMantiuk06->operator_options.mantiuk06options.contrastfactor = contrastFactor;
        tmoMantiuk06->operator_options.mantiuk06options.saturationfactor = saturationFactor;
        tmoMantiuk06->operator_options.mantiuk06options.detailfactor = detailFactor;
        tmoMantiuk06->operator_options.mantiuk06options.contrastequalization = contrastEqualization;

		addPreview(new PreviewLabel(0, tmoMantiuk06, index++), m_modelPreviews->record(selectedRow));
    }

    sqlQuery = "SELECT *, 'mantiuk08' AS operator FROM mantiuk08";
    m_modelPreviews->setQuery(sqlQuery);

    float colorSaturation,
          contrastEnhancement,
          luminanceLevel;
    bool manualLuminanceLevel;
    
    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoMantiuk08 = new TonemappingOptions;
		colorSaturation      = m_modelPreviews->record(selectedRow).value("colorSaturation").toFloat();
		contrastEnhancement  = m_modelPreviews->record(selectedRow).value("contrastEnhancement").toFloat();
		luminanceLevel       = m_modelPreviews->record(selectedRow).value("luminanceLevel").toFloat();
		manualLuminanceLevel = m_modelPreviews->record(selectedRow).value("manualLuminanceLevel").toBool();

		fillCommonValues(tmoMantiuk08, origxsize, PREVIEW_WIDTH, mantiuk08, m_modelPreviews->record(selectedRow));

		tmoMantiuk08->operator_options.mantiuk08options.colorsaturation = colorSaturation;
        tmoMantiuk08->operator_options.mantiuk08options.contrastenhancement = contrastEnhancement;
        tmoMantiuk08->operator_options.mantiuk08options.luminancelevel = luminanceLevel;
        tmoMantiuk08->operator_options.mantiuk08options.setluminance = manualLuminanceLevel;

		addPreview(new PreviewLabel(0, tmoMantiuk08, index++), m_modelPreviews->record(selectedRow));
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
		multiplier = m_modelPreviews->record(selectedRow).value("multiplier").toFloat();
		rod        = m_modelPreviews->record(selectedRow).value("rod").toFloat();
		cone       = m_modelPreviews->record(selectedRow).value("cone").toFloat();
		autolum    = m_modelPreviews->record(selectedRow).value("autolum").toBool();
		local      = m_modelPreviews->record(selectedRow).value("local").toBool();

		fillCommonValues(tmoPattanaik, origxsize, PREVIEW_WIDTH, pattanaik, m_modelPreviews->record(selectedRow));

        tmoPattanaik->operator_options.pattanaikoptions.autolum = autolum;
        tmoPattanaik->operator_options.pattanaikoptions.local = local;
        tmoPattanaik->operator_options.pattanaikoptions.cone = cone;
        tmoPattanaik->operator_options.pattanaikoptions.rod = rod;
        tmoPattanaik->operator_options.pattanaikoptions.multiplier = multiplier;

		addPreview(new PreviewLabel(0, tmoPattanaik, index++), m_modelPreviews->record(selectedRow));
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
		scales   = m_modelPreviews->record(selectedRow).value("scales").toBool();
		key      = m_modelPreviews->record(selectedRow).value("key").toFloat();
		phi      = m_modelPreviews->record(selectedRow).value("phi").toFloat();
		irange   = m_modelPreviews->record(selectedRow).value("range").toInt();
		lower    = m_modelPreviews->record(selectedRow).value("lower").toInt();
		upper    = m_modelPreviews->record(selectedRow).value("upper").toInt();

		fillCommonValues(tmoReinhard02, origxsize, PREVIEW_WIDTH, reinhard02, m_modelPreviews->record(selectedRow));

        tmoReinhard02->operator_options.reinhard02options.scales = scales;
        tmoReinhard02->operator_options.reinhard02options.key = key;
        tmoReinhard02->operator_options.reinhard02options.phi = phi;
        tmoReinhard02->operator_options.reinhard02options.range = irange;
        tmoReinhard02->operator_options.reinhard02options.lower = lower;
        tmoReinhard02->operator_options.reinhard02options.upper = upper;

		addPreview(new PreviewLabel(0, tmoReinhard02, index++), m_modelPreviews->record(selectedRow));
    }

    sqlQuery = "SELECT *, 'reinhard05' AS operator FROM reinhard05";
    m_modelPreviews->setQuery(sqlQuery);

    float brightness,
          chromaticAdaptation,
          lightAdaptation;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount(); selectedRow++) {
        TonemappingOptions *tmoReinhard05 = new TonemappingOptions;
		brightness          = m_modelPreviews->record(selectedRow).value("brightness").toFloat();
		chromaticAdaptation = m_modelPreviews->record(selectedRow).value("chromaticAdaptation").toFloat();
		lightAdaptation     = m_modelPreviews->record(selectedRow).value("lightAdaptation").toFloat();

		fillCommonValues(tmoReinhard05, origxsize, PREVIEW_WIDTH, reinhard05, m_modelPreviews->record(selectedRow));

        tmoReinhard05->operator_options.reinhard05options.brightness = brightness;
        tmoReinhard05->operator_options.reinhard05options.chromaticAdaptation = chromaticAdaptation;
        tmoReinhard05->operator_options.reinhard05options.lightAdaptation = lightAdaptation;

		addPreview(new PreviewLabel(0, tmoReinhard05, index++), m_modelPreviews->record(selectedRow));
    }
    m_previewSettings->updatePreviews(m_frame);
}

void TonemappingSettings::addPreview(PreviewLabel* previewLabel, const QSqlRecord& record)
{
	QString comment = record.value("comment").toString();

	m_previewLabelList.append(previewLabel);

	previewLabel->setComment(comment);
	connect(previewLabel, SIGNAL(clicked(int)), m_previewSettings, SLOT(selectLabel(int)));
	connect(previewLabel, SIGNAL(clicked(int)), this, SLOT(updateListView(int)));
	connect(previewLabel, SIGNAL(clicked(TonemappingOptions *)), this, SLOT(tonemapPreview(TonemappingOptions *)));
	m_previewSettings->addPreviewLabel(previewLabel);
	m_Ui->listWidget->addItem(comment);
}

void TonemappingSettings::fillCommonValues(TonemappingOptions * tmOptions, int origxsize, int previewWidth, TMOperator tOperator, const QSqlRecord& record)
{
	float pregamma = record.value("pregamma").toFloat();

    tmOptions->origxsize = origxsize;
    tmOptions->xsize = previewWidth;
    tmOptions->pregamma = pregamma;
    tmOptions->tmoperator = tOperator;
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
    switch (index) {
        case 0:
            std::sort(l.begin(), l.end(), compareByComment);
        break;
        case 1:
            std::sort(l.begin(), l.end(), compareByOperator);
        break;
        case 2:
            std::sort(l.begin(), l.end(), compareByMostUsefulOperators);
        break;
    }
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

void TonemappingSettings::tonemapPreview(TonemappingOptions* opt)
{
    m_wantsTonemap =  true;
    accept();
}

void TonemappingSettings::on_btnTonemap_clicked()
{
    m_wantsTonemap =  true;
    accept();
}
