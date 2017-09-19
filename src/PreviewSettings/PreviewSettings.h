/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Franco Comida
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

#ifndef PREVIEWSETTINGS_IMPL_H
#define PREVIEWSETTINGS_IMPL_H

#include <QWidget>

#include "UI/FlowLayout.h"

// forward declaration
namespace pfs {
class Frame;  // #include "Libpfs/frame.h"
}

class TonemappingOptions;  // #include "Core/TonemappingOptions.h"
class PreviewLabel;        // #include "PreviewSettings/PreviewLabel.h"

class PreviewSettings : public QWidget {
    Q_OBJECT

   public:
    explicit PreviewSettings(QWidget *parent = 0);
    ~PreviewSettings();
    void addPreviewLabel(PreviewLabel *label);
    PreviewLabel *getPreviewLabel(int index);
    QSize getLabelSize();
    int getSize() { return m_ListPreviewLabel.size(); }
    void clear();

   protected:
    virtual void changeEvent(QEvent *event);

   public Q_SLOTS:
    void selectLabel(int index);
    void updatePreviews(pfs::Frame *frame);

   protected Q_SLOTS:
    void tonemapPreview(TonemappingOptions *);

   Q_SIGNALS:
    void startTonemapping(TonemappingOptions *);
    void triggered();

   private:
    int m_original_width_frame;
    QList<PreviewLabel *> m_ListPreviewLabel;
    FlowLayout *m_flowLayout;
};
#endif
