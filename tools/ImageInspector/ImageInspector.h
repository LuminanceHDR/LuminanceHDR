/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Davide Anastasia
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
 */

//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \date October 20th, 2012

#ifndef IMAGEINSPECTOR_H
#define IMAGEINSPECTOR_H

#include <iosfwd>

#include <QObject>
#include <QString>
#include <QScopedPointer>

#include <Core/IOWorker.h>
#include <Libpfs/frame.h>

#include "ImageInspectorStats.h"

class ImageInspector : public QObject
{
    Q_OBJECT
public:
    ImageInspector();
    ~ImageInspector();

    //! \return true if the inspection has been completed
    bool inspect(const QString& filename, std::ostream &out);

private:
    // IO Worker
    QScopedPointer<IOWorker> m_ioWorker;

    // Current file stats
    QString m_currentFilename;
    QScopedPointer<pfs::Frame> m_currentFrame;

    ImageInspectorStats m_stats;
};

#endif // IMAGEINSPECTOR_H
