/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2010 Franco Comida
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
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <vector>
#include <cmath>

#include <QString>
#include <QFileInfo>
#include <QDebug>
#include <QFile>

#include "Libpfs/domio.h"
#include "Fileformat/pfsinraw.h"
#include "Libpfs/frame.h"
#include "Common/LuminanceOptions.h"

/**************************** From UFRAW sourcecode ********************************
 *
 * Convert between Temperature and RGB.
 * Base on information from http://www.brucelindbloom.com/
 * The fit for D-illuminant between 4000K and 15000K are from CIE
 * The generalization to 2000K < T < 4000K and the blackbody fits
 * are my own and should be taken with a grain of salt.
 */
static const double XYZ_to_RGB[3][3] = {
    { 3.24071,	-0.969258,  0.0556352 },
    {-1.53726,	1.87599,    -0.203996 },
    {-0.498571,	0.0415557,  1.05707 } };

void Temperature_to_RGB(double T, double RGB[3])
{
    int c;
    double xD, yD, X, Y, Z, max;
    // Fit for CIE Daylight illuminant
    if (T<= 4000) {
	xD = 0.27475e9/(T*T*T) - 0.98598e6/(T*T) + 1.17444e3/T + 0.145986;
    } else if (T<= 7000) {
	xD = -4.6070e9/(T*T*T) + 2.9678e6/(T*T) + 0.09911e3/T + 0.244063;
    } else {
	xD = -2.0064e9/(T*T*T) + 1.9018e6/(T*T) + 0.24748e3/T + 0.237040;
    }
    yD = -3*xD*xD + 2.87*xD - 0.275;

    // Fit for Blackbody using CIE standard observer function at 2 degrees
    //xD = -1.8596e9/(T*T*T) + 1.37686e6/(T*T) + 0.360496e3/T + 0.232632;
    //yD = -2.6046*xD*xD + 2.6106*xD - 0.239156;

    // Fit for Blackbody using CIE standard observer function at 10 degrees
    //xD = -1.98883e9/(T*T*T) + 1.45155e6/(T*T) + 0.364774e3/T + 0.231136;
    //yD = -2.35563*xD*xD + 2.39688*xD - 0.196035;

    X = xD/yD;
    Y = 1;
    Z = (1-xD-yD)/yD;
    max = 0;
    for (c=0; c<3; c++)
    {
        RGB[c] = X*XYZ_to_RGB[0][c] + Y*XYZ_to_RGB[1][c] + Z*XYZ_to_RGB[2][c];
        if (RGB[c]>max) max = RGB[c];
    }
    for (c=0; c<3; c++)
    {
        RGB[c] = RGB[c]/max;
    }
}
/*********** END UFRAW CODE *****************************************************/


#define P1 RawProcessor.imgdata.idata
#define S RawProcessor.imgdata.sizes
#define C RawProcessor.imgdata.color
#define T RawProcessor.imgdata.thumbnail
#define P2 RawProcessor.imgdata.other
#define OUT RawProcessor.imgdata.params

