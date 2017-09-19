/**
 * This file is a part of Luminance HDR package
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
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#include <QImage>
#include <QString>

#include <Libpfs/frame.h>
#include "HdrCreationItem.h"

HdrCreationItem::HdrCreationItem(const QString &filename)
    : m_filename(filename),
      m_convertedFilename(filename),
      m_alignedFilename(filename),
      m_averageLuminance(-1.f),
      m_exposureTime(-1.f),
      m_datamin(0.f),
      m_datamax(1.f),
      m_frame(std::make_shared<pfs::Frame>()),
      m_thumbnail(new QImage()) {
    // qDebug() << QString("Building HdrCreationItem for %1").arg(m_filename);
}

HdrCreationItem::HdrCreationItem(const QString &filename,
                                 const QString &convertedFilename)
    : m_filename(filename),
      m_convertedFilename(convertedFilename),
      m_alignedFilename(convertedFilename),
      m_averageLuminance(-1.f),
      m_exposureTime(-1.f),
      m_datamin(0.f),
      m_datamax(1.f),
      m_frame(std::make_shared<pfs::Frame>()),
      m_thumbnail(new QImage()) {}

HdrCreationItem::~HdrCreationItem() {
    // qDebug() << QString("Destroying HdrCreationItem for %1").arg(m_filename);
}
