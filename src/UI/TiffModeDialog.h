/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Davide Anastasia
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
 */
#ifndef TIFFMODEDIALOG_H
#define TIFFMODEDIALOG_H

#include <QDialog>
#include <QScopedPointer>

#include "Common/LuminanceOptions.h"

namespace Ui {
class TiffModeDialog;
}

class TiffModeDialog : public QDialog {
    Q_OBJECT

   public:
    explicit TiffModeDialog(bool hdrMode = false, int defaultValue = -1,
                            QWidget *parent = 0);
    ~TiffModeDialog();

    int getTiffWriterMode();

   private:
    bool m_hdrMode;
    QScopedPointer<Ui::TiffModeDialog> m_ui;
    QScopedPointer<LuminanceOptions> m_options;
};

#endif  // TIFFMODEDIALOG_H
