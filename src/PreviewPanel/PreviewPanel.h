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
#include <QPixmap>
#include <QImage>

#include "PreviewLabel.h"
#include "ui_PreviewPanel.h"

#include "Libpfs/frame.h"
#include "Core/TonemappingOptions.h"
#include "Threads/TMOThread.h"

class PreviewPanel : public QWidget, private Ui::PreviewPanel
{
    Q_OBJECT

public:
    PreviewPanel(QWidget *parent = 0);
    ~PreviewPanel();

public Q_SLOTS:
    void updatePreviews(pfs::Frame* frame);

protected Q_SLOTS:
    void addSmallPreviewResult(QImage*);
    void setPixmap(const QPixmap &p, int n);
    void tonemapPreview(int n);
    void deleteTMOThread(TMOThread *th);
    void showError(const char *);

protected:
    PreviewLabel *labelMantiuk06;
    PreviewLabel *labelMantiuk08;
    PreviewLabel *labelFattal;
    PreviewLabel *labelDrago;
    PreviewLabel *labelDurand;
    PreviewLabel *labelReinhard02;
    PreviewLabel *labelReinhard05;
    PreviewLabel *labelAshikhmin;
    PreviewLabel *labelPattanaik;

    void setupConnections();

Q_SIGNALS:
    void startTonemapping(TonemappingOptions*);

private:
    bool is_frame_set;
    int original_width_frame;
    pfs::Frame* current_frame;
    TonemappingOptions *opts;
    int m_previewImgNum;

    void generatePreviews();

    QList<TonemappingOptions> list_previews;
    QHash<int, TMOperator> id_tm_operator;

    void buildPreviewsDataStructure();
};
#endif
