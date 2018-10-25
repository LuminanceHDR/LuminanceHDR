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

#include <Core/TonemappingOptions.h>
#include <PreviewPanel/PreviewLabel.h>
#include <TonemappingPanel/TonemappingSettings.h>
#include <TonemappingPanel/ui_TonemappingSettings.h>

namespace  // anoymous namespace
{
const int PREVIEW_WIDTH = PREVIEW_WIDTH;

bool compareByComment(PreviewLabel *l1, PreviewLabel *l2) {
    QString s1 = l1->getComment(), s2 = l2->getComment();
    return s1 < s2;
}

bool compareByOperator(PreviewLabel *l1, PreviewLabel *l2) {
    TonemappingOptions *opts1 = l1->getTonemappingOptions(),
                       *opts2 = l2->getTonemappingOptions();
    return opts1->getPostfix() < opts2->getPostfix();
}

bool compareByMostUsefulOperators(PreviewLabel *l1, PreviewLabel *l2) {
    TonemappingOptions *opts1 = l1->getTonemappingOptions(),
                       *opts2 = l2->getTonemappingOptions();
    return opts1->getRatingForOperator() < opts2->getRatingForOperator();
}
}

TonemappingSettings::TonemappingSettings(QWidget *parent, pfs::Frame *frame, QString conn)
    : QDialog(parent),
      m_frame(frame),
      m_modelPreviews(new QSqlQueryModel()),
      m_wantsTonemap(false),
      m_databaseconnection(conn),
      m_Ui(new Ui::TonemappingSettings) {
    m_Ui->setupUi(this);

    m_Ui->splitter->setStretchFactor(0, 1);
    m_Ui->splitter->setStretchFactor(1, 10);

    m_previewSettings = new PreviewSettings(m_Ui->scrollArea);

    m_Ui->scrollArea->setWidgetResizable(true);

    m_Ui->scrollArea->setWidget(m_previewSettings);

    fillPreviews();

    if (m_Ui->listWidget->count() != 0) {
        m_currentIndex = 0;
        m_Ui->listWidget->setCurrentRow(0);
        m_previewSettings->selectLabel(0);
    } else {
        m_Ui->applyButton->setDisabled(true);
        m_Ui->btnTonemap->setDisabled(true);
    }

    sortPreviews(0);  // by comment

    connect(m_Ui->listWidget, &QListWidget::currentRowChanged, this,
            &TonemappingSettings::listWidgetChanged);
    connect(m_Ui->comboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(sortPreviews(int)));
    connect(m_previewSettings, &PreviewSettings::triggered, this,
            &QDialog::accept);
}

TonemappingSettings::~TonemappingSettings() { qDeleteAll(m_previewLabelList); }

