/**
 * This file is a part of Qtpfsgui package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2007 Giuseppe Rota
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
 */

#ifndef SMARTSA_H
#define SMARTSA_H

#include <QScrollArea>
#include <QScrollBar>
#include <QMouseEvent>

#include "selectableLabel.h"

class SmartScrollArea : public QScrollArea {
	Q_OBJECT
public:
	SmartScrollArea( QWidget *parent, SelectableLabel *imagelabel );
	void zoomIn();
	void zoomOut();
	void fitToWindow(bool checked);
	void normalSize();
	void scaleLabelToFit();
	double getScaleFactor() {
		return scaleFactor;
	}
	bool isFitting() {
		return fittingwin;
	}
	QRect getSelectionRect();
	void hideRubberBand();
protected:
	void resizeEvent ( QResizeEvent * );
protected slots:
	void update(QPoint diff);
private:
	SelectableLabel *imageLabel;
	double scaleFactor;
	bool fittingwin;
	void scaleImage(double);
	void adjustScrollBar(QScrollBar *scrollBar, double factor);
signals:
	void selectionReady();
};
#endif




