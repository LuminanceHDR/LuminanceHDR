/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Franco Comida
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

#ifndef SIMPLEPREVIEWLABEL_H
#define SIMPLEPREVIEWLABEL_H

#include <QLabel>
#include <QMouseEvent>

class SimplePreviewLabel : public QLabel {
    Q_OBJECT

   public:
    SimplePreviewLabel(int index, QWidget *parent = 0);
    ~SimplePreviewLabel();

    bool isSelected() { return m_selected; }

   signals:
    void selected(int);

   protected:
    void mousePressEvent(QMouseEvent *event);
    int m_index;
    bool m_selected;
};

#endif
