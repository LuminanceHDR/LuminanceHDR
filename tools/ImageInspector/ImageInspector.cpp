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

#include "ImageInspector.h"

#include <ostream>

#include <QFile>
#include "Libpfs/frame.h"

using namespace std;

ImageInspector::ImageInspector()
    : m_ioWorker(new IOWorker)
{}

ImageInspector::~ImageInspector()
{}

bool ImageInspector::inspect(const QString &filename, std::ostream &out)
{
    using namespace pfs;

    m_currentFilename = filename;
    if ( !QFile::exists( filename ) )
    {
        m_currentFrame.reset();
        m_stats = ImageInspectorStats();    // reset stats

        out << "File " << m_currentFilename.toLocal8Bit().constData()
            << " does not exist" << endl;

        return false;
    }

    out << "Start reading..." << std::flush;

    m_currentFrame.reset( m_ioWorker->read_hdr_frame(filename) );

    out << " done!" << std::endl;

    if ( !m_currentFrame )
    {
        out << "I cannot read " << m_currentFilename.toLocal8Bit().constData()
            << endl;

        m_stats = ImageInspectorStats();    // reset stats
        return false;
    }

    // print stats
    out << "Size: "
        << m_currentFrame->getWidth() << " x "
        << m_currentFrame->getHeight() << endl;

    ChannelContainer& channels = m_currentFrame->getChannels();
    out << "Channels: " << channels.size() << endl;

    for (ChannelContainer::const_iterator it = channels.begin(), itEnd = channels.end();
         it != itEnd;
         ++it)
    {
        const float* dataStart = (*it)->getRawData();
        const float* dataEnd = dataStart + ((*it)->getWidth()*(*it)->getHeight());

        ImageInspectorStats t = std::for_each(dataStart, dataEnd, ImageInspectorStats());

        out << "Channel: " << (*it)->getName() << std::endl;
        out << t << std::endl;
    }

    return true;
}
