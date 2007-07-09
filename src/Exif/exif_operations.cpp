/**
 * This file is a part of Qtpfsgui package.
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
#include "exif_operations.h"

Exiv2::ExifKey ExifOperations::fnum ("Exif.Photo.FNumber");
Exiv2::ExifKey ExifOperations::fnum2 ("Exif.Photo.ApertureValue");
Exiv2::ExifKey ExifOperations::iso ("Exif.Photo.ISOSpeedRatings");
Exiv2::ExifKey ExifOperations::expotime ("Exif.Photo.ExposureTime");
Exiv2::ExifKey ExifOperations::expotime2 ("Exif.Photo.ShutterSpeedValue");

void ExifOperations::writeExifData(const std::string& filename, const std::string& comment) {
	Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename);
	image->readMetadata();
	Exiv2::ExifData &exifData = image->exifData();
	exifData["Exif.Image.Software"]="Created with opensource tool Qtpfsgui, http://qtpfsgui.sourceforge.net";
	exifData["Exif.Image.ImageDescription"]=comment;
	exifData["Exif.Photo.UserComment"]=(QString("charset=\"Ascii\" ") + QString::fromStdString(comment)).toStdString();
	image->setExifData(exifData);
	image->writeMetadata();
}

void ExifOperations::copyExifData(const std::string& from, const std::string& to, bool dont_overwrite) {
		std::cerr << "processing file: " << from.c_str() << " and " << to.c_str() << std::endl;
		//get source and destination exif data
		Exiv2::Image::AutoPtr sourceimage = Exiv2::ImageFactory::open(from);
		Exiv2::Image::AutoPtr destimage = Exiv2::ImageFactory::open(to);
		sourceimage->readMetadata();
		//FIXME this one below can throw an exception: low priority
		destimage->readMetadata(); //doesn't matter if it is empty
		Exiv2::ExifData &src_exifData = sourceimage->exifData();
		Exiv2::ExifData &dest_exifData = destimage->exifData(); //doesn't matter if it is empty
		Exiv2::ExifData::const_iterator end_src = src_exifData.end(); //end delimiter for this source image data
		//for all the tags in the source exif data
		for (Exiv2::ExifData::const_iterator i = src_exifData.begin(); i != end_src; ++i) {
			//check if current source key exists in destination file
			Exiv2::ExifData::iterator maybe_exists = dest_exifData.findKey( Exiv2::ExifKey(i->key()) );
// 			//if exists AND we are told not to overwrite
			if (maybe_exists != dest_exifData.end() && dont_overwrite) {
				continue;
			} else {
				//here we copy the value
				//we create a new tag in the destination file, the tag has the key of the source
				Exiv2::Exifdatum& dest_tag = dest_exifData[i->key()];
				//now the tag has also the value of the source
				dest_tag.setValue(&(i->value()));
			}
		}
// 		try {
		destimage->writeMetadata();
// 		} catch (Exiv2::Error &e) { //TODO on error continue and provide log
// 			std::cerr << e.what();
// 		}
}

float ExifOperations::obtain_expotime(const std::string& filename) {
try {
	Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename);
	image->readMetadata();
	Exiv2::ExifData &exifData = image->exifData();
	if (exifData.empty())
		return -1;

	Exiv2::ExifData::const_iterator iexpo = exifData.findKey(expotime);
	Exiv2::ExifData::const_iterator iexpo2 = exifData.findKey(expotime2);
	Exiv2::ExifData::const_iterator iiso  = exifData.findKey(iso);
	Exiv2::ExifData::const_iterator ifnum = exifData.findKey(fnum);
	Exiv2::ExifData::const_iterator ifnum2 = exifData.findKey(fnum2);
	float expo=-1; float iso=-1; float fnum=-1;

	if (iexpo != exifData.end()) {
		expo=iexpo->toFloat();
	} else if (iexpo2 != exifData.end()) {
		long num=1, div=1;
		double tmp = std::exp(std::log(2.0) * iexpo2->toFloat());
		if (tmp > 1) {
			div = static_cast<long>(tmp + 0.5);
		}
		else {
			num = static_cast<long>(1/tmp + 0.5);
		}
		expo=static_cast<float>(num)/static_cast<float>(div);
	}

	if (ifnum != exifData.end()) {
		fnum=ifnum->toFloat();
	} else if (ifnum2 != exifData.end()) {
		fnum=static_cast<float>(std::exp(std::log(2.0) * ifnum2->toFloat() / 2));
	}
	if (fnum==0)
		return -1;

	if (iiso == exifData.end()) {
		iso=100.0;
	} else {
		iso=iiso->toFloat();
	}

	if (expo!=-1 && iso!=-1 && fnum!=-1) {
// 		std::cerr << "expo=" << expo << " fnum=" << fnum << " iso=" << iso << " |returned=" << (expo * iso) / (fnum*fnum*12.07488f) << std::endl;
		return ( (expo * iso) / (fnum*fnum*12.07488f) ); //this is PFM :)
	} else {
		return -1;
	}
} catch (Exiv2::AnyError& e) {
	return -1;
}
}

