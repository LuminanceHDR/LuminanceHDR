/**
 * This file is a part of Qtpfsgui package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2009 Franco Comida
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
 * @author Franco Comida <fcomida@users.sourceforge.net>
 */

#ifndef GENERICVIEWER_H
#define GENERICVIEWER_H

#include <QWidget>
#include <QPixmap>
#include <QVBoxLayout>
#include <QToolBar>
#include <QToolButton>

#include "smart_scroll_area.h"
#include "panIconWidget.h"
#include "selectionTool.h"

class GenericViewer : public QWidget 
{
Q_OBJECT
public:
	GenericViewer(QWidget *parent = 0, bool ns = false, bool ncf = false);
public slots:
	virtual void setLabelPixmap(const QPixmap pix);
	virtual void fitToWindow(bool checked);
        virtual void zoomIn();
        virtual void zoomOut();
        virtual	void normalSize();
	virtual bool getFittingWin();
        virtual bool hasSelection();
	virtual void setSelectionTool(bool);
	virtual const float getScaleFactor();
	virtual const QRect getSelectionRect(void);
	virtual void removeSelection();
        virtual bool needsSaving();
        virtual void setNeedsSaving(bool);
	virtual const QString getFileName();
	virtual void setFileName(const QString);
	virtual int getHorizScrollBarValue();
	virtual int getVertScrollBarValue();
	virtual float getImageScaleFactor();
	virtual void setHorizScrollBarValue(int value);
	virtual void setVertScrollBarValue(int value);
	virtual void zoomToFactor(float factor);
	virtual void levelsRequested(bool) = 0; // only used by LdrViewer
        virtual QString getFilenamePostFix() = 0; // only used by LdrViewer 
        virtual QString getExifComment() = 0; // only used by LdrViewer
	virtual const QImage getQImage() = 0; // only used by LdrViewer
protected slots:
	virtual void slotPanIconSelectionMoved(QRect, bool);
	virtual void slotPanIconHidden();
	virtual void slotCornerButtonPressed();
protected:
	QImage image;
        bool NeedsSaving;
        bool noCloseFlag;
	QLabel imageLabel;
	QVBoxLayout *VBL_L;
	QToolBar *toolBar;
	QToolButton *cornerButton;
	SmartScrollArea *scrollArea;
	PanIconWidget *panIconWidget;
	QString filename;
        bool isSelectionReady;
        bool isSelectionToolVisible;
	virtual void closeEvent ( QCloseEvent * event );
signals:
	virtual void selectionReady(bool isReady);
	virtual void levels_closed(bool isReady); // only used by LdrViewer
};
#endif
