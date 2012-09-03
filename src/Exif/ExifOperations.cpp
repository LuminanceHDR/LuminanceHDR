/**
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2007 Giuseppe Rota
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */

#include <image.hpp>
#include <cmath>
#include <iostream>

#include "ExifOperations.h"
#include "arch/math.h"

namespace ExifOperations
{
  
    void writeExifData(const std::string& filename, const std::string& comment, float expotime)
    {
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename);
        image->readMetadata();
        Exiv2::ExifData &exifData = image->exifData();
        exifData["Exif.Image.Software"]="Created with opensource tool Luminance HDR, http://qtpfsgui.sourceforge.net";
        exifData["Exif.Image.ImageDescription"]=comment;
        exifData["Exif.Photo.UserComment"]=(QString("charset=\"Ascii\" ") + QString::fromStdString(comment)).toStdString();
        if (expotime != 100.0f) {
            const Exiv2::ValueType<float> v(expotime*12.07488f/100);    
            const Exiv2::ValueType<Exiv2::Rational> r(v.toRational());
            exifData["Exif.Photo.ExposureTime"] = r;
            const Exiv2::ValueType<float> f(1.0);   
            const Exiv2::ValueType<Exiv2::Rational> fr(f.toRational());
            exifData["Exif.Photo.FNumber"] = fr;
        }
        image->setExifData(exifData);
        image->writeMetadata();
    }
  
    void copyExifData(const std::string& from, const std::string& to, const std::string& comment, bool dont_overwrite, bool destIsLDR)
    {
        std::cerr << "processing file: " << from.c_str() << " and " << to.c_str() << std::endl;
        //get source and destination exif data
        //THROWS, if opening the file fails or it contains data of an unknown image type.
        Exiv2::Image::AutoPtr sourceimage = Exiv2::ImageFactory::open(from);
        Exiv2::Image::AutoPtr destimage = Exiv2::ImageFactory::open(to);
        //Callers must check the size of individual metadata types before accessing the data.
        //readMetadata THROWS an exception if opening or reading of the file fails or the image data is not valid (does not look like data of the specific image type).
        sourceimage->readMetadata();
        Exiv2::ExifData &srcExifData = sourceimage->exifData();
        if (srcExifData.empty())
            throw Exiv2::Error(1, "No exif data found in the image");
        if (dont_overwrite) {
            //doesn't throw anything if it is empty
            destimage->readMetadata();
            //doesn't throw anything if it is empty
            Exiv2::ExifData &dest_exifData = destimage->exifData();
            //end delimiter for this source image data
            Exiv2::ExifData::const_iterator end_src = srcExifData.end();
            //for all the tags in the source exif data
            for (Exiv2::ExifData::const_iterator i = srcExifData.begin(); i != end_src; ++i) {
                //check if current source key exists in destination file
                Exiv2::ExifData::iterator maybe_exists = dest_exifData.findKey( Exiv2::ExifKey(i->key()) );
                //if exists AND we are told not to overwrite
                if (maybe_exists != dest_exifData.end())
                    continue;
                else {
                    //here we copy the value
                    //we create a new tag in the destination file, the tag has the key of the source
                    Exiv2::Exifdatum& dest_tag = dest_exifData[i->key()];
                    //now the tag has also the value of the source
                    dest_tag.setValue(&(i->value()));               }
                }
        }
        else {
            if (destIsLDR) {
                //copy all tags from source except exposure time and aperture
                Exiv2::ExifData destExifData;
                if (comment != "") {
                    destExifData["Exif.Image.Software"]="Created with opensource tool Luminance HDR, http://qtpfsgui.sourceforge.net";
                    destExifData["Exif.Image.ImageDescription"]=comment;
                    destExifData["Exif.Photo.UserComment"]=(QString("charset=\"Ascii\" ") + QString::fromStdString(comment)).toStdString();
                }
                Exiv2::ExifData::const_iterator end_src = srcExifData.end();
    
                //for all the tags in the source exif data
                for (Exiv2::ExifData::const_iterator i = srcExifData.begin(); i != end_src; ++i) {
                    Exiv2::Exifdatum& sourceDatum = srcExifData[i->key()];
                    if (sourceDatum.key() == "Exif.Photo.FNumber")
                        continue;
                    if (sourceDatum.key() == "Exif.Photo.ExposureTime")
                        continue;
                    if (comment != "" && sourceDatum.key() == "Exif.Image.Software")
                        continue;
                    if (comment != "" && sourceDatum.key() == "Exif.Image.ImageDescription")
                        continue;
                    if (comment != "" && sourceDatum.key() == "Exif.Photo.UserComment")
                        continue;
                    const Exiv2::ExifKey destKey(i->key());
                    destExifData.add(destKey, &(i->value()));
                }
                destimage->setExifData(destExifData);
            }
            else
                destimage->setExifData(srcExifData);
        }
    
        //THROWS Exiv2::Error if the operation fails
        destimage->writeMetadata();
    }
  
  /**
   * This function obtains the "average scene luminance" (cd/m^2) from an image file.
   * "average scene luminance" is the L (aka B) value mentioned in [1]
   * You have to take a log2f of the returned value to get an EV value.
   * 
   * We are using K=12.07488f and the exif-implied value of N=1/3.125 (see [1]).
   * K=12.07488f is the 1.0592f * 11.4f value in pfscalibration's pfshdrcalibrate.cpp file.
   * Based on [3] we can say that the value can also be 12.5 or even 14.
   * Another reference for APEX is [4] where N is 0.3, closer to the APEX specification of 2^(-7/4)=0.2973.
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
   * F-number and shutter speed are mandatory in exif data for EV calculation, iso is not.
   */
  float obtain_avg_lum(const std::string& filename)
  {
    try
    {
      Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename);
      image->readMetadata();
      Exiv2::ExifData &exifData = image->exifData();
      if (exifData.empty())
        return -1;
      
      Exiv2::ExifData::const_iterator iexpo = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ExposureTime"));
      Exiv2::ExifData::const_iterator iexpo2 = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ShutterSpeedValue"));
      Exiv2::ExifData::const_iterator iiso  = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ISOSpeedRatings"));
      Exiv2::ExifData::const_iterator ifnum = exifData.findKey(Exiv2::ExifKey("Exif.Photo.FNumber"));
      Exiv2::ExifData::const_iterator ifnum2 = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ApertureValue"));
      
      // default not valid values
      float expo  = -1;
      float iso   = -1;
      float fnum  = -1;
      
      if (iexpo != exifData.end())
      {
        expo=iexpo->toFloat();
      }
      else if (iexpo2 != exifData.end())
      {
        long num=1, div=1;
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
        fnum = static_cast<float>(std::exp(std::log(2.0) * ifnum2->toFloat() / 2));
      }
      // some cameras/lens DO print the fnum but with value 0, and this is not allowed for ev computation purposes.
      if (fnum == 0)
        return -1;
      
      //if iso is found use that value, otherwise assume a value of iso=100. (again, some cameras do not print iso in exif).
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
        //      std::cerr << "expo=" << expo << " fnum=" << fnum << " iso=" << iso << " |returned=" << (expo * iso) / (fnum*fnum*12.07488f) << std::endl;
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
  
  int obtain_rotation(const std::string& filename)
  {
    try
    {
      Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename);
      image->readMetadata();
      Exiv2::ExifData &exifData = image->exifData();
      if (exifData.empty())
        return 0;
      
      Exiv2::ExifData::const_iterator degrees = exifData.findKey(Exiv2::ExifKey("Exif.Image.Orientation"));
      
      if (degrees != exifData.end())
      {

        /*
            http://jpegclub.org/exif_orientation.html


        Value   0th Row     0th Column
            1   top         left side
            2   top         right side
            3   bottom      right side
            4   bottom      left side
            5   left side   top
            6   right side  top
            7   right side  bottom
            8   left side   bottom

        */

        float rotation = degrees->toFloat();     

        switch((int)rotation)
        {
        case 3:
            return 180;
        case 6:
            return 90;
        case 8:
            return 270;
        }
      }
      return 0;
    }
    catch (Exiv2::AnyError& e)
    {
      return 0;
    }
  }
}
