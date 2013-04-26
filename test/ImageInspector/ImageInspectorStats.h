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
//! \date October 21st, 2012

#ifndef IMAGEINSPECTORSTATS_H
#define IMAGEINSPECTORSTATS_H

#include <cstddef>
#include <limits>
#include <iosfwd>

class ImageInspectorStats
{
public:
    ImageInspectorStats()
        : m_parsedSamples(0)
        , m_max( -std::numeric_limits<float>::max() )
        , m_min( std::numeric_limits<float>::max() )
        , m_numInf(0)
        , m_numNan(0)
    {}

    void operator()(float value);

    friend std::ostream& operator<<(std::ostream& out, const ImageInspectorStats& stats);

private:
    size_t m_parsedSamples;

    float m_max;
    float m_min;

    size_t m_numInf;
    size_t m_numNan;
};

std::ostream& operator<<(std::ostream& out, const ImageInspectorStats& stats);

#endif // IMAGEINSPECTORSTATS_H
