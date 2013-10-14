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

#include "ImageInspectorStats.h"

#include <cfloat>
#include <cmath>
#include <ostream>
#include <sstream>
#include "arch/math.h"

using namespace std;

void ImageInspectorStats::operator()(float value)
{
    m_parsedSamples++;

    if ( isnan(value) )
    {
        m_numNan++;
        return;
    }

    if ( !finite(value) )
    {
        m_numInf++;
        return;
    }

    if ( value > m_max )
        m_max = value;
    else if ( value < m_min )
        m_min = value;
}

std::ostream& operator<<(std::ostream& out, const ImageInspectorStats& stats)
{
    stringstream ss;

    ss << "{ Samples: " << stats.m_parsedSamples << ", ";
    ss << "Min: " << stats.m_min << ", ";
    ss << "Max: " << stats.m_max << ", ";
    ss << "NaN: " << stats.m_numNan << ", ";
    ss << "Inf: " << stats.m_numInf << "}";

    return (out << ss.str());
}
