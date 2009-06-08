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
 * based on Rafal Mantiuk and Grzegorz Krawczyk 's pfsview code
 */

#ifndef IMAGEHDRVIEWER_H
#define IMAGEHDRVIEWER_H

#include <QImage>
#include <QComboBox>
#include <QLabel>
#include <QToolBar>
#include <QResizeEvent>
#include <QToolButton>
#include <QProgressDialog>
#include "../Common/genericViewer.h"
#include "../Common/smart_scroll_area.h"
#include "../Common/panIconWidget.h"
#include "../Common/selectionTool.h"
#include "luminancerange_widget.h"
#include "../Libpfs/array2d.h"
#include "../Libpfs/pfs.h"

enum LumMappingMethod {
  MAP_LINEAR,
  MAP_GAMMA1_4,
  MAP_GAMMA1_8,
  MAP_GAMMA2_2,
  MAP_GAMMA2_6,
  MAP_LOGARITHMIC
};


class HdrViewer : public GenericViewer {
	Q_OBJECT
public:
	HdrViewer (QWidget *parent, unsigned int negcol, unsigned int naninfcol, bool NeedsSaving, bool noCloseFlag = false );
	~HdrViewer();
	LuminanceRangeWidget *lumRange;
	bool NeedsSaving;
	QToolBar *toolBar;
	QString filename;
	double getScaleFactor();
	bool getFittingWin();
	void updateHDR(pfs::Frame*);
	pfs::Frame* &getHDRPfsFrame();
	void update_colors(unsigned int negcol, unsigned int naninfcol);
	void saveHdrPreview();
	void showLoadDialog();
	void hideLoadDialog(void);
	QRect getSelectionRect(void);
	void setFlagUpdateImage(bool updateImage);
signals:
	void selectionReady(bool isReady);
public slots:
	void zoomIn();
	void zoomOut();
	void fitToWindow(bool checked);
	void normalSize();
	void updateRangeWindow();
	int getLumMappingMethod();
	void setLumMappingMethod( int method );
	void setMaximum(int max);
	void setValue(int value);
	void removeSelection();
	bool hasSelection();
private slots:
	void slotPanIconSelectionMoved(QRect, bool);
	void slotPanIconHidden();
	void slotCornerButtonPressed();
	void setSelection(bool);
protected:
	QLabel *imageLabel;
	SmartScrollArea *scrollArea;
	QImage *image;
	void setRangeWindow( float min, float max );
	const pfs::Array2D *getPrimaryChannel();
	void updateImage();
	void closeEvent ( QCloseEvent * event );
private:
	PanIconWidget *panIconWidget;
	QToolButton *cornerButton;
	void mapFrameToImage();
	QComboBox *mappingMethodCB;
	pfs::Array2D *workArea[3];
	pfs::Frame *pfsFrame;
	LumMappingMethod mappingMethod;
	float minValue;
	float maxValue;
	unsigned int naninfcol,negcol;
	QProgressDialog *progress;
	bool selection;
	bool flagUpdateImage;
	bool noCloseFlag;
};
#endif
