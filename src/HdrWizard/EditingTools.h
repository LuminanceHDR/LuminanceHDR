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
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef EDITINGTOOLS_H
#define EDITINGTOOLS_H

#include "ui_EditingTools.h"
#include "Common/global.h"
#include "Viewers/SelectionTool.h"
#include "HdrCreation/HdrCreationManager.h"
#include "PreviewWidget.h"
#include "Common/LuminanceOptions.h"

class HistogramLDR;
class PanIconWidget;

//defined in mtb_alignment.cpp
QImage* shiftQImage(const QImage *in, int dx, int dy);

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
	QList<QImage*> original_ldrlist;
	QStringList filelist;
	HdrCreationManager *hcm;

	QScrollArea *scrollArea;
	PreviewWidget *previewWidget;
	int additional_shift_value;
	QList< QPair<int,int> > HV_offsets;
	HistogramLDR *histogram;
	QSize previousPreviewWidgetSize;
	PanIconWidget *panIconWidget;
	QToolButton *cornerButton;
	SelectionTool *selectionTool;
	bool m_MdrSaved;
	LuminanceOptions luminanceOptions;
	QVector<float> expotimes;
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
	void crop_stack();
	void nextClicked();
	void maskColorButtonClicked();
	void antighostToolButtonToggled(bool);
	void blendModeCBIndexChanged(int);
	void setupConnections();
	void updateScrollBars(QPoint diff);
	void restoreSaveImagesButtonState();
};



#endif
