/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2016 Franco Comida
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
 * @author Franco Comida <francocomida@users.sourceforge.net>
 */

#ifndef EXPORTTOHTMLDIALOG_H
#define EXPORTTOHTMLDIALOG_H

#include <QDialog>
#include <QScopedPointer>

namespace Ui {
class ExportToHtmlDialog;
}

namespace pfs {
class Frame;
}

class ExportToHtmlDialog : public QDialog {
    Q_OBJECT
   public:
    ExportToHtmlDialog(QWidget *parent, pfs::Frame *frame);

   private:
    pfs::Frame *m_frame;
    QString m_pageName;
    QString m_outputFolder;
    QString m_imagesFolder;

    void check_enable_export();

    QScopedPointer<Ui::ExportToHtmlDialog> m_Ui;

   private slots:
    void onExportButtonClicked();
    void onOutputFolderButtonClicked();
    void onEditPageNameFinished();
    void onEditOutputFolderFinished();
    void onEditImagesFolderFinished();

   public:
    ~ExportToHtmlDialog();
};

#endif  // EXPORTTOHTMLDIALOG_H
