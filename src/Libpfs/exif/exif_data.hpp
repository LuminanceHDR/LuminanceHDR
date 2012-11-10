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

#include <string>
#include <stdexcept>
#include <iosfwd>

namespace pfs
{
//! \namespace Contains all the operations based on EXIF data
namespace exif
{

//! \class exif_data
//! \brief Holds Exif Data
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \
//!
class exif_data
{
public:
    //! \brief empty ctor
    exif_data();

    //!
    //! \param[in] filename Name of source file
    explicit exif_data(const std::string& filename);

    //! \brief read exif data from file
    //! \param[in] filename Name of source file
    void from_file(const std::string& filename);

    const float& exposure_time() const;
    bool is_exposure_time() const;
    void exposure_time(float et);

    const float& iso_speed() const;
    bool is_iso_speed() const;
    void iso_speed(float iso);

    const float& f_number() const;
    bool is_f_number() const;
    void f_number(float fnum);

    float exposure_value() const;
    bool is_exposure_value() const;

    const float& exposure_value_compensation() const;
    bool is_exposure_value_compensation() const;
    void exposure_value_compensation(float evcomp);

    //! \brief This function obtains the "average scene luminance" (cd/m^2) from
    //! an image file.
    //! "average scene luminance" is the L (aka B) value mentioned in [1]
    //! You have to take a log2f of the returned value to get an EV value.
    //!
    //! We are using K=12.07488f and the exif-implied value of N=1/3.125 (see [1]).
    //! K=12.07488f is the 1.0592f * 11.4f value in pfscalibration's
    //! pfshdrcalibrate.cpp file.
    //! Based on [3] we can say that the value can also be 12.5 or even 14.
    //! Another reference for APEX is [4] where N is 0.3, closer to the APEX
    //! specification of 2^(-7/4)=0.2973.
    //!
    //! [1] http://en.wikipedia.org/wiki/APEX_system
    //! [2] http://en.wikipedia.org/wiki/Exposure_value
    //! [3] http://en.wikipedia.org/wiki/Light_meter
    //! [4] http://doug.kerr.home.att.net/pumpkin/#APEX
    //!
    //! This function tries first to obtain the shutter speed from either of
    //! two exif tags (there is no standard between camera manifacturers):
    //! ExposureTime or ShutterSpeedValue.
    //! Same thing for f-number: it can be found in FNumber or in ApertureValue.
    //!
    //! F-number and shutter speed are mandatory in exif data for EV
    //! calculation, iso is not.
    //! \note This description is copied from the original source code in
    //! Luminance HDR http://qtpfsgui.sourceforge.net/
    float average_scene_luminance() const;

    //! \brief reset Exif Data
    void reset();

    //! \brief returns whether enough information are available to compute
    //! additional values
    bool is_valid() const;

    friend std::ostream& operator<<(std::ostream& out,
                                    const exif_data& exifdata);
private:
    float m_exposure_time;
    float m_iso_speed;
    float m_f_number;
    float m_ev_compensation;
};

std::ostream& operator<<(std::ostream& out, const exif_data& exifdata);

} // exif
} // pfs
