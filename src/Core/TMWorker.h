/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2011 Davide Anastasia
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
 * Original Work
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#ifndef __TM_WORKER_H__
#define __TM_WORKER_H__

#include <QObject>
#include <QString>

// Forward declaration
namespace pfs {
    class Frame;
}

class TonemappingOptions;

class TMWorker: public QObject
{
    Q_OBJECT

public:
    TMWorker(QObject* parent = 0);
    ~TMWorker();

public Q_SLOTS:
    pfs::Frame* getTonemappedFrame(/* const */pfs::Frame*, TonemappingOptions*);
    void tonemapFrame(pfs::Frame*, TonemappingOptions*);

private:
    pfs::Frame* preProcessFrame(pfs::Frame*, TonemappingOptions*);
    void postProcessFrame(pfs::Frame*, TonemappingOptions*);

Q_SIGNALS:
    void tonemappingSuccess(pfs::Frame*, TonemappingOptions*);
    void tonemappingFailed(QString);
};

#endif
