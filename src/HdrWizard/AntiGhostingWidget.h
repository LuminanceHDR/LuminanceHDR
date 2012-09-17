/**
 * This file is a part of Luminance HDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2012 Franco Comida
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

#ifndef ANTIGHOSTINGWIDGET_H
#define ANTIGHOSTINGWIDGET_H

#include <QWidget>
#include <QImage>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QEvent>

class AntiGhostingWidget : public QWidget
{
Q_OBJECT
public:
    AntiGhostingWidget(QWidget *parent, QImage *mask);
    ~AntiGhostingWidget();
    
    QSize sizeHint () const {
        return m_agMask->size();
    }
    
    void setMask(QImage *mask) {
        m_agMask = mask;
        m_mx = m_my = 0;
    }

    void setHV_offset(QPair<int, int> HV_offset) {
        m_mx = HV_offset.first;
        m_my = HV_offset.second;
    }

    void updateVertShift(int);
    void updateHorizShift(int);

    void setDrawWithBrush();
    void setDrawPath();
public slots:
    void switchAntighostingMode(bool);
    void setBrushSize(const int);
    void setBrushStrength(const int);
    void setBrushColor(const QColor);
    void setBrushMode(bool);

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);
    void enterEvent(QEvent *event);
    void timerEvent(QTimerEvent *event);

private:
    QImage *m_agMask;
    QPoint m_mousePos;
    int m_timerid;
    QPixmap *m_agcursorPixmap;
    int m_requestedPixmapSize, m_previousPixmapSize;
    int m_requestedPixmapStrength, m_previousPixmapStrength;
    QColor m_requestedPixmapColor, m_previousPixmapColor;
    bool m_brushAddMode;//false means brush is in remove mode.
    void fillAntiGhostingCursorPixmap();
    void drawWithBrush();
    void drawPath();
 
    float m_scaleFactor;
    int m_mx, m_my;

    QPoint m_firstPoint;
    QPoint m_lastPoint;
    QPoint m_currentPoint;
    QPainterPath m_path;
    bool m_drawingPathEnded;

    enum {BRUSH, PATH} m_drawingMode;
signals:
    void moved(QPoint diff);
};

#endif
