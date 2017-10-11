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
 *
 */

#ifndef TONEMAPPINGSETTINGS_H
#define TONEMAPPINGSETTINGS_H

#include <QDialog>
#include <QSqlTableModel>

#include <Core/TonemappingOptions.h>
#include <Libpfs/frame.h>
#include <PreviewSettings/PreviewSettings.h>

namespace Ui {
class TonemappingSettings;
}

class TonemappingSettings : public QDialog {
    Q_OBJECT

   public:
    TonemappingSettings(QWidget *parent = 0, pfs::Frame *frame = NULL, QString databaseconnection = "");
    ~TonemappingSettings();
    TonemappingOptions *getTonemappingOptions();
    bool wantsTonemap() { return m_wantsTonemap; }

   protected:
    void fillPreviews();
    pfs::Frame *m_frame;
    PreviewSettings *m_previewSettings;
    QSqlQueryModel *m_modelPreviews;
    int m_currentIndex;
    QList<PreviewLabel *> m_previewLabelList;
    bool m_wantsTonemap;
    QString m_databaseconnection;
    QScopedPointer<Ui::TonemappingSettings> m_Ui;

   protected Q_SLOTS:
    void listWidgetChanged(int row);
    void updateListView(int);
    void sortPreviews(int);
    void tonemapPreview(TonemappingOptions *);
    void on_btnTonemap_clicked();

   private:
    void addPreview(PreviewLabel *previewLabel, const QSqlRecord &record);
    void fillCommonValues(TonemappingOptions *tmOptions, int origxsize,
                          int previewWidth, TMOperator tOperator,
                          const QSqlRecord &record);
};

#endif
