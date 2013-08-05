/**
 * This file is a part of LuminanceHDR package.
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

#ifndef FITSIMPORTER_H
#define FITSIMPORTER_H

#include <QDialog>

namespace Ui {
class FitsImporter;
}

class FitsImporter : public QDialog 
{
    Q_OBJECT

public:
    FitsImporter(QWidget *parent = 0);
    ~FitsImporter();

    QString getLuminosityChannel() { return m_luminosityChannel; };
    QString getRedChannel() { return m_redChannel; };
    QString getGreenChannel() { return m_greenChannel; };
    QString getBlueChannel() { return m_blueChannel; };
    QString getHChannel() { return m_hChannel; };

protected slots:
    void on_pushButtonLuminosity_clicked();
    void on_pushButtonRed_clicked();
    void on_pushButtonGreen_clicked();
    void on_pushButtonBlue_clicked();
    void on_pushButtonH_clicked();

protected:
    void checkOKButton();

    QString m_luminosityChannel;
    QString m_redChannel;
    QString m_greenChannel;
    QString m_blueChannel;
    QString m_hChannel;

    QScopedPointer<Ui::FitsImporter> m_ui;
};

#endif 
