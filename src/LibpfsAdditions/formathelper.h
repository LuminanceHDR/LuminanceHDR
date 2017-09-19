/*
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Daniel Kaneider
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

//! @author Daniel Kaneider <danielkaneider@users.sourceforge.net>

#ifndef LIBPFSADDITIONS_FORMATHELPER_H
#define LIBPFSADDITIONS_FORMATHELPER_H

#include <QAbstractButton>
#include <QComboBox>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include "Common/LuminanceOptions.h"

#include <Libpfs/params.h>

namespace pfsadditions {

class FormatHelper : public QObject {
    Q_OBJECT

   public:
    FormatHelper();

    //! \brief virtual dtor, enable derivation
    ~FormatHelper() {}

    void initConnection(QComboBox *comboBox, QAbstractButton *settingsButton,
                        bool hdr);
    pfs::Params getParams();
    QString getFileExtension();

    void loadFromSettings(const QString prefix);
    void writeSettings(const QString prefix);

    static pfs::Params getParamsFromSettings(const QString prefix, bool hdr);

   protected:
    void setDefaultParams(int format);

    void updateButton(int format);

    static pfs::Params getParamsForFormat(int format);
    static QString getFileExtensionForFormat(int format);

   protected slots:
    void comboBoxIndexChanged(int idx);
    void buttonPressed();

   private:
    QComboBox *m_comboBox;
    QAbstractButton *m_settingsButton;
    pfs::Params m_params;
    bool m_hdr;
};
}

#endif  // LIBPFSADDITIONS_FORMATHELPER_H
