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

#include <boost/math/special_functions/round.hpp>

#include <QApplication>
#include <QDesktopWidget>
#include <QPaintEvent>
#include <QPainter>
#include <cmath>

#include "arch/math.h"

#include "PanIconWidget.h"

PanIconWidget::PanIconWidget(QWidget *parent, Qt::WindowFlags flags)
    : QFrame(parent, flags),
      xpos(0),
      ypos(0),
      m_width(0),
      m_height(0),
      m_orgWidth(0),
      m_orgHeight(0) {
    this->setAttribute(Qt::WA_DeleteOnClose);
    //     this->setFrameStyle(QFrame::Box|QFrame::Plain);
    //     setLineWidth(1);
    //     setMidLineWidth(2);
    //     setContentsMargins(2,2,2,2);
    m_image = NULL;
    moveSelection = false;
    setMouseTracking(true);  // necessary?
}

void PanIconWidget::setImage(const QImage *fullsize) {
    m_image = new QImage(fullsize->scaled(180, 120, Qt::KeepAspectRatio));
    m_width = m_image->width();
    m_height = m_image->height();
    m_orgWidth = fullsize->width();
    m_orgHeight = fullsize->height();
    setFixedSize(m_width + 2 * frameWidth(), m_height + 2 * frameWidth());
    //     m_rect = QRect(width()/2-m_width/2, height()/2-m_height/2, m_width,
    //     m_height);
    // KPopupFrame::setMainWidget = resize
    resize(m_width + 2 * frameWidth(), m_height + 2 * frameWidth());
    //     qDebug("wanted w=%d, h=%d",m_width+2*frameWidth(),
    //     m_height+2*frameWidth());
}

void PanIconWidget::regionSelectionMoved() {
    int x = (int)boost::math::round(
        ((float)m_localRegionSelection.x() /*- (float)m_rect.x()*/) *
        ((float)m_orgWidth / (float)m_width));

    int y = (int)boost::math::round(
        ((float)m_localRegionSelection.y() /*- (float)m_rect.y()*/) *
        ((float)m_orgHeight / (float)m_height));

    int w = (int)boost::math::round((float)m_localRegionSelection.width() *
                                    ((float)m_orgWidth / (float)m_width));

    int h = (int)boost::math::round((float)m_localRegionSelection.height() *
                                    ((float)m_orgHeight / (float)m_height));

    regionSelection.setX(x);
    regionSelection.setY(y);
    regionSelection.setWidth(w);
    regionSelection.setHeight(h);

    update();
    emit selectionMoved(regionSelection);
}

void PanIconWidget::setMouseFocus() {
    raise();
    xpos = m_localRegionSelection.center().x();
    ypos = m_localRegionSelection.center().y();
    moveSelection = true;
    //     emit signalSelectionTakeFocus(); //start moving? hook?
}

void PanIconWidget::setCursorToLocalRegionSelectionCenter(void) {
    QCursor::setPos(mapToGlobal(m_localRegionSelection.center()));
}

void PanIconWidget::setRegionSelection(QRect rs) {
    // rs is the rect of viewport (xy,wh) over entire image area (corrected
    // considering zoom)
    // store it
    this->regionSelection = rs;
    // and now scale it to this widget's size/original_image_size
    m_localRegionSelection.setX(/*m_rect.x() +*/ (
        int)((float)rs.x() * ((float)m_width / (float)m_orgWidth)));

    m_localRegionSelection.setY(/*m_rect.y() +*/ (
        int)((float)rs.y() * ((float)m_height / (float)m_orgHeight)));

    m_localRegionSelection.setWidth(
        (int)((float)rs.width() * ((float)m_width / (float)m_orgWidth)));

    m_localRegionSelection.setHeight(
        (int)((float)rs.height() * ((float)m_height / (float)m_orgHeight)));
}

void PanIconWidget::mousePressEvent(QMouseEvent *e) {
    if ((e->button() == Qt::LeftButton || e->button() == Qt::MidButton) &&
        m_localRegionSelection.contains(e->x(), e->y())) {
        xpos = e->x();
        ypos = e->y();
        moveSelection = true;
        //         emit signalSelectionTakeFocus();
    }
}

void PanIconWidget::mouseMoveEvent(QMouseEvent *e) {
    if (moveSelection &&
        (e->buttons() == Qt::LeftButton || e->buttons() == Qt::MidButton)) {
        int newxpos = e->x();
        int newypos = e->y();

        m_localRegionSelection.translate(newxpos - xpos, newypos - ypos);

        xpos = newxpos;
        ypos = newypos;

        // Perform normalization of selection area.

        if (m_localRegionSelection.left() < /*m_rect.left()*/ 0)
            m_localRegionSelection.moveLeft(/*m_rect.left()*/ 0);

        if (m_localRegionSelection.top() < /*m_rect.top()*/ 0)
            m_localRegionSelection.moveTop(/*m_rect.top()*/ 0);

        if (m_localRegionSelection.right() > /*m_rect.right()*/ width())
            m_localRegionSelection.moveRight(/*m_rect.right()*/ width());

        if (m_localRegionSelection.bottom() > /*m_rect.bottom()*/ height())
            m_localRegionSelection.moveBottom(/*m_rect.bottom()*/ height());

        update();
        regionSelectionMoved();
        return;
    } else {
        if (m_localRegionSelection.contains(e->x(), e->y()))
            QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
        else
            QApplication::restoreOverrideCursor();
    }
}

void PanIconWidget::mouseReleaseEvent(QMouseEvent *) {
    if (moveSelection) {
        moveSelection = false;
        QApplication::restoreOverrideCursor();
        regionSelectionMoved();
        emit finished();
    }
}

PanIconWidget::~PanIconWidget() {
    if (m_image) delete m_image;
}

void PanIconWidget::paintEvent(QPaintEvent *e) {
    if (m_image == NULL) return;
    QPainter p(this);
    p.drawImage(e->rect(), *m_image);

    p.setPen(QPen(Qt::white, 1, Qt::SolidLine));
    p.drawRect(m_localRegionSelection.x(), m_localRegionSelection.y(),
               m_localRegionSelection.width(), m_localRegionSelection.height());

    p.setPen(QPen(Qt::red, 1, Qt::DotLine));

    p.drawRect(m_localRegionSelection.x(), m_localRegionSelection.y(),
               m_localRegionSelection.width(), m_localRegionSelection.height());
    //     qDebug("m_image w=%d, h=%d",m_image->width(), m_image->height());
}

void PanIconWidget::popup(const QPoint &pos) {
    QRect d = QApplication::desktop()->screenGeometry();
    int x = pos.x();
    int y = pos.y();
    int w = width();
    int h = height();
    if (x + w > d.x() + d.width()) x = d.width() - w;
    if (y + h > d.y() + d.height()) y = d.height() - h;
    if (x < d.x()) x = 0;
    if (y < d.y()) y = 0;

    // Pop the thingy up.
    move(x, y);
    show();
}
// KPopupFrame::resizeEvent(QResizeEvent*)
// main->setGeometry(frameWidth(), frameWidth(),
//           width()-2*frameWidth(), height()-2*frameWidth());
