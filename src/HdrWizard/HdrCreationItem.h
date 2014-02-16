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
#ifndef HDRCREATIONITEM_H
#define HDRCREATIONITEM_H

#include <QSharedPointer>
#include <QImage>
#include <QString>
#include <Libpfs/frame.h>

#include <cmath>
#include "arch/math.h"

// defines an element that contains all the informations for this particular
// image to be used inside the HdrWizard
class HdrCreationItem
{
public:
    HdrCreationItem(const QString& filename);
    ~HdrCreationItem();

    const QString& filename() const     { return m_filename; }
    const pfs::FramePtr& frame() const  { return m_frame; }
    pfs::FramePtr& frame()              { return m_frame; }
    bool isValid() const                { return m_frame->isValid(); }

    bool hasAverageLuminance() const    { return (m_averageLuminance != -1.f); }
    void setAverageLuminance(float avl) { m_averageLuminance = avl; }
    float getAverageLuminance() const   { return m_averageLuminance; }

    bool hasExposureTime() const        { return (m_exposureTime != -1.f); }
    void setExposureTime(float e)       { m_exposureTime = e; }
    float getExposureTime() const       { return m_exposureTime; }

    bool hasEV() const                  { return hasAverageLuminance(); }
    void setEV(float ev)                { m_averageLuminance = std::pow(2.f, ev); }
    float getEV() const                 { return log2(m_averageLuminance); }

    QImage& qimage()                    { return *m_thumbnail; }
    const QImage& qimage() const        { return *m_thumbnail; }

private:
    QString                 m_filename;
    float                   m_averageLuminance;
    float                   m_exposureTime;
    pfs::FramePtr           m_frame;
    QSharedPointer<QImage>  m_thumbnail;
};

typedef std::vector< HdrCreationItem > HdrCreationItemContainer;

#endif
