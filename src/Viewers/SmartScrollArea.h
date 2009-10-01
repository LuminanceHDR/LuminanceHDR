/**
 * This file is a part of LuminanceHDR package.
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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef SMARTSA_H
#define SMARTSA_H

#include <QScrollArea>
#include <QScrollBar>
#include <QLabel>
#include <QMouseEvent>

#include "Common/SelectionTool.h"

class SmartScrollArea : public QScrollArea {
	Q_OBJECT
public:
	SmartScrollArea( QWidget *parent, QLabel &imagelabel );
	void zoomIn();
	void zoomOut();
	void zoomToFactor(float factor);
	void fitToWindow(bool checked);
	void normalSize();
	void scaleLabelToFit();
	void scaleImage(double);
	void scaleImage();
	int  getHorizScrollBarValue();
	int  getVertScrollBarValue();
	float getScaleFactor();
	float getImageScaleFactor();
	void setHorizScrollBarValue(int value);
	void setVertScrollBarValue(int value);
	bool isFittedToWindow();
	QRect getSelectionRect();
	void setSelectionTool(bool toggled);
	bool hasSelection();
	void removeSelection();
protected:
	void resizeEvent ( QResizeEvent * );
	void mousePressEvent(QMouseEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
protected slots:
	void updateScrollBars(QPoint diff);
	void valueChanged(int value);
	void ensureVisible(int x, int y, int w, int h);
private:
	QLabel &imageLabel;
	SelectionTool *selectionTool;
	QPoint m_mousePos;
	float scaleFactor;
	float previousScaleFactor;
	bool fittingwin;
	bool panning;
	void adjustScrollBar(QScrollBar *scrollBar, double factor);
signals:
	void changed(); 
	void selectionReady(bool);
};

#endif

