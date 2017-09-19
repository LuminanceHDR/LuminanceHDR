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

#include "PreviewFrame.h"

PreviewFrame::PreviewFrame(QWidget *parent) : QFrame(parent), m_index(0) {
    m_flowLayout = new FlowLayout;
    setLayout(m_flowLayout);
    QPalette *palette = new QPalette();
    palette->setColor(QPalette::Foreground, Qt::red);
    setPalette(*palette);
}

PreviewFrame::~PreviewFrame() {}

void PreviewFrame::addLabel(SimplePreviewLabel *label) {
    connect(label, &SimplePreviewLabel::selected, this,
            &PreviewFrame::selectLabel);
    label->setFrameStyle(QFrame::Box);
    m_labels.push_back(label);
    m_flowLayout->addWidget(label);
}

void PreviewFrame::selectLabel(int index) {
    m_labels[m_index]->setLineWidth(1);
    m_index = index;
    m_labels[m_index]->setLineWidth(3);
}
