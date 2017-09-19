/**
 * This file is a part of Luminance HDR package.
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
 * Improvements, bugfixing, anti-ghosting
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef EDITINGTOOLS_H
#define EDITINGTOOLS_H

#include <QDialog>
#include <QMap>

#include "Common/LuminanceOptions.h"
#include "Common/global.h"
#include "HdrWizard/HdrCreationManager.h"
#include "PreviewWidget.h"

class HistogramLDR;
class PanIconWidget;

namespace Ui {
class EditingToolsDialog;
}

class EditingTools : public QDialog {
    Q_OBJECT
   public:
    EditingTools(HdrCreationManager *, bool autoAg, QWidget *parent = 0);
    ~EditingTools();
    bool isAutoAntighostingEnabled() { return m_doAutoAntighosting == true; }
    bool isManualAntighostingEnabled() {
        return m_doManualAntighosting == true;
    }
    int getAgGoodImageIndex() { return m_agGoodImageIndex; }

   protected:
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);

   private:
    QScopedPointer<Ui::EditingToolsDialog> m_Ui;
    QList<QImage *> m_originalImagesList;
    QList<QImage *> m_antiGhostingMasksList;
    QImage *m_antiGhostingMask;
    int m_currentAgMaskIndex;
    QStringList m_fileList;
    HdrCreationManager *m_hcm;
    QMap<QString, int> m_filesMap;
    QScrollArea *m_scrollArea;
    PreviewWidget *m_previewWidget;
    int m_additionalShiftValue;
    QList<QPair<int, int>> m_HV_offsets;
    HistogramLDR *m_histogram;
    QSize m_previousPreviewWidgetSize;
    bool m_imagesSaved;
    int m_agGoodImageIndex;
    bool m_antiGhosting;
    LuminanceOptions m_luminanceOptions;
    QVector<float> m_expotimes;
    bool m_patches[agGridSize][agGridSize];
    int m_gridX;
    int m_gridY;
    bool m_doAutoAntighosting;
    bool m_doManualAntighosting;
    QImage *m_patchesMask;
    bool m_patchesEdited;

    void setAntiGhostingWidget(QImage *, QPair<int, int>);
    void cropAgMasks(const QRect &ca);
    void computeAgMask();

   private slots:
    void saveImagesButtonClicked();
    void updatePivot(int);
    void updateMovable(int);
    void upClicked();
    void rightClicked();
    void downClicked();
    void leftClicked();
    void horizShiftChanged(int);
    void vertShiftChanged(int);
    void resetCurrent();
    void resetAll();
    void prevLeft();
    void nextLeft();
    void prevBoth();
    void nextBoth();
    void prevRight();
    void nextRight();
    void enterWhatsThis();
    void zoomIn();
    void zoomOut();
    void fitPreview();
    void fillPreview();
    void origSize();
    void cropStack();
    void nextClicked();
    void maskColorButtonClicked();
    void lassoColorButtonClicked();
    void antighostToolButtonToggled(bool);
    void blendModeCBIndexChanged(int);
    void setupConnections();
    void restoreSaveImagesButtonState();
    void addGoodImage();
    void removeGoodImage();
    void updateAgMask(int);
    void antighostToolButtonPaintToggled(bool);
    void saveAgMask();
    void applySavedAgMask();
    void on_recomputePatches_pushButton_clicked();
    void on_autoAG_checkBox_toggled(bool toggled);
    void updateThresholdSlider(int newValue);
    void updateThresholdSpinBox(double newThreshold);
    void setPatchesEdited();
};

#endif
