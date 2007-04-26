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

#ifndef IMAGELDRVIEWER_H
#define IMAGELDRVIEWER_H

#include "smart_scroll_area.h"
#include "tonemapping_widget.h"

class LdrViewer : public QWidget {
	Q_OBJECT
public:
	LdrViewer(QWidget *parent, QImage*, tonemapping_options*);
	~LdrViewer();
	bool getFittingWin();
	void LevelsRequested(bool);
	QString getFilenamePostFix();
	QString getExifComment();
	QImage* getQImage();
signals:
	void levels_closed();
public slots:
	void fitToWindow(bool checked);
private slots:
	void updatePreview(unsigned char *);
	void restoreoriginal();
private:
	void parseOptions(tonemapping_options *opts);
	QString caption,postfix,exif_comment;
protected:
	QLabel *imageLabel;
	SmartScrollArea *scrollArea;
	QImage *origimage,*currentimage;
	QImage previewimage;
};

#endif
