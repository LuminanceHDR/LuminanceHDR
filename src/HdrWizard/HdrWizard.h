/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyrighr (C) 2010,2011,2012 Franco Comida
 * Copyright (C) 2013 Davide Anastasia
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
 */

#ifndef HDRWIZARD_IMPL_H
#define HDRWIZARD_IMPL_H

#include <QDebug>
#include <QDialog>
#include <QFuture>
#include <QFutureWatcher>
#include <QString>
#include <QVector>

#include "Common/LuminanceOptions.h"
#include "HdrWizard/HdrCreationManager.h"

namespace Ui {
class HdrWizard;
}

class HdrWizard : public QDialog {
    Q_OBJECT
   private:
    // members ... private functions are below
    QScopedPointer<Ui::HdrWizard> m_Ui;
    QScopedPointer<HdrCreationManager> m_hdrCreationManager;

    QFutureWatcher<void> m_futureWatcher;
    QFuture<pfs::Frame *> m_future;

    LuminanceOptions luminance_options;

    QStringList m_inputFilesName;

    // the new hdr, returned by the HdrCreationManager class
    pfs::Frame *m_pfsFrameHDR;

    // hdr creation parameters
    QVector<FusionOperatorConfig> m_customConfig;
    bool m_patches[agGridSize][agGridSize];
    bool m_doAutoAntighosting;
    bool m_doManualAntighosting;
    int m_agGoodImageIndex;
    bool m_processing;
    ProgressHelper m_ph;

   public:
    HdrWizard(QWidget *parent, const QStringList &files,
              const QStringList &inputFilesName,
              const QVector<float> &inputExpoTimes);
    ~HdrWizard();

    //! \brief get the current PFS Frame
    pfs::Frame *getPfsFrameHDR() { return m_pfsFrameHDR; }

    //! \brief return the caption text
    QString getCaptionTEXT();
    QStringList getInputFilesNames();

   protected:
    void resizeEvent(QResizeEvent *);
    void keyPressEvent(QKeyEvent *);
    void dragEnterEvent(QDragEnterEvent *);
    void dropEvent(QDropEvent *);
    void setEVsValues();

   private:
    void updateTableGrid();
    void enableNextOrWarning(const QStringList &filesWithoutExif);
    void updateLabelMaybeNext(size_t numFilesWithoutExif);

   signals:
    void setValue(int value);
    void setRange(int min, int max);

   private slots:
    void loadInputFiles(const QStringList &files);
    void loadInputFilesDone();

    void loadImagesButtonClicked();
    void removeImageButtonClicked();
    void clearListButtonClicked();

    void inputHdrFileSelected(int currentRow);

    void updateEVSlider(int newValue);
    void updateEVSpinBox(double newValue);

   private slots:
    // void fileLoaded(int index, const QString& fname, float expotime);
    // void finishedLoadingInputFiles(const QStringList& NoExifFiles);
    void errorWhileLoading(const QString &errormessage);

    // void updateGraphicalEVvalue(float expotime, int index_in_table);

    void finishedAligning(int);
    void alignSelectionClicked();

    // HDR Creation Model Functions.....
    void predefConfigsComboBoxActivated(int);
    void customConfigCheckBoxToggled(bool);

    void weightingFunctionComboBoxActivated(int);
    void responseCurveComboBoxActivated(int);
    void modelComboBoxActivated(int);

    bool loadRespCurve();
    void saveRespCurveFileButtonClicked();
    // ...end!

    void NextFinishButtonClicked();
    void currentPageChangedInto(int);
    void editingEVfinished();
    void reject();
    void ais_failed(QProcess::ProcessError);
    void writeAisData(QByteArray data);
    void setupConnections();
    void on_pushButtonSaveSettings_clicked();
    void updateProgressBar(int);
    void updateThresholdSlider(int);
    void updateThresholdSpinBox(double);

    void createHdr();
    void createHdrFinished();
    void autoAntighostingFinished();
    void updateHideLogButtonText(bool);
};

#endif
