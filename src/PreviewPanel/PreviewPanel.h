/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2011 Franco Comida
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

#ifndef PREVIEWPANEL_IMPL_H
#define PREVIEWPANEL_IMPL_H

#include <QWidget>

// forward declaration
namespace pfs {
class Frame;  // #include "Libpfs/frame.h"
}

namespace Ui {
class PreviewPanel;
}

class TonemappingOptions;  // #include "Core/TonemappingOptions.h"
class PreviewLabel;        // #include "PreviewPanel/PreviewLabel.h"

class PreviewPanel : public QWidget {
    Q_OBJECT

   public:
    explicit PreviewPanel(QWidget *parent = 0);
    ~PreviewPanel();
    QSize getLabelSize();
    PreviewLabel *getLabel(int);

   public Q_SLOTS:
    void updatePreviews(pfs::Frame *frame, int index = -1);
    void setAutolevels(bool, float);

   protected Q_SLOTS:
    void tonemapPreview(TonemappingOptions *);

   Q_SIGNALS:
    void startTonemapping(TonemappingOptions *);

   private:
    int m_original_width_frame;
    bool m_doAutolevels;
    float m_autolevelThreshold;
    QVector<PreviewLabel *> m_ListPreviewLabel;
};
#endif
