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
 * Improvements, bugfixing, anti ghosting
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef EDITINGTOOLS_H
#define EDITINGTOOLS_H

#include <QMap>

#include "ui_EditingTools.h"
#include "PreviewWidget.h"
#include "Common/global.h"
#include "Viewers/SelectionTool.h"
#include "HdrCreation/HdrCreationManager.h"
#include "PreviewWidget.h"
#include "AntiGhostingWidget.h"
#include "Common/LuminanceOptions.h"

class HistogramLDR;
class PanIconWidget;

class EditingTools : public QDialog, private Ui::EditingToolsDialog
{
Q_OBJECT
public:
	EditingTools(HdrCreationManager *, QWidget *parent=0);
	~EditingTools();
protected:
	void keyPressEvent(QKeyEvent *);
	void keyReleaseEvent(QKeyEvent *);
private:
	QList<QImage*> m_originalImagesList;
	QList<QImage*> m_antiGhostingMasksList;
	QStringList m_fileList;
	HdrCreationManager *m_hcm;
    QMap<QString, int> m_filesMap;
	QScrollArea *m_scrollArea;
	PreviewWidget *m_previewWidget;
	AntiGhostingWidget *m_agWidget;
	int m_additionalShiftValue;
	QList< QPair<int,int> > m_HV_offsets;
	HistogramLDR *m_histogram;
	QSize m_previousPreviewWidgetSize;
	PanIconWidget *m_panIconWidget;
	QToolButton *m_cornerButton;
	SelectionTool *m_selectionTool;
	bool m_imagesSaved;
    int m_goodImageIndex;
    bool m_antiGhosting;
	LuminanceOptions m_luminanceOptions;
	QVector<float> m_expotimes;

	void setAntiGhostingWidget(QImage*, QPair<int, int>);
private slots:
	void slotPanIconSelectionMoved(QRect);
	void slotPanIconHidden();
	void slotCornerButtonPressed();
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
	void fitPreview(bool);
	void origSize();
	void cropStack();
	void nextClicked();
	void maskColorButtonClicked();
	void lassoColorButtonClicked();
	void antighostToolButtonToggled(bool);
	void blendModeCBIndexChanged(int);
	void setupConnections();
	void updateScrollBars(QPoint diff);
	void restoreSaveImagesButtonState();
    void addGoodImage();
    void removeGoodImage();
    void updateAgMask(int);
    void antighostToolButtonPaintToggled(bool);
    void saveAgMask();
    void applySavedAgMask();
};

#endif
