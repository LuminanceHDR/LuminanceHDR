/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2007 Giuseppe Rota
 * Copyright (C) 2012 Franco Comida
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
 */

//! \author Giuseppe Rota <grota@users.sourceforge.net>
//! \author Franco Comida
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#include <QDebug>

#include <cmath>
#include <iostream>
#include <set>
#include <string>

#include <boost/assign/list_of.hpp>

#include <exif.hpp>
#include <image.hpp>

#include "Common/config.h"
#include "ExifOperations.h"
#include "arch/math.h"

using namespace boost;
using namespace boost::assign;

namespace ExifOperations {
typedef std::set<std::string> KeysDictionary;

struct ValidExifDictionary {
    // Note: don't use tagLabel, as it might contain white spaces
    bool operator()(const std::string &groupName, const std::string &tagName,
                    const std::string &key) const {
        // support for GPSInfo
        if (groupName == "GPSInfo") return true;
        // remove info about thumbnail
        if (groupName == "Thumbnail") return false;

        // search into the map if I don't find any better match
        return sm_validExifTags.count(key);
    }

   private:
    static KeysDictionary sm_validExifTags;
};

KeysDictionary ValidExifDictionary::sm_validExifTags = list_of
    // should I keep this? Isn't the version written by Exiv2?
    //        ("Exif.Photo.ExifVersion")
    // Exif.Image
    ("Exif.Image.Software")("Exif.Image.ImageDescription")(
        "Exif.Image.Orientation")("Exif.Image.Make")("Exif.Image.Model")(
        "Exif.Image.DateTime")("Exif.Image.ProcessingSoftware")(
        "Exif.Image.GPSTag")("Exif.Image.SensingMethod")(
        "Exif.Image.MaxApertureValue")("Exif.Image.SelfTimerMode")(
        "Exif.Image.ShutterSpeedValue")("Exif.Image.ApertureValue")(
        "Exif.Image.ExposureBiasValue")("Exif.Image.SubjectDistance")(
        "Exif.Image.MeteringMode")("Exif.Image.CameraSerialNumber")(
        "Exif.Image.LensInfo")("Exif.Image.ExposureTime")("Exif.Image.FNumber")(
        "Exif.Image.ExposureProgram")
    // Exif.Photo
    ("Exif.Photo.SensitivityType")("Exif.Photo.ISOSpeed")(
        "Exif.Photo.ISOSpeedRatings")("Exif.Photo.ISOSpeedLatitudeyyy")(
        "Exif.Photo.ISOSpeedLatitudezzz")("Exif.Photo.FileSource")(
        "Exif.Photo.SceneType")("Exif.Photo.SceneCaptureType")(
        "Exif.Photo.UserComment")("Exif.Image.SensingMethod")(
        "Exif.Photo.ShutterSpeedValue")("Exif.Photo.ApertureValue")(
        "Exif.Photo.BrightnessValue")("Exif.Photo.ExposureBiasValue")(
        "Exif.Photo.MaxApertureValue")("Exif.Photo.SubjectDistance")(
        "Exif.Photo.MeteringMode")("Exif.Photo.LightSource")(
        "Exif.Photo.Flash")("Exif.Photo.FocalLength")("Exif.Photo.SubjectArea")(
        "Exif.Photo.UserComment")("Exif.Photo.ExposureMode")(
        "Exif.Photo.DigitalZoomRatio")("Exif.Photo.FocalLengthIn35mmFilm")(
        "Exif.Photo.BodySerialNumber")("Exif.Photo.LensSpecification")(
        "Exif.Photo.LensMake")("Exif.Photo.LensModel")(
        "Exif.Photo.LensSerialNumber")("Exif.Photo.DateTimeOriginal")(
        "Exif.Photo.DateTimeDigitized")
    // Same tags as Exif.Image
    ("Exif.Photo.ExposureTime")("Exif.Photo.FNumber")(
        "Exif.Photo.ExposureProgram");

void copyExifData(const std::string &from, const std::string &to,
                  bool dontOverwrite, const std::string &comment,
                  bool destIsLDR, bool keepRotation) {
#ifndef NDEBUG
    std::clog << "Processing EXIF from " << from << " to " << to << std::endl;
#endif

    try {
        Exiv2::Image::AutoPtr sourceImage;
        Exiv2::ExifData srcExifData;

        if (!from.empty()) {
            // get source exif data
            sourceImage = Exiv2::ImageFactory::open(from);

            sourceImage->readMetadata();
            srcExifData = sourceImage->exifData();

            if (srcExifData.empty()) {
#ifndef NDEBUG
                std::clog << "No exif data found in the image: " << from
                          << std::endl;
#endif
                return;
            }
        }

        // get destination exif data
        Exiv2::Image::AutoPtr destinationImage = Exiv2::ImageFactory::open(to);

        if (dontOverwrite) {
            // doesn't throw anything if it is empty
            destinationImage->readMetadata();
            // doesn't throw anything if it is empty
            Exiv2::ExifData &destExifData = destinationImage->exifData();

            if (!from.empty()) {
                // for all the tags in the source exif data
                for (Exiv2::ExifData::const_iterator it = srcExifData.begin(),
                                                     itEnd = srcExifData.end();
                     it != itEnd; ++it) {
                    if (ValidExifDictionary()(it->groupName(), it->tagName(),
                                              it->key())) {
                        Exiv2::ExifData::iterator outIt =
                            destExifData.findKey(Exiv2::ExifKey(it->key()));
                        if (outIt == destExifData.end()) {
                            // we create a new tag in the destination file, the
                            // tag has the
                            // key of the source
                            Exiv2::Exifdatum &destTag = destExifData[it->key()];
                            // now the tag has also the value of the source
                            destTag.setValue(&(it->value()));
                        }
                    }
#ifndef NDEBUG
                    else {
                        std::clog << "Found invalid key: " << it->key() << "\n";
                    }
#endif
                }
            }

            // rotation support
            if (!keepRotation) {
                destExifData["Exif.Image.Orientation"] = 1;
            }
        } else if (destIsLDR) {
            // copy all valid tag
            Exiv2::ExifData destExifData;
            if (!from.empty()) {
                for (Exiv2::ExifData::const_iterator it = srcExifData.begin(),
                                                     itEnd = srcExifData.end();
                     it != itEnd; ++it) {
                    // Note: the algorithm is similar to the C++11 std::copy_if(
                    // )
                    if (ValidExifDictionary()(it->groupName(), it->tagName(),
                                              it->key())) {
                        // overwrite tag if found!
                        destExifData[it->key()] = it->value();
                    }
#ifndef NDEBUG
                    else {
                        std::clog << "Found invalid key: " << it->key() << "\n";
                    }
#endif
                }
            }

            // if comment is not empty, overwrite/set comment values
            if (!comment.empty()) {
#ifndef NDEBUG
                std::clog << "EXIF comments: " << comment << "\n";
#endif
                destExifData["Exif.Image.ProcessingSoftware"] =
                    "Luminance HDR " + std::string(LUMINANCEVERSION);
                destExifData["Exif.Image.Software"] =
                    "Luminance HDR " + std::string(LUMINANCEVERSION);
                destExifData["Exif.Image.ImageDescription"] = comment;
                destExifData["Exif.Photo.UserComment"] =
                    "charset=\"Ascii\" " + comment;
            }

            // rotation support
            if (!keepRotation) {
                destExifData["Exif.Image.Orientation"] = 1;
            }

            destinationImage->setExifData(destExifData);
        } else {
            destinationImage->setExifData(srcExifData);
        }
        destinationImage->writeMetadata();
    } catch (Exiv2::AnyError &e) {
#ifndef NDEBUG
        qDebug() << e.what();
#endif
    }
}

/*
 * This function obtains the "average scene luminance" (cd/m^2) from an image
 * file.
 * "average scene luminance" is the L (aka B) value mentioned in [1]
 * You have to take a log2f of the returned value to get an EV value.
 *
 * We are using K=12.07488f and the exif-implied value of N=1/3.125 (see [1]).
 * K=12.07488f is the 1.0592f * 11.4f value in pfscalibration's
 * pfshdrcalibrate.cpp file.
 * Based on [3] we can say that the value can also be 12.5 or even 14.
 * Another reference for APEX is [4] where N is 0.3, closer to the APEX
 * specification of 2^(-7/4)=0.2973.
 *
 * [1] http://en.wikipedia.org/wiki/APEX_system
 * [2] http://en.wikipedia.org/wiki/Exposure_value
 * [3] http://en.wikipedia.org/wiki/Light_meter
 * [4] http://doug.kerr.home.att.net/pumpkin/#APEX
 *
 * This function tries first to obtain the shutter speed from either of
 * two exif tags (there is no standard between camera manifacturers):
 * ExposureTime or ShutterSpeedValue.
 * Same thing for f-number: it can be found in FNumber or in ApertureValue.
 *
 * F-number and shutter speed are mandatory in exif data for EV calculation, iso
 * is not.
 */
/*
float obtain_avg_lum(const std::string& filename)
{
    try
    {
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename);
        image->readMetadata();
        Exiv2::ExifData &exifData = image->exifData();
        if (exifData.empty())
            return -1;

        Exiv2::ExifData::const_iterator iexpo =
exifData.findKey(Exiv2::ExifKey("Exif.Photo.ExposureTime"));
        Exiv2::ExifData::const_iterator iexpo2 =
exifData.findKey(Exiv2::ExifKey("Exif.Photo.ShutterSpeedValue"));
        Exiv2::ExifData::const_iterator iiso  =
exifData.findKey(Exiv2::ExifKey("Exif.Photo.ISOSpeedRatings"));
        Exiv2::ExifData::const_iterator ifnum =
exifData.findKey(Exiv2::ExifKey("Exif.Photo.FNumber"));
        Exiv2::ExifData::const_iterator ifnum2 =
exifData.findKey(Exiv2::ExifKey("Exif.Photo.ApertureValue"));

        // default not valid values
        float expo  = -1;
        float iso   = -1;
        float fnum  = -1;

        if (iexpo != exifData.end())
        {
            expo = iexpo->toFloat();
        }
        else if (iexpo2 != exifData.end())
        {
            long num = 1;
            long div = 1;
            double tmp = std::exp(std::log(2.0) * iexpo2->toFloat());
            if (tmp > 1)
            {
                div = static_cast<long>(tmp + 0.5);
            }
            else
            {
                num = static_cast<long>(1/tmp + 0.5);
            }
            expo = static_cast<float>(num)/static_cast<float>(div);
        }

        if (ifnum != exifData.end())
        {
            fnum = ifnum->toFloat();
        }
        else if (ifnum2 != exifData.end())
        {
            fnum = static_cast<float>(std::exp(std::log(2.0) * ifnum2->toFloat()
/ 2));
        }
        // some cameras/lens DO print the fnum but with value 0, and this is not
allowed for ev computation purposes.
        if (fnum == 0)
            return -1;

        // if iso is found use that value, otherwise assume a value of iso = 100
        // (again, some cameras do not print iso in exif).
        if (iiso == exifData.end())
        {
            iso = 100.0;
        }
        else
        {
            iso = iiso->toFloat();
        }

        //At this point the three variables have to be != -1
        if (expo!=-1 && iso!=-1 && fnum!=-1)
        {
            //      std::cerr << "expo=" << expo << " fnum=" << fnum << " iso="
<< iso << " |returned=" << (expo * iso) / (fnum*fnum*12.07488f) << std::endl;
            return ( (expo * iso) / (fnum*fnum*12.07488f) );
        }
        else
        {
            return -1;
        }
    }
    catch (Exiv2::AnyError& e)
    {
        return -1;
    }
}
*/

float getExposureTime(const std::string &filename) {
    try {
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename);
        image->readMetadata();
        Exiv2::ExifData &exifData = image->exifData();
        if (exifData.empty()) return -1;

        Exiv2::ExifData::const_iterator iexpo =
            exifData.findKey(Exiv2::ExifKey("Exif.Photo.ExposureTime"));
        Exiv2::ExifData::const_iterator iexpo2 =
            exifData.findKey(Exiv2::ExifKey("Exif.Photo.ShutterSpeedValue"));
        // Exiv2::ExifData::const_iterator iiso  =
        // exifData.findKey(Exiv2::ExifKey("Exif.Photo.ISOSpeedRatings"));
        // Exiv2::ExifData::const_iterator ifnum =
        // exifData.findKey(Exiv2::ExifKey("Exif.Photo.FNumber"));
        // Exiv2::ExifData::const_iterator ifnum2 =
        // exifData.findKey(Exiv2::ExifKey("Exif.Photo.ApertureValue"));

        float expo = -1;

        if (iexpo != exifData.end()) {
            expo = iexpo->toFloat();
        } else if (iexpo2 != exifData.end()) {
            long num = 1;
            long div = 1;
            double tmp = std::exp(std::log(2.0) * iexpo2->toFloat());
            if (tmp > 1) {
                div = static_cast<long>(tmp + 0.5);
            } else {
                num = static_cast<long>(1 / tmp + 0.5);
            }
            expo = static_cast<float>(num) / static_cast<float>(div);
        }
        if (expo != -1) {
            return expo;
        } else {
            return -1;
        }
    } catch (Exiv2::AnyError &e) {
        return -1;
    }
}

float getAverageLuminance(const std::string &filename) {
    try {
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename);
        image->readMetadata();
        Exiv2::ExifData &exifData = image->exifData();

        // Exif.Image.ExposureBiasValue
        Exiv2::ExifData::const_iterator itExpValue =
            exifData.findKey(Exiv2::ExifKey("Exif.Image.ExposureBiasValue"));
        if (itExpValue != exifData.end()) {
            return pow(2.0f, itExpValue->toFloat());
        }

        // Exif.Photo.ExposureBiasValue
        itExpValue =
            exifData.findKey(Exiv2::ExifKey("Exif.Photo.ExposureBiasValue"));
        if (itExpValue != exifData.end()) {
            return pow(2.0f, itExpValue->toFloat());
        }

        std::clog << "Cannot find ExposureBiasValue for " << filename
                  << std::endl;

        return -1.0;
    } catch (Exiv2::AnyError &e) {
        return -1.0;
    }
}

}  // ExifOperations
