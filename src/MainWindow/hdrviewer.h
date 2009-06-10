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
#include <QResizeEvent>
#include <QProgressDialog>
#include "../Common/genericViewer.h"
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
	HdrViewer (QWidget *parent, bool ns = false, bool ncf = false,  unsigned int negcol = 0, unsigned int naninfcol = 0);
	~HdrViewer();
	void updateHDR(pfs::Frame*);
	pfs::Frame* getHDRPfsFrame();
	void saveHdrPreview();
	void showLoadDialog();
	void hideLoadDialog(void);
	void setFlagUpdateImage(bool updateImage);
	LuminanceRangeWidget* lumRange();
	void update_colors(unsigned int negcol, unsigned int naninfcol);
	void mapFrameToImage();
 	void levelsRequested(bool); // do nothing but used by LdrViewer (its own implementation)
        QString getFilenamePostFix(); // do nothing but used by LdrViewer (its own implementation)
        QString getExifComment(); // do nothing but used by LdrViewer (its own implementation)
	const QImage* getQImage(); // do nothing but used by LdrViewer (its own implementation)
public slots:
	void updateRangeWindow();
	int getLumMappingMethod();
	void setLumMappingMethod( int method );
	void setMaximum(int max);
	void setValue(int value);
protected:
	QImage *image;
	LuminanceRangeWidget *m_lumRange;
	QComboBox *mappingMethodCB;
	pfs::Array2D *workArea[3];
	pfs::Frame *pfsFrame;
	LumMappingMethod mappingMethod;
	float minValue;
	float maxValue;
	unsigned int naninfcol,negcol;
	QProgressDialog *progress;
	bool flagUpdateImage;

	void setRangeWindow( float min, float max );
	const pfs::Array2D *getPrimaryChannel();
	void updateImage();
};
#endif