pfs::Frame* readRawIntoPfsFrame(const char *filename, const char *tempdir, LuminanceOptions *options, bool writeOnDisk, progress_callback cb, void *callback_data)
{
  LibRaw RawProcessor;
  RawProcessor.set_progress_handler(cb, callback_data);
  int ret;

  OUT.output_bps = 16;

  //OUT.gamm[0] = 1/2.4;   //sRGB
  //OUT.gamm[1] = 12.92;   //sRGB
  OUT.gamm[0] = OUT.gamm[1] = 1; 
 
  if ( options->isRawFourColorRGB() )
    OUT.four_color_rgb = 1;

  if ( options->isRawDoNotUseFujiRotate() )
    OUT.use_fuji_rotate = 0; // do not rotate or strech pixels on fuji cameras - default = 1 (rotate)
 
  OUT.user_qual = options->getRawUserQuality();
  OUT.med_passes = options->getRawMedPasses();

  if (options->getRawWhiteBalanceMethod() == 1) // Camera WB
    OUT.use_camera_wb = 1;
  else if (options->getRawWhiteBalanceMethod() == 2) // Auto WB
    OUT.use_auto_wb = 1;
  else { // Manual WB
    double Temp = options->getRawTemperatureKelvin();
    double RGB[3];

    Temperature_to_RGB(Temp, RGB);

    RGB[1] = RGB[1] / options->getRawGreen();

   if ((ret = RawProcessor.open_file(filename)) != LIBRAW_SUCCESS)
   {
      qDebug() << "Error Opening RAW File";
      RawProcessor.recycle();
      throw QObject::tr("Error Opening RAW File");
   }
    
    bool identify = true;
    if ((ret = RawProcessor.adjust_sizes_info_only()) != LIBRAW_SUCCESS)
    {
      identify = false;
    }

    int numcolors = P1.colors;
    std::vector<double> daylightMult(numcolors);

    for(int c = 0 ; c < numcolors ; c++)
      daylightMult[c] = C.pre_mul[c];

    if (identify) { //TODO
      RGB[0] = daylightMult[0] / RGB[0];
      RGB[1] = daylightMult[1] / RGB[1];
      RGB[2] = daylightMult[2] / RGB[2];
    }
    else {
      RGB[0] = 1.0 / RGB[0];
      RGB[1] = 1.0 / RGB[1];
      RGB[2] = 1.0 / RGB[2];
    }

    RawProcessor.recycle();

    OUT.user_mul[0] = RGB[0];
    OUT.user_mul[1] = RGB[1];
    OUT.user_mul[2] = RGB[2];
    OUT.user_mul[3] = RGB[1];
  }

  if (options->getRawHighlightsMode() < 3)
  {
      OUT.highlight = options->getRawHighlightsMode();
  }
  else
  {
      OUT.highlight = options->getRawHighlightsMode() + options->getRawLevel();
  }
  OUT.no_auto_bright = !options->isRawAutoBrightness();
  OUT.bright = options->getRawBrightness();

  if ( options->isRawUseBlack() )
  {
      OUT.user_black = options->getRawUserBlack();
  }
  if ( options->isRawUseSaturation() )
  {
      OUT.user_sat = options->getRawUserSaturation();
  }

  if ( options->isRawUseNoiseReduction() )
  {
      OUT.threshold = options->getRawNoiseReductionThreshold();
  }
  if ( options->isRawUseChroma() )
  {
      OUT.aber[0] = options->getRawAber0();
      OUT.aber[2] = options->getRawAber2();
  }

  QString fname;
  QByteArray ba;
  
  if (options->getCameraProfile() == 1)
  {
      OUT.camera_profile = (char*)"embed";
  }
  else if (options->getCameraProfile() == 2)
  {
      fname = options->getCameraProfileFileName();
      ba = QFile::encodeName( fname );
      OUT.camera_profile = ba.data();
      qDebug() << "Camera profile: " << fname;
  }

  OUT.output_color = 1; // sRGB

  if ((ret = RawProcessor.open_file(filename)) != LIBRAW_SUCCESS)
  {
    qDebug() << "Error Opening RAW File";
    RawProcessor.recycle();
    throw QObject::tr("Error Opening RAW File");
  }

  if ((ret = RawProcessor.unpack()) != LIBRAW_SUCCESS)
  {
    qDebug() << "Error Unpacking RAW File";
    RawProcessor.recycle();
    throw QObject::tr("Error Unpacking RAW File");
  }

  if ((ret = RawProcessor.dcraw_process()) != LIBRAW_SUCCESS)
  {
    qDebug() << "Error Processing RAW File";
    RawProcessor.recycle();
    throw QObject::tr("Error Processing RAW File");
  }

  qDebug() << "Width: " << S.width << " Height: " << S.height;
  qDebug() << "iWidth: " << S.iwidth << " iHeight: " << S.iheight;
  qDebug() << "Make: " << P1.make;
  qDebug() << "Model: " << P1.model;
  qDebug() << "ISO: " << P2.iso_speed;
  qDebug() << "Shutter: " << P2.shutter;
  qDebug() << "Aperture: " << P2.aperture;
  qDebug() << "Focal Length: " << P2.focal_len;

  libraw_processed_image_t *image = RawProcessor.dcraw_make_mem_image(&ret);

  if (image == NULL)
  {
    qDebug() << "Memory Error in processing RAW File";
    RawProcessor.recycle();
    throw QObject::tr("Memory Error in processing RAW File");
  }

  int W = image->width;
  int H = image->height;
  
  pfs::DOMIO pfsio;
  pfs::Frame *frame = pfsio.createFrame( W, H );

  if (frame == NULL)
  {
    RawProcessor.recycle();
    throw QObject::tr("Error Creating PFS Frame");
  }

  pfs::Channel *Xc, *Yc, *Zc;
  frame->createXYZChannels( Xc, Yc, Zc );

  float* r =  Xc->getChannelData()->getRawData();
  float* g =  Yc->getChannelData()->getRawData();
  float* b =  Zc->getChannelData()->getRawData();

  const unsigned char* raw_data = image->data;
  for (int idx = 0; idx < H*W; ++idx)
  {
      *r++ = raw_data[0] + (raw_data[1] << 8); raw_data += 2;
      *g++ = raw_data[0] + (raw_data[1] << 8); raw_data += 2;
      *b++ = raw_data[0] + (raw_data[1] << 8); raw_data += 2;
  }

//  pfs::Array2D* X = Xc->getChannelData();
//  pfs::Array2D* Y = Yc->getChannelData();
//  pfs::Array2D* Z = Zc->getChannelData();
//  int d = 0;
//  for (int j = 0; j < H; j++)
//  {
//      for (int i = 0; i < W; i++)
//      {
//          (*X)(i,j) = image->data[d]   + 256.0*image->data[d+1];
//          (*Y)(i,j) = image->data[d+2] + 256.0*image->data[d+3];
//          (*Z)(i,j) = image->data[d+4] + 256.0*image->data[d+5];
//          d += 6;
//      }
//  }

  qDebug() << "Data size: " << image->data_size << " " << W*H*3*2;
  qDebug() << "W: " << W << " H: " << H;

  if (writeOnDisk)   // for align_image_stack and thumbnails
  {
    QString fname = QFile::decodeName(filename);
    QString tmpdir = QFile::decodeName(tempdir);
    QFileInfo qfi(fname);
    QString outname = tmpdir + "/" + qfi.baseName() + ".tiff";

    RawProcessor.dcraw_ppm_tiff_writer(QFile::encodeName(outname));

    if( (ret = RawProcessor.unpack_thumb() ) == LIBRAW_SUCCESS)
    {
      QString suffix = T.tformat == LIBRAW_THUMBNAIL_JPEG ? "thumb.jpg" : "thumb.ppm"; 
      QString thumbname = tmpdir + "/" + qfi.baseName() + "." + suffix;
      RawProcessor.dcraw_thumb_writer(QFile::encodeName(thumbname));
    }
    qDebug() << "Filename: " << filename;
    qDebug() << "Outname: " << qPrintable(outname);
  }
  
  LibRaw::dcraw_clear_mem(image);
  RawProcessor.recycle();

  return frame;
}
