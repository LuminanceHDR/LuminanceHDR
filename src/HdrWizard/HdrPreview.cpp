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


#include "HdrWizard/ui_HdrPreview.h"
#include "HdrWizard/HdrPreview.h"

HdrPreview::HdrPreview(QDialog *parent)
    : QDialog(parent),
      m_Ui(new Ui::HdrPreview),
      m_viewer(new HdrViewer(NULL)) {

    m_Ui->setupUi(this);
    m_Ui->hdrPreviewFrame->layout()->addWidget(m_viewer.data());
    m_viewer->setFocus();
}

HdrPreview::~HdrPreview() {
}

void HdrPreview::showEvent(QShowEvent *) {
    m_viewer->setFocus();
}
