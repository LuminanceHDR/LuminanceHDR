/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2018 Franco Comida
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

#ifndef HDRPREVIEW_H
#define HDRPREVIEW_H

#include <QDialog>

#include "Viewers/HdrViewer.h"

namespace Ui {
class HdrPreview;
}

class HdrPreview : public QDialog {
    Q_OBJECT
    public:

    HdrPreview(QDialog *parent);
    ~HdrPreview();

    void setFrame(std::shared_ptr<pfs::Frame> frame) {m_viewer->setFrameShared(frame); }

    protected:
    QScopedPointer<Ui::HdrPreview> m_Ui;
    QSharedPointer<HdrViewer> m_viewer;

    void showEvent(QShowEvent *);
};

#endif
