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

#ifndef PREVIEWFRAME_H
#define PREVIEWFRAME_H

#include <QFrame>
#include "FlowLayout.h"

#include "SimplePreviewLabel.h"

class PreviewFrame : public QFrame {
    Q_OBJECT

   public:
    PreviewFrame(QWidget *parent = 0);
    ~PreviewFrame();

    void addLabel(SimplePreviewLabel *label);
    SimplePreviewLabel *getLabel(int index) { return m_labels[index]; }
    int getSelectedLabel() { return m_index; }

   public slots:
    void selectLabel(int index);

   protected:
    QList<SimplePreviewLabel *> m_labels;
    int m_index;
    FlowLayout *m_flowLayout;
};

#endif
