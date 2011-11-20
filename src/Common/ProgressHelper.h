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

#ifndef PROGRESS_HELPER
#define PROGRESS_HELPER

#include <QObject>

class ProgressHelper : public QObject {
Q_OBJECT

public:
	ProgressHelper(QObject *p = 0);

        ///! backcompatibility
        void newValue(int progress)
        {
            emitSetValue(progress);
        }
        void emitSetValue(int progress);
        void emitSetMaximum(int max);
        void emitSetMinimum(int min);

        bool isTerminationRequested();

public slots:
        void terminate(bool b = true);
private:
	bool m_terminate;
signals:
        void setValue(int progress);
        void setMaximum(int max);
        void setMinimum(int min);
};

#endif
