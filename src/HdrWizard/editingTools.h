/**
 * This file is a part of Qtpfsgui package.
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
 *
 */

#ifndef ALIGNMENTDIALOG_IMPL_H
#define ALIGNMENTDIALOG_IMPL_H

#include <QSettings>
#include "../generated_uic/ui_aligndialog.h"
#include "previewWidget.h"
#include "../../panIconWidget.h"


class HistogramLDR;
//defined in mtb_alignment.cpp
QImage* shiftQImage(const QImage *in, int dx, int dy);

class AlignmentDialog : public QDialog, private Ui::AlignmentDialog
{
Q_OBJECT
public:
	AlignmentDialog(QWidget *parent, QList<QImage*>&ldrlist, QList<bool> &ldr_tiff_input, QStringList fileStringList, Qt::ToolButtonStyle s);
	~AlignmentDialog();
protected:
	void keyPressEvent(QKeyEvent *);
	void keyReleaseEvent(QKeyEvent *);
private:
	QScrollArea *scrollArea;
	PreviewWidget *previewWidget;
	int additional_shift_value;
	QList<QImage*> &original_ldrlist;
	QList<bool> &ldr_tiff_input;
	QList< QPair<int,int> > HV_offsets;
	HistogramLDR *histogram;
	QSize previousPreviewWidgetSize;
	PanIconWidget *panIconWidget;
	QToolButton *cornerButton;
	QString recentPathInputLDR;
	QSettings settings;
	void applyShiftsToImageStack();
private slots:
	void slotPanIconSelectionMoved(QRect, bool);
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
};



#endif