void TonemappingSettings::fillPreviews() {
    int index = 0;
    QSqlDatabase db = QSqlDatabase::database(m_databaseconnection);
    QString sqlQuery;
    int origxsize = m_frame->getWidth();

    sqlQuery =
        QStringLiteral("SELECT *, 'ashikhmin' AS operator FROM ashikhmin");
    m_modelPreviews->setQuery(sqlQuery, db);

    bool simple;
    bool eq2;
    float lct;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount();
         selectedRow++) {
        TonemappingOptions *tmoAshikhmin = new TonemappingOptions;
        simple = m_modelPreviews->record(selectedRow)
                     .value(QStringLiteral("simple"))
                     .toBool();
        eq2 = m_modelPreviews->record(selectedRow)
                  .value(QStringLiteral("eq2"))
                  .toBool();
        lct = m_modelPreviews->record(selectedRow)
                  .value(QStringLiteral("lct"))
                  .toFloat();

        fillCommonValues(tmoAshikhmin, origxsize, PREVIEW_WIDTH, ashikhmin,
                         m_modelPreviews->record(selectedRow));

        tmoAshikhmin->operator_options.ashikhminoptions.simple = simple;
        tmoAshikhmin->operator_options.ashikhminoptions.eq2 = eq2;
        tmoAshikhmin->operator_options.ashikhminoptions.lct = lct;

        addPreview(new PreviewLabel(0, tmoAshikhmin, index++),
                   m_modelPreviews->record(selectedRow));
    }

    sqlQuery = QStringLiteral("SELECT *, 'drago' AS operator FROM drago");
    m_modelPreviews->setQuery(sqlQuery, db);

    float bias;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount();
         selectedRow++) {
        TonemappingOptions *tmoDrago = new TonemappingOptions;
        bias = m_modelPreviews->record(selectedRow)
                   .value(QStringLiteral("bias"))
                   .toFloat();

        fillCommonValues(tmoDrago, origxsize, PREVIEW_WIDTH, drago,
                         m_modelPreviews->record(selectedRow));

        tmoDrago->operator_options.dragooptions.bias = bias;

        addPreview(new PreviewLabel(0, tmoDrago, index++),
                   m_modelPreviews->record(selectedRow));
    }

    sqlQuery = QStringLiteral("SELECT *, 'durand' AS operator FROM durand");
    m_modelPreviews->setQuery(sqlQuery, db);

    float spatial, range, base;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount();
         selectedRow++) {
        TonemappingOptions *tmoDurand = new TonemappingOptions;
        spatial = m_modelPreviews->record(selectedRow)
                      .value(QStringLiteral("spatial"))
                      .toFloat();
        range = m_modelPreviews->record(selectedRow)
                    .value(QStringLiteral("range"))
                    .toFloat();
        base = m_modelPreviews->record(selectedRow)
                   .value(QStringLiteral("base"))
                   .toFloat();

        fillCommonValues(tmoDurand, origxsize, PREVIEW_WIDTH, durand,
                         m_modelPreviews->record(selectedRow));

        tmoDurand->operator_options.durandoptions.spatial = spatial;
        tmoDurand->operator_options.durandoptions.range = range;
        tmoDurand->operator_options.durandoptions.base = base;

        addPreview(new PreviewLabel(0, tmoDurand, index++),
                   m_modelPreviews->record(selectedRow));
    }

    sqlQuery = QStringLiteral("SELECT *, 'fattal' AS operator FROM fattal");
    m_modelPreviews->setQuery(sqlQuery, db);

    float alpha, beta, colorSat, noiseReduction;
    bool fftsolver;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount();
         selectedRow++) {
        TonemappingOptions *tmoFattal = new TonemappingOptions;
        alpha = m_modelPreviews->record(selectedRow)
                    .value(QStringLiteral("alpha"))
                    .toFloat();
        beta = m_modelPreviews->record(selectedRow)
                   .value(QStringLiteral("beta"))
                   .toFloat();
        colorSat = m_modelPreviews->record(selectedRow)
                       .value(QStringLiteral("colorSaturation"))
                       .toFloat();
        noiseReduction = m_modelPreviews->record(selectedRow)
                             .value(QStringLiteral("noiseReduction"))
                             .toFloat();
        fftsolver = !m_modelPreviews->record(selectedRow)
                         .value(QStringLiteral("oldFattal"))
                         .toBool();

        fillCommonValues(tmoFattal, origxsize, PREVIEW_WIDTH, fattal,
                         m_modelPreviews->record(selectedRow));

        tmoFattal->operator_options.fattaloptions.alpha = alpha;
        tmoFattal->operator_options.fattaloptions.beta = beta;
        tmoFattal->operator_options.fattaloptions.color = colorSat;
        tmoFattal->operator_options.fattaloptions.noiseredux = noiseReduction;
        tmoFattal->operator_options.fattaloptions.fftsolver = fftsolver;

        addPreview(new PreviewLabel(0, tmoFattal, index++),
                   m_modelPreviews->record(selectedRow));
    }

    sqlQuery =
        QStringLiteral("SELECT *, 'ferradans' AS operator FROM ferradans");
    m_modelPreviews->setQuery(sqlQuery, db);

    float rho, inv_alpha;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount();
         selectedRow++) {
        TonemappingOptions *tmoFerradans = new TonemappingOptions;
        rho = m_modelPreviews->record(selectedRow)
                  .value(QStringLiteral("rho"))
                  .toFloat();
        inv_alpha = m_modelPreviews->record(selectedRow)
                        .value(QStringLiteral("inv_alpha"))
                        .toFloat();

        fillCommonValues(tmoFerradans, origxsize, PREVIEW_WIDTH, ferradans,
                         m_modelPreviews->record(selectedRow));

        tmoFerradans->operator_options.ferradansoptions.rho = rho;
        tmoFerradans->operator_options.ferradansoptions.inv_alpha = inv_alpha;

        addPreview(new PreviewLabel(0, tmoFerradans, index++),
                   m_modelPreviews->record(selectedRow));
    }

    sqlQuery =
        QStringLiteral("SELECT *, 'mantiuk06' AS operator FROM mantiuk06");
    m_modelPreviews->setQuery(sqlQuery, db);

    bool contrastEqualization;
    float contrastFactor;
    float saturationFactor;
    float detailFactor;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount();
         selectedRow++) {
        TonemappingOptions *tmoMantiuk06 = new TonemappingOptions;
        contrastEqualization =
            m_modelPreviews->record(selectedRow)
                .value(QStringLiteral("contrastEqualization"))
                .toBool();
        contrastFactor = m_modelPreviews->record(selectedRow)
                             .value(QStringLiteral("contrastFactor"))
                             .toFloat();
        saturationFactor = m_modelPreviews->record(selectedRow)
                               .value(QStringLiteral("saturationFactor"))
                               .toFloat();
        detailFactor = m_modelPreviews->record(selectedRow)
                           .value(QStringLiteral("detailFactor"))
                           .toFloat();

        fillCommonValues(tmoMantiuk06, origxsize, PREVIEW_WIDTH, mantiuk06,
                         m_modelPreviews->record(selectedRow));

        tmoMantiuk06->operator_options.mantiuk06options.contrastfactor =
            contrastFactor;
        tmoMantiuk06->operator_options.mantiuk06options.saturationfactor =
            saturationFactor;
        tmoMantiuk06->operator_options.mantiuk06options.detailfactor =
            detailFactor;
        tmoMantiuk06->operator_options.mantiuk06options.contrastequalization =
            contrastEqualization;

        addPreview(new PreviewLabel(0, tmoMantiuk06, index++),
                   m_modelPreviews->record(selectedRow));
    }

    sqlQuery =
        QStringLiteral("SELECT *, 'mantiuk08' AS operator FROM mantiuk08");
    m_modelPreviews->setQuery(sqlQuery, db);

    float colorSaturation, contrastEnhancement, luminanceLevel;
    bool manualLuminanceLevel;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount();
         selectedRow++) {
        TonemappingOptions *tmoMantiuk08 = new TonemappingOptions;
        colorSaturation = m_modelPreviews->record(selectedRow)
                              .value(QStringLiteral("colorSaturation"))
                              .toFloat();
        contrastEnhancement = m_modelPreviews->record(selectedRow)
                                  .value(QStringLiteral("contrastEnhancement"))
                                  .toFloat();
        luminanceLevel = m_modelPreviews->record(selectedRow)
                             .value(QStringLiteral("luminanceLevel"))
                             .toFloat();
        manualLuminanceLevel =
            m_modelPreviews->record(selectedRow)
                .value(QStringLiteral("manualLuminanceLevel"))
                .toBool();

        fillCommonValues(tmoMantiuk08, origxsize, PREVIEW_WIDTH, mantiuk08,
                         m_modelPreviews->record(selectedRow));

        tmoMantiuk08->operator_options.mantiuk08options.colorsaturation =
            colorSaturation;
        tmoMantiuk08->operator_options.mantiuk08options.contrastenhancement =
            contrastEnhancement;
        tmoMantiuk08->operator_options.mantiuk08options.luminancelevel =
            luminanceLevel;
        tmoMantiuk08->operator_options.mantiuk08options.setluminance =
            manualLuminanceLevel;

        addPreview(new PreviewLabel(0, tmoMantiuk08, index++),
                   m_modelPreviews->record(selectedRow));
    }

    sqlQuery =
        QStringLiteral("SELECT *, 'pattanaik' AS operator FROM pattanaik");
    m_modelPreviews->setQuery(sqlQuery, db);

    float multiplier, rod, cone;
    bool autolum, local;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount();
         selectedRow++) {
        TonemappingOptions *tmoPattanaik = new TonemappingOptions;
        multiplier = m_modelPreviews->record(selectedRow)
                         .value(QStringLiteral("multiplier"))
                         .toFloat();
        rod = m_modelPreviews->record(selectedRow)
                  .value(QStringLiteral("rod"))
                  .toFloat();
        cone = m_modelPreviews->record(selectedRow)
                   .value(QStringLiteral("cone"))
                   .toFloat();
        autolum = m_modelPreviews->record(selectedRow)
                      .value(QStringLiteral("autolum"))
                      .toBool();
        local = m_modelPreviews->record(selectedRow)
                    .value(QStringLiteral("local"))
                    .toBool();

        fillCommonValues(tmoPattanaik, origxsize, PREVIEW_WIDTH, pattanaik,
                         m_modelPreviews->record(selectedRow));

        tmoPattanaik->operator_options.pattanaikoptions.autolum = autolum;
        tmoPattanaik->operator_options.pattanaikoptions.local = local;
        tmoPattanaik->operator_options.pattanaikoptions.cone = cone;
        tmoPattanaik->operator_options.pattanaikoptions.rod = rod;
        tmoPattanaik->operator_options.pattanaikoptions.multiplier = multiplier;

        addPreview(new PreviewLabel(0, tmoPattanaik, index++),
                   m_modelPreviews->record(selectedRow));
    }

    sqlQuery =
        QStringLiteral("SELECT *, 'reinhard02' AS operator FROM reinhard02");
    m_modelPreviews->setQuery(sqlQuery, db);

    bool scales;
    float key, phi;
    int irange, lower, upper;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount();
         selectedRow++) {
        TonemappingOptions *tmoReinhard02 = new TonemappingOptions;
        scales = m_modelPreviews->record(selectedRow)
                     .value(QStringLiteral("scales"))
                     .toBool();
        key = m_modelPreviews->record(selectedRow)
                  .value(QStringLiteral("key"))
                  .toFloat();
        phi = m_modelPreviews->record(selectedRow)
                  .value(QStringLiteral("phi"))
                  .toFloat();
        irange = m_modelPreviews->record(selectedRow)
                     .value(QStringLiteral("range"))
                     .toInt();
        lower = m_modelPreviews->record(selectedRow)
                    .value(QStringLiteral("lower"))
                    .toInt();
        upper = m_modelPreviews->record(selectedRow)
                    .value(QStringLiteral("upper"))
                    .toInt();

        fillCommonValues(tmoReinhard02, origxsize, PREVIEW_WIDTH, reinhard02,
                         m_modelPreviews->record(selectedRow));

        tmoReinhard02->operator_options.reinhard02options.scales = scales;
        tmoReinhard02->operator_options.reinhard02options.key = key;
        tmoReinhard02->operator_options.reinhard02options.phi = phi;
        tmoReinhard02->operator_options.reinhard02options.range = irange;
        tmoReinhard02->operator_options.reinhard02options.lower = lower;
        tmoReinhard02->operator_options.reinhard02options.upper = upper;

        addPreview(new PreviewLabel(0, tmoReinhard02, index++),
                   m_modelPreviews->record(selectedRow));
    }

    sqlQuery =
        QStringLiteral("SELECT *, 'reinhard05' AS operator FROM reinhard05");
    m_modelPreviews->setQuery(sqlQuery, db);

    float brightness, chromaticAdaptation, lightAdaptation;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount();
         selectedRow++) {
        TonemappingOptions *tmoReinhard05 = new TonemappingOptions;
        brightness = m_modelPreviews->record(selectedRow)
                         .value(QStringLiteral("brightness"))
                         .toFloat();
        chromaticAdaptation = m_modelPreviews->record(selectedRow)
                                  .value(QStringLiteral("chromaticAdaptation"))
                                  .toFloat();
        lightAdaptation = m_modelPreviews->record(selectedRow)
                              .value(QStringLiteral("lightAdaptation"))
                              .toFloat();

        fillCommonValues(tmoReinhard05, origxsize, PREVIEW_WIDTH, reinhard05,
                         m_modelPreviews->record(selectedRow));

        tmoReinhard05->operator_options.reinhard05options.brightness =
            brightness;
        tmoReinhard05->operator_options.reinhard05options.chromaticAdaptation =
            chromaticAdaptation;
        tmoReinhard05->operator_options.reinhard05options.lightAdaptation =
            lightAdaptation;

        addPreview(new PreviewLabel(0, tmoReinhard05, index++),
                   m_modelPreviews->record(selectedRow));
    }

    sqlQuery =
        QStringLiteral("SELECT *, 'ferwerda' AS operator FROM ferwerda");
    m_modelPreviews->setQuery(sqlQuery, db);

    float mul, adapt_lum;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount();
         selectedRow++) {
        TonemappingOptions *tmoFerwerda96 = new TonemappingOptions;
        mul = m_modelPreviews->record(selectedRow)
                         .value(QStringLiteral("maxlum"))
                         .toFloat();
        adapt_lum = m_modelPreviews->record(selectedRow)
                                  .value(QStringLiteral("adaptlum"))
                                  .toFloat();

        fillCommonValues(tmoFerwerda96, origxsize, PREVIEW_WIDTH, ferwerda,
                         m_modelPreviews->record(selectedRow));

        tmoFerwerda96->operator_options.ferwerdaoptions.multiplier =
            mul;
        tmoFerwerda96->operator_options.ferwerdaoptions.adaptationluminance =
            adapt_lum;

        addPreview(new PreviewLabel(0, tmoFerwerda96, index++),
                   m_modelPreviews->record(selectedRow));
    }

    sqlQuery =
        QStringLiteral("SELECT *, 'kimkautz' AS operator FROM kimkautz");
    m_modelPreviews->setQuery(sqlQuery, db);

    float kk_c1, kk_c2;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount();
         selectedRow++) {
        TonemappingOptions *tmoKimKautz08 = new TonemappingOptions;
        kk_c1 = m_modelPreviews->record(selectedRow)
                         .value(QStringLiteral("kk_c1"))
                         .toFloat();
        kk_c2 = m_modelPreviews->record(selectedRow)
                                  .value(QStringLiteral("kk_c2"))
                                  .toFloat();

        fillCommonValues(tmoKimKautz08, origxsize, PREVIEW_WIDTH, kimkautz,
                         m_modelPreviews->record(selectedRow));

        tmoKimKautz08->operator_options.kimkautzoptions.c1 =
            kk_c1;
        tmoKimKautz08->operator_options.kimkautzoptions.c2 =
            kk_c2;

        addPreview(new PreviewLabel(0, tmoKimKautz08, index++),
                   m_modelPreviews->record(selectedRow));
    }

    sqlQuery =
        QStringLiteral("SELECT *, 'vanhateren' AS operator FROM vanhateren");
    m_modelPreviews->setQuery(sqlQuery, db);

    float pupil_area;

    for (int selectedRow = 0; selectedRow < m_modelPreviews->rowCount();
         selectedRow++) {
        TonemappingOptions *tmoVanHateren06 = new TonemappingOptions;
        pupil_area = m_modelPreviews->record(selectedRow)
                         .value(QStringLiteral("pupil_area"))
                         .toFloat();
        fillCommonValues(tmoVanHateren06, origxsize, PREVIEW_WIDTH, vanhateren,
                         m_modelPreviews->record(selectedRow));

        tmoVanHateren06->operator_options.vanhaterenoptions.pupil_area =
            pupil_area;

        addPreview(new PreviewLabel(0, tmoVanHateren06, index++),
                   m_modelPreviews->record(selectedRow));
    }
    m_previewSettings->updatePreviews(m_frame);
}

