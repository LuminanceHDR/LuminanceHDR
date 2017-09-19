/**
 * This file is a part of Luminance HDR package.
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

#include <QMenu>

#include "PreviewLabel.h"

PreviewLabel::PreviewLabel(QWidget *parent, TMOperator tm_operator)
    : QLabel(parent),
      m_TMOptions(new TonemappingOptions),
      m_index(-1),
      m_isFromPanel(true) {
    m_TMOptions->tmoperator = tm_operator;
}

PreviewLabel::PreviewLabel(QWidget *parent,
                           TonemappingOptions *tonemappingOptions, int index)
    : QLabel(parent),
      m_TMOptions(tonemappingOptions),
      m_index(index),
      m_isFromPanel(false) {}

PreviewLabel::~PreviewLabel() { delete m_TMOptions; }

void PreviewLabel::mousePressEvent(QMouseEvent *event) {
    if (event->buttons() == Qt::LeftButton) {
        (m_isFromPanel) ? emit clicked(m_TMOptions) : emit clicked(m_index);
    } else if (event->buttons() == Qt::RightButton) {
        QMenu menu(this);
        menu.addActions(actions());
        menu.exec(event->globalPos());
    }
}

void PreviewLabel::mouseDoubleClickEvent(QMouseEvent *event) {
    emit clicked(m_TMOptions);
}

void PreviewLabel::assignNewQImage(QSharedPointer<QImage> new_qimage) {
    setPixmap(QPixmap::fromImage(*new_qimage));
    adjustSize();
}

void PreviewLabel::setComment(QString comment) { m_comment = comment; }

QString PreviewLabel::getComment() { return m_comment; }

void PreviewLabel::setIndex(int index) { m_index = index; }
