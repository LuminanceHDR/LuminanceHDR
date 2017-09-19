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

#include <QColor>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QToolButton>
#include <QVBoxLayout>

#include "AutoAntighosting.h"  // Just for agGridSize !!!

class IGraphicsView;
class IGraphicsPixmapItem;
class PanIconWidget;

class PreviewWidget : public QWidget {
    Q_OBJECT
   public:
    //! \brief Enum containing the list of possible view mode
    enum ViewerMode { FIT_WINDOW = 0, FILL_WINDOW = 1, NORMAL_SIZE = 2 };

    PreviewWidget(QWidget *parent, QImage *m, const QImage *p);
    ~PreviewWidget();
    QSize sizeHint() const { return m_previewImage->size(); }
    float getScaleFactor();
    QImage *getPreviewImage() {
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
    int getWidth();
    int getHeight();
    bool isFittedToWindow();
    bool isFilledToWindow();
    bool isNormalSize();

    void setMask(QImage *mask);
    QImage *getMask();  // Conversion to QImage to QPixmap is made for speed
                        // optimization
                        // we need to return a QImage from the modified QPixmap

    void setPatchesMask(QImage *mask);
    void setHV_offset(QPair<int, int> HV_offset) {
        m_mx = HV_offset.first;
        m_my = HV_offset.second;
    }

    void setDrawWithBrush();
    void setDrawPath();
    // void renderPatchesMask(bool patches[][agGridSize], const int gridX, const
    // int gridY);
    void renderPatchesMask();

   public slots:
    void requestedBlendMode(int);
    void updateView();  // tells the Viewer to update the View area
    void updatePreviewImage();

    void zoomIn();
    void zoomOut();

    void fitToWindow();
    void fillToWindow();
    void normalSize();

    //! \brief get viewer mode (Fit, Fill or Normal Size)
    ViewerMode getViewerMode();

    //! \brief set viewer mode (Fit, Fill or Normal Size)
    void setViewerMode(ViewerMode viewer_mode);

    // selection properties!
    bool hasSelection();
    void setSelectionTool(bool);
    QRect getSelectionRect();
    void removeSelection();

    void switchAntighostingMode(bool);
    void switchViewPatchesMode(bool, bool[][agGridSize], const int, const int);
    void getPatches(bool[][agGridSize]);
    void setBrushSize(const int);
    void setBrushStrength(const int);
    void setBrushColor(const QColor);
    void setLassoColor(const QColor);
    void setBrushMode(bool);
    void saveAgMask();
    QImage *getSavedAgMask();

   protected slots:
    void slotPanIconSelectionMoved(QRect);
    void slotPanIconHidden();
    void slotCornerButtonPressed();
    void scrollBarChanged(int /*value*/);

   signals:
    void moved(QPoint diff);
    void selectionReady(bool isReady);
    void changed(
        PreviewWidget *v);  // emitted when zoomed in/out, scrolled ....
    void patchesEdited();

   protected:
    bool eventFilter(QObject *object, QEvent *event);
    virtual void timerEvent(QTimerEvent *event);

   private:
    // 5 blending modes
    inline QRgb computeOnlyMovable(const QRgb *Mrgba,
                                   const QRgb * /*Prgba*/) const {
        return *Mrgba;
    }
    inline QRgb computeOnlyPivot(const QRgb * /*Mrgba*/,
                                 const QRgb *Prgba) const {
        return *Prgba;
    }
    inline QRgb computeAddRgba(const QRgb *Mrgba, const QRgb *Prgba) const {
        int ro, go, bo;
        int Mred = qRed(*Mrgba);
        int Mgreen = qGreen(*Mrgba);
        int Mblue = qBlue(*Mrgba);
        int Malpha = qAlpha(*Mrgba);
        int Pred = qRed(*Prgba);
        int Pgreen = qGreen(*Prgba);
        int Pblue = qBlue(*Prgba);
        int Palpha = qAlpha(*Prgba);
        // blend samples using alphas as weights
        ro = (Pred * Palpha + Mred * Malpha) / 510;
        go = (Pgreen * Palpha + Mgreen * Malpha) / 510;
        bo = (Pblue * Palpha + Mblue * Malpha) / 510;
        // the output image still has alpha=255 (opaque)
        return qRgba(ro, go, bo, 255);
    }
    inline QRgb computeDiffRgba(const QRgb *Mrgba, const QRgb *Prgba) const {
        int ro, go, bo;
        int Mred = qRed(*Mrgba);
        int Mgreen = qGreen(*Mrgba);
        int Mblue = qBlue(*Mrgba);
        int Malpha = qAlpha(*Mrgba);
        int Pred = qRed(*Prgba);
        int Pgreen = qGreen(*Prgba);
        int Pblue = qBlue(*Prgba);
        int Palpha = qAlpha(*Prgba);
        // blend samples using alphas as weights
        ro = qAbs(Pred * Palpha - Mred * Malpha) / 255;
        go = qAbs(Pgreen * Palpha - Mgreen * Malpha) / 255;
        bo = qAbs(Pblue * Palpha - Mblue * Malpha) / 255;
        // the output image still has alpha=255 (opaque)
        return qRgba(ro, go, bo, 255);
    }

    QRgb (PreviewWidget::*blendmode)(const QRgb *, const QRgb *) const;
    void renderPreviewImage(QRgb (PreviewWidget::*f)(const QRgb *, const QRgb *)
                                const,
                            const QRect a = QRect());
    void renderAgMask();
    void scrollAgMask(int, int);

    // the out and 2 in images
    QImage *m_previewImage;
    QImage *m_movableImage;
    const QImage *m_pivotImage;
    QImage *m_agMask;
    QImage *m_originalAgMask;
    QImage *m_patchesMask;
    QPixmap *m_agMaskPixmap;
    QImage *m_savedMask;

    QToolButton *mCornerButton;
    PanIconWidget *mPanIconWidget;

    QVBoxLayout *mVBL;

    QGraphicsScene *mScene;
    IGraphicsView *mView;
    ViewerMode mViewerMode;
    IGraphicsPixmapItem *mPixmap, *mAgPixmap;

    QRegion m_prevComputed;
    QRect m_rect;
    // movable and pivot's x,y shifts
    int m_mx, m_my, m_px, m_py;
    int m_old_mx, m_old_my;
    // zoom factor
    // float m_scaleFactor;

    int m_timerid;
    QPixmap *m_agcursorPixmap;
    int m_requestedPixmapSize, m_previousPixmapSize;
    int m_requestedPixmapStrength, m_previousPixmapStrength;
    QColor m_requestedPixmapColor, m_previousPixmapColor, m_requestedLassoColor;
    bool m_brushAddMode;  // false means brush is in remove mode.
    void fillAntiGhostingCursorPixmap();
    void drawWithBrush();
    void drawPath();

    QPointF m_mousePos;
    QPointF m_firstPoint;
    QPointF m_lastPoint;
    QPointF m_currentPoint;
    QPainterPath m_path;
    bool m_drawingPathEnded;

    bool m_patches[agGridSize][agGridSize];
    int m_gridX;
    int m_gridY;

    enum { BRUSH, PATH } m_drawingMode;
    enum { EditingMode, AntighostingMode, ViewPatches } m_mode;
};

#endif