void TonemappingSettings::addPreview(PreviewLabel *previewLabel,
                                     const QSqlRecord &record) {
    QString comment = record.value(QStringLiteral("comment")).toString();

    m_previewLabelList.append(previewLabel);

    previewLabel->setComment(comment);
    connect(previewLabel, SIGNAL(clicked(int)), m_previewSettings,
            SLOT(selectLabel(int)));
    connect(previewLabel, SIGNAL(clicked(int)), this,
            SLOT(updateListView(int)));
    connect(previewLabel, SIGNAL(clicked(TonemappingOptions *)), this,
            SLOT(tonemapPreview(TonemappingOptions *)));
    m_previewSettings->addPreviewLabel(previewLabel);
    m_Ui->listWidget->addItem(comment);
}

void TonemappingSettings::fillCommonValues(TonemappingOptions *tmOptions,
                                           int origxsize, int previewWidth,
                                           TMOperator tOperator,
                                           const QSqlRecord &record) {
    float pregamma = record.value(QStringLiteral("pregamma")).toFloat();
    float postsaturation = record.value(QStringLiteral("postsaturation")).toFloat();
    float postgamma = record.value(QStringLiteral("postgamma")).toFloat();

    tmOptions->origxsize = origxsize;
    tmOptions->xsize = previewWidth;
    tmOptions->pregamma = pregamma;
    tmOptions->postsaturation = postsaturation;
    tmOptions->postgamma = postgamma;
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
    return m_previewSettings->getPreviewLabel(m_currentIndex)
        ->getTonemappingOptions();
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

void TonemappingSettings::tonemapPreview(TonemappingOptions *opt) {
    m_wantsTonemap = true;
    accept();
}

void TonemappingSettings::on_btnTonemap_clicked() {
    m_wantsTonemap = true;
    accept();
}
