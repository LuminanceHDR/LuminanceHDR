/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2009 Franco Comida
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

//! \author Franco Comida <fcomida@users.sourceforge.net>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef PROGRESSHELPER_H
#define PROGRESSHELPER_H

#include <QObject>
#include "Libpfs/progress.h"

//! \brief glue between pfs::Progress and Qt signal/slot
class ProgressHelper : public QObject, public pfs::Progress {
    Q_OBJECT
   public:
    explicit ProgressHelper(QObject *p = 0);

    void setValue(int value);
    void setRange(int minimum, int maximum);
    void setMaximum(int maximum);
    void setMinimum(int minimum);

   public slots:
    void qtCancel();

   signals:
    void qtSetValue(int value);
    void qtSetRange(int minimum, int maximum);
    void qtSetMaximum(int max);
    void qtSetMinimum(int min);
};

#endif  // PROGRESSHELPER_H
