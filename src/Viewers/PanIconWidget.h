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
 *
 */

#ifndef PANICONWIDGET_H
#define PANICONWIDGET_H

#include <QFrame>

class PanIconWidget : public QFrame {
    Q_OBJECT
   public:
    PanIconWidget(QWidget *parent = 0, Qt::WindowFlags flags = Qt::Popup);
    ~PanIconWidget();
    void setImage(const QImage *fullsize_zoomed_image);
    void popup(const QPoint &pos);
    void setRegionSelection(QRect regionSelection);
    void setMouseFocus(void);
    void setCursorToLocalRegionSelectionCenter(void);
   signals:
    // void signalSelectionMoved( QRect rect, bool targetDone );
    void selectionMoved(QRect rect);
    void signalHidden(void);
    void finished();

   public slots:
   protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);

   private:
    /** Recalculate the target selection position and emit 'selectionMoved'.*/
    void regionSelectionMoved();
    // coordinates relative to this widget
    int xpos;
    int ypos;
    bool moveSelection;
    QRect regionSelection;         // Original size image selection.
    QRect m_localRegionSelection;  // Thumbnail size selection.
    int m_width;
    int m_height;
    int m_orgWidth;
    int m_orgHeight;
    QImage *m_image;
};
#endif
