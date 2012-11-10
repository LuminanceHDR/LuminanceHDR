/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Davide Anastasia
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 */

#include "exif_data.hpp"

#include <exiv2/exiv2.hpp>
#include <cmath>
#include <iostream>

namespace pfs
{
namespace exif
{

namespace
{
// reflected-light meter calibration constant
const float K = 12.07488f;
// default ISO value for camera that do not report io
const float DEFAULT_ISO = 100.f;
// invalid value
const float INVALID_VALUE = -1.f;
// invalid value
const float INVALID_EV_VALUE = -100000.f;
// default EVCOMP value
const float DEFAULT_EVCOMP = 0.0f;

float log_base(float value, float base)
{
   return (std::log(value) / std::log(base));
}

}

exif_data::exif_data()
{
    reset();
}

exif_data::exif_data(const std::string& filename)
{
    reset();
    from_file(filename);
}

void exif_data::from_file(const std::string& filename)
{
    reset();
    try
    {
        ::Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename);
        image->readMetadata();
        ::Exiv2::ExifData &exifData = image->exifData();

        // if data is empty
        if (exifData.empty()) return;

        // Exiv2 iterator in read-only
        ::Exiv2::ExifData::const_iterator it = exifData.end();
        if ((it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ExposureTime"))) != exifData.end())
        {
            m_exposure_time = it->toFloat();
        }
        else if ((it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ShutterSpeedValue"))) != exifData.end())
        {
            long num = 1;
            long div = 1;
            float tmp = std::exp(std::log(2.0f) * it->toFloat());
            if (tmp > 1)
            {
                div = static_cast<long>(tmp + 0.5f);
            }
            else
            {
                num = static_cast<long>(1.0f/tmp + 0.5f);
            }
            m_exposure_time = static_cast<float>(num)/div;
        }


        if ((it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.FNumber"))) != exifData.end())
        {
            m_f_number = it->toFloat();
        }
        else if ((it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ApertureValue"))) != exifData.end())
        {
            m_f_number = static_cast<float>(expf(logf(2.0f) * it->toFloat() / 2.f));
        }
        // some cameras/lens DO print the fnum but with value 0, and this is not allowed for ev computation purposes.
        if (m_f_number == 0.0f)
        {
            m_f_number = INVALID_VALUE;
        }

        //if iso is found use that value, otherwise assume a value of iso=100. (again, some cameras do not print iso in exif).
        if ((it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ISOSpeedRatings"))) != exifData.end())
        {
            m_iso_speed = it->toFloat();
        }

        if ((it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ExposureBiasValue"))) != exifData.end())
        {
            m_ev_compensation = it->toFloat();
        }
    }
    catch (Exiv2::AnyError& e)
    {
        return;
    }
}

const float& exif_data::exposure_time() const
{
    return m_exposure_time;
}
bool exif_data::is_exposure_time() const
{
    return (m_exposure_time != INVALID_VALUE);
}
void exif_data::exposure_time(float et)
{
    m_exposure_time = et;
}

const float& exif_data::iso_speed() const
{
    return m_iso_speed;
}
bool exif_data::is_iso_speed() const
{
    return true;
}
void exif_data::iso_speed(float iso)
{
    m_iso_speed = iso;
}

const float& exif_data::f_number() const
{
    return m_f_number;
}
bool exif_data::is_f_number() const
{
    return (m_f_number != INVALID_VALUE);
}
void exif_data::f_number(float fnum)
{
    m_f_number = fnum;
}

float exif_data::exposure_value() const
{
    if ( is_f_number() && is_exposure_time() )
    {
        return log_base((m_f_number*m_f_number)/m_exposure_time, 2.0f);
    }
    return INVALID_EV_VALUE;
}
bool exif_data::is_exposure_value() const
{
    return (exposure_value() != INVALID_EV_VALUE);
}

const float& exif_data::exposure_value_compensation() const
{
    return m_ev_compensation;
}
bool exif_data::is_exposure_value_compensation() const
{
    return (m_ev_compensation != 0.0f);
}
void exif_data::exposure_value_compensation(float evcomp)
{
    m_ev_compensation = evcomp;
}

float exif_data::average_scene_luminance() const
{
    if ( is_iso_speed() && is_f_number() && is_exposure_time() )
    {
        return ( (m_exposure_time * m_iso_speed) / (m_f_number*m_f_number*K) );
    }
    return INVALID_VALUE;
}

void exif_data::reset()
{
    // reset internal value
    m_exposure_time = INVALID_VALUE;
    m_iso_speed = DEFAULT_ISO;
    m_f_number = INVALID_VALUE;
    m_ev_compensation = DEFAULT_EVCOMP;
}

bool exif_data::is_valid() const
{
    return ( is_iso_speed() && is_f_number() && is_exposure_time() );
}

std::ostream& operator<<(std::ostream& out, const exif_data& exif_data)
{
    out << "Exposure time = " << exif_data.m_exposure_time << ", ";
    out << "F value = " << exif_data.m_f_number << ", ";
    out << "ISO = " << exif_data.m_iso_speed << ", ";
    out << "Exposure value = " << exif_data.exposure_value() << " (" << exif_data.m_ev_compensation << "), ";
    out << "Average Scene Luminance = " << exif_data.average_scene_luminance();

    return out;
}

}   // namespace exif
}   // namespace LibHDR
