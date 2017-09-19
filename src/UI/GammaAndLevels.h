/**
 * This file is a part of LuminanceHDR package.
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

#ifndef GAMMA_AND_LEVELS_H
#define GAMMA_AND_LEVELS_H

#include <QDialog>
#include <QImage>
#include <QWidget>

#include "Viewers/GenericViewer.h"

namespace Ui {
class LevelsDialog;
}

class GrayBar : public QWidget {
    Q_OBJECT
   public:
    GrayBar(QWidget *parent, bool two_handles = false);
    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    bool dont_emit;

   protected:
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

   private:
    // graphical coordinate, 0-width(), NOT 0-255
    int blackpos, gammapos, whitepos;
    float blackgrayratio;
    bool twohandles;
    enum draggingT { DRAGNONE, DRAGBLACK, DRAGGRAY, DRAGWHITE } dragging;
    draggingT findHandle(int x, int y);
   public slots:
    void resetvalues();
    void changeBlack(int);
    void changeGamma(double);
    void changeWhite(int);
   signals:
    void black_changed(int);
    void gamma_changed(double);
    void white_changed(int);
    void default_gamma_black_white();
    void default_black_white();
};

class HistogramLDR : public QWidget {
    Q_OBJECT
   public:
    HistogramLDR(QWidget *parent);
    ~HistogramLDR();
    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    void setData(const QImage *data);

    void setFrame(bool b = true);
    void setColorHistogram(bool b = true);

    bool isFrame() { return isDrawFrame; }
    bool isColorHistogram() { return isDrawColorHist; }

   protected:
    //! \brief repaints canvas
    void paintEvent(QPaintEvent *);
    //! \brief disable/enable color histogram on each double click
    void mouseDoubleClickEvent(QMouseEvent *event);

   private:
    // LDR means 256 bins
    float m_GreyHist[256];
    float m_RedHist[256];
    float m_GreenHist[256];
    float m_BlueHist[256];

    bool isDrawFrame;
    bool isDrawColorHist;
};

class GammaAndLevels : public QDialog {
    Q_OBJECT
   private:
    const QImage m_ReferenceQImage;  // can only be read

    int blackin, whitein, blackout, whiteout;
    float gamma;

    GrayBar *gb1;
    GrayBar *gb2;
    HistogramLDR *histogram;

    QScopedPointer<Ui::LevelsDialog> m_Ui;

    void refreshLUT();

   public:
    GammaAndLevels(QWidget *parent, const QImage &image);
    ~GammaAndLevels();

    QImage getReferenceQImage();

    float getBlackPointInput();
    float getBlackPointOutput();
    float getWhitePointInput();
    float getWhitePointOutput();
    float getGamma();

   signals:
    void updateQImage(QImage image);

   private slots:
    void resetValues();
    void updateBlackIn(int);
    void updateGamma(double);
    void updateWhiteIn(int);
    void updateBlackOut(int);
    void updateWhiteOut(int);
    void defaultGammaBlackWhiteIn();
    void defaultBlackWhiteOut();
};

#endif
