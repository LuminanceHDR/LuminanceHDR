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

#ifndef  IMAGEHDRVIEWER_H
#define IMAGEHDRVIEWER_H
#include <QMainWindow>
#include <QImage>
#include <QComboBox>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QScrollArea>
#include <QLabel>
#include <QToolBar>
#include <QResizeEvent>
#include "luminancerange_widget.h"
#include "libpfs/array2d.h"
#include "libpfs/pfs.h"

enum LumMappingMethod {
  MAP_LINEAR,
  MAP_GAMMA1_4,
  MAP_GAMMA1_8,
  MAP_GAMMA2_2,
  MAP_GAMMA2_6,
  MAP_LOGARITHMIC
};


class ImageMDIwindow : public QMainWindow {
	Q_OBJECT
public:
	ImageMDIwindow ( QWidget *parent, unsigned int negcol, unsigned int naninfcol );
	~ImageMDIwindow();
	LuminanceRangeWidget *lumRange;
	QToolBar *toolBar;
	double getScaleFactor();
	bool getFittingWin();
	void updateHDR(pfs::Frame*);
	pfs::Frame* getHDRPfsFrame();
	void update_colors( unsigned int negcol, unsigned int naninfcol );
public slots:
	void updateRangeWindow();
	void setLumMappingMethod( int method );
	void zoomIn();
	void zoomOut();
	void fitToWindow(bool checked);
	void normalSize();
protected:
	void resizeEvent ( QResizeEvent * event );
	QLabel *imageLabel;
	QScrollArea *scrollArea;
	QImage *image;
	void setRangeWindow( float min, float max );
	const pfs::Array2D *getPrimaryChannel();
	void updateImage();
private:
	pfs::Frame* hdrpfsframe;
	void scaleImage(double);
	void scaleImageToFit();
	void adjustScrollBar(QScrollBar *scrollBar, double factor);
	void mapFrameToImage();
	QComboBox *mappingMethodCB;
	pfs::Array2D *workArea[3];
	pfs::Frame *pfsFrame;
	LumMappingMethod mappingMethod;
	float minValue;
	float maxValue;
	double scaleFactor;
	bool fittingwin;
	unsigned int naninfcol,negcol;
};
#endif
