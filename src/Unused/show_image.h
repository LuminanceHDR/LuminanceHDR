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
 */


#ifndef SHOW_IM_H
#define SHOW_IM_H
#include <QPainter>
#include <QScrollArea>
#include <QMouseEvent>

struct CP;
struct Image {
	bool master; //if master, the image will not be moved
	QString filename; //for displaying purposes
	QImage *payload; //the data is required in the CP tab
	QList<CP*> listcps; // the list of control points (CP) belonging to this image
};

struct CP {
	double x,y; //coordinates of point
	Image *whobelongsto;
};

class ShowImage;
class ScrollArea : public QScrollArea
{
Q_OBJECT

public:
	ScrollArea(ShowImage *);
	~ScrollArea();

protected:
	void mousePressEvent(QMouseEvent *e) {
		mousePos = e->pos();
	}
	void mouseMoveEvent(QMouseEvent *e);
// 	void mouseReleaseEvent(QMouseEvent *e);
private:
	QPoint mousePos;
};

class ShowImage : public QWidget
{
Q_OBJECT
public:
	ShowImage(QList<Image *> *p, int listimgs_own_idx, QList<QPair<CP*,CP*> > *pairs/*, bool left*/);
	~ShowImage();
	void update_firstPoint(CP*);

public slots:
	void clear_temp_CP();
	void update_other_idx(int);

signals:
	void finished_point_in_image(int,int);
	void point_finished_moving(CP*);

protected:
	void paintEvent(QPaintEvent *e);

	void mousePressEvent(QMouseEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);

	QSize sizeHint () const { //adjustSize() uses this.
		return QSize(data->payload->width(),data->payload->height());
	}

private:
	int ownindex; //index (in listimages) of our image
	QList<Image *> *ptrlistimages; //we need a pointer to the list of images to find out which of our points we have to draw
	QList<QPair<CP*,CP*> > *pairs; //we need a pointer to the list of pairs  to find out which of our points we have to draw
	int otherindex; //index (in listimages) of the currently selected image in other side (left or right)
	Image *data;
	CP *temp; //this is the pointer to the CP that exists only on the current side (left or right)
	CP *dragged; //this is the pointer to one of our CPs, if dragged
	bool left;
};
#endif
