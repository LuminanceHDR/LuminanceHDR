/**
 * This file is a part of Luminance HDR package.
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

#ifndef PREVIEWWIDGET_H
#define PREVIEWWIDGET_H

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QColor>
#include <QResizeEvent>
#include <QScrollBar>
#include <QScrollArea>
#include <QImage>

class PreviewWidget :public QObject, public QGraphicsItem
{
Q_OBJECT

public:
    PreviewWidget(QImage *m, const QImage *p);
    ~PreviewWidget();
    QSize sizeHint () const {
        return m_previewImage->size();
    }
    QImage * getPreviewImage() {
        renderPreviewImage(blendmode);
        return m_previewImage;
    }
    void setPivot(QImage *p, int p_px, int p_py);
    void setPivot(QImage *p);
    void setMovable(QImage *m, int p_mx, int p_my);
    void setMovable(QImage *m);
    void updateVertShiftMovable(int v);
    void updateHorizShiftMovable(int h);
    void updateHorizShiftPivot(int h);
    void updateVertShiftPivot(int v);
    int getWidth() { return m_previewImage->width(); }
    int getHeight() { return m_previewImage->height(); }
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                QWidget *widget);
public Q_SLOTS:
    void requestedBlendMode(int);
protected:
    void resizeEvent(QResizeEvent *event);

private:
    //5 blending modes
    inline QRgb computeOnlyMovable(const QRgb *Mrgba, const QRgb */*Prgba*/) const {
        return *Mrgba;
    }
    inline QRgb computeOnlyPivot(const QRgb */*Mrgba*/, const QRgb *Prgba) const {
        return *Prgba;
    }
    inline QRgb computeAddRgba(const QRgb *Mrgba, const QRgb *Prgba) const {
        int ro,go,bo;
        int Mred   = qRed(*Mrgba);
        int Mgreen = qGreen(*Mrgba);
        int Mblue  = qBlue(*Mrgba);
        int Malpha = qAlpha(*Mrgba);
        int Pred   = qRed(*Prgba);
        int Pgreen = qGreen(*Prgba);
        int Pblue  = qBlue(*Prgba);
        int Palpha = qAlpha(*Prgba);
        //blend samples using alphas as weights
        ro = ( Pred*Palpha + Mred*Malpha )/510;
        go = ( Pgreen*Palpha + Mgreen*Malpha )/510;
        bo = ( Pblue*Palpha + Mblue*Malpha )/510;
        //the output image still has alpha=255 (opaque)
        return qRgba(ro,go,bo,255);
    }
    inline QRgb computeDiffRgba(const QRgb *Mrgba, const QRgb *Prgba) const {
        int ro,go,bo;
        int Mred        = qRed(*Mrgba);
        int Mgreen      = qGreen(*Mrgba);
        int Mblue       = qBlue(*Mrgba);
        int Malpha      = qAlpha(*Mrgba);
        int Pred        = qRed(*Prgba);
        int Pgreen      = qGreen(*Prgba);
        int Pblue       = qBlue(*Prgba);
        int Palpha      = qAlpha(*Prgba);
        //blend samples using alphas as weights
        ro = qAbs( Pred*Palpha - Mred*Malpha )/255;
        go = qAbs( Pgreen*Palpha - Mgreen*Malpha )/255;
        bo = qAbs( Pblue*Palpha - Mblue*Malpha )/255;
        //the output image still has alpha=255 (opaque)
        return qRgba(ro,go,bo,255);
    }

    QRgb(PreviewWidget::*blendmode)(const QRgb*,const QRgb*)const;
    void renderPreviewImage(QRgb(PreviewWidget::*f)(const QRgb*,const QRgb*)const,const QRect a = QRect());

    // the out and 2 in images
    QImage *m_previewImage;
    QImage *m_movableImage;
    const QImage *m_pivotImage;

    QRegion m_prevComputed;

    //movable and pivot's x,y shifts
    int m_mx, m_my, m_px, m_py;

    //for panning with mid-button
    QPoint m_mousePos;

    enum {LB_nomode,LB_antighostingmode} m_leftButtonMode;
};

#endif
