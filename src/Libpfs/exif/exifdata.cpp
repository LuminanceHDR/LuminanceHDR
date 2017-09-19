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

#include "exifdata.hpp"

#include <cmath>
#include <exiv2/exiv2.hpp>
#include <iostream>

namespace pfs {
namespace exif {

namespace {
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

float log_base(float value, float base) {
    return (std::log(value) / std::log(base));
}
}

ExifData::ExifData() { reset(); }

ExifData::ExifData(const std::string &filename) { fromFile(filename); }

void ExifData::fromFile(const std::string &filename) {
    reset();
    try {
        ::Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename);
        image->readMetadata();
        ::Exiv2::ExifData &exifData = image->exifData();

        // if data is empty
        if (exifData.empty()) return;

        // Exiv2 iterator in read-only
        ::Exiv2::ExifData::const_iterator it = exifData.end();
        if ((it = exifData.findKey(Exiv2::ExifKey(
                 "Exif.Photo.ExposureTime"))) != exifData.end()) {
            m_exposureTime = it->toFloat();
        } else if ((it = exifData.findKey(Exiv2::ExifKey(
                        "Exif.Photo.ShutterSpeedValue"))) != exifData.end()) {
            long num = 1;
            long div = 1;
            float tmp = std::exp(std::log(2.0f) * it->toFloat());
            if (tmp > 1) {
                div = static_cast<long>(tmp + 0.5f);
            } else {
                num = static_cast<long>(1.0f / tmp + 0.5f);
            }
            m_exposureTime = static_cast<float>(num) / div;
        }

        if ((it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.FNumber"))) !=
            exifData.end()) {
            m_FNumber = it->toFloat();
        } else if ((it = exifData.findKey(Exiv2::ExifKey(
                        "Exif.Photo.ApertureValue"))) != exifData.end()) {
            m_FNumber =
                static_cast<float>(expf(logf(2.0f) * it->toFloat() / 2.f));
        }
        // some cameras/lens DO print the fnum but with value 0, and this is not
        // allowed for ev computation purposes.
        if (m_FNumber == 0.0f) {
            m_FNumber = INVALID_VALUE;
        }

        // if iso is found use that value, otherwise assume a value of iso=100.
        // (again, some cameras do not print iso in exif).
        if ((it = exifData.findKey(Exiv2::ExifKey(
                 "Exif.Photo.ISOSpeedRatings"))) != exifData.end()) {
            m_isoSpeed = it->toFloat();
        }

        if ((it = exifData.findKey(Exiv2::ExifKey(
                 "Exif.Photo.ExposureBiasValue"))) != exifData.end()) {
            m_EVCompensation = it->toFloat();
        }

        // exif orientation --------
        /*
         *           http://jpegclub.org/exif_orientation.html
         *
         *
         *       Value   0th Row     0th Column
         *           1   top         left side
         *           2   top         right side
         *           3   bottom      right side
         *           4   bottom      left side
         *           5   left side   top
         *           6   right side  top
         *           7   right side  bottom
         *           8   left side   bottom
         *
         */
        if ((it = exifData.findKey(Exiv2::ExifKey("Exif.Image.Orientation"))) !=
            exifData.end()) {
            long rotation = it->toLong();
            switch (rotation) {
                case 3:
                    m_orientation = 180;
                    break;
                case 6:
                    m_orientation = 90;
                    break;
                case 8:
                    m_orientation = 270;
                    break;
            }
        }
    } catch (Exiv2::AnyError &e) {
        return;
    }
}

const float &ExifData::getExposureTime() const { return m_exposureTime; }
bool ExifData::hasExposureTime() const {
    return (m_exposureTime != INVALID_VALUE);
}
void ExifData::setExposureTime(float et) { m_exposureTime = et; }

const float &ExifData::getIsoSpeed() const { return m_isoSpeed; }
bool ExifData::hasIsoSpeed() const { return true; }
void ExifData::setIsoSpeed(float iso) { m_isoSpeed = iso; }

const float &ExifData::getFNumber() const { return m_FNumber; }
bool ExifData::hasFNumber() const { return (m_FNumber != INVALID_VALUE); }
void ExifData::setFNumber(float fnum) { m_FNumber = fnum; }

float ExifData::getExposureValue() const {
    if (hasFNumber() && hasExposureTime()) {
        return log_base((m_FNumber * m_FNumber) / m_exposureTime, 2.0f);
    }
    return INVALID_EV_VALUE;
}
bool ExifData::hasExposureValue() const {
    return (getExposureValue() != INVALID_EV_VALUE);
}

const float &ExifData::getExposureValueCompensation() const {
    return m_EVCompensation;
}
bool ExifData::hasExposureValueCompensation() const {
    return (m_EVCompensation != 0.0f);
}
void ExifData::setExposureValueCompensation(float evcomp) {
    m_EVCompensation = evcomp;
}

float ExifData::getAverageSceneLuminance() const {
    if (hasIsoSpeed() && hasFNumber() && hasExposureTime()) {
        return ((m_exposureTime * m_isoSpeed) / (m_FNumber * m_FNumber * K));
    }
    return INVALID_VALUE;
}

short ExifData::getOrientationDegree() const { return m_orientation; }

void ExifData::reset() {
    // reset internal value
    m_exposureTime = INVALID_VALUE;
    m_isoSpeed = DEFAULT_ISO;
    m_FNumber = INVALID_VALUE;
    m_EVCompensation = DEFAULT_EVCOMP;
    m_orientation = 0;
}

bool ExifData::isValid() const {
    return (hasIsoSpeed() && hasFNumber() && hasExposureTime());
}

std::ostream &operator<<(std::ostream &out, const ExifData &exifData) {
    out << "Exposure time = " << exifData.m_exposureTime << ", ";
    out << "F value = " << exifData.m_FNumber << ", ";
    out << "ISO = " << exifData.m_isoSpeed << ", ";
    out << "Exposure value = " << exifData.getExposureValue() << " ("
        << exifData.m_EVCompensation << "), ";
    out << "Average Scene Luminance = " << exifData.getAverageSceneLuminance();

    return out;
}

}  // namespace exif
}  // namespace LibHDR
