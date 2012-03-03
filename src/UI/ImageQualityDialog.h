/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2009 Franco Comida
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

#ifndef IMAGEQUALITYDIALOG_IMPL_H
#define IMAGEQUALITYDIALOG_IMPL_H

#include <QDialog>

namespace Ui {
    class ImgQualityDialog;
}

class ImageQualityDialog : public QDialog //, private Ui::ImgQualityDialog
{
    Q_OBJECT

public:
    ImageQualityDialog(const QImage *img, QString fmt, QWidget *parent = 0);
    ~ImageQualityDialog();
    int getQuality(void);
protected slots:
    void on_getSizeButton_clicked();
    void reset(int);

protected:
    const QImage *image;
    QString format;

    QScopedPointer<Ui::ImgQualityDialog> m_Ui;
};
#endif
