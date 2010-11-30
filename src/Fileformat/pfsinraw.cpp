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

#include <cmath>
#include <libraw/libraw.h>
#include <QString>
#include <QFileInfo>
#include <iostream>

#include "Libpfs/pfs.h"
#include "pfsinraw.h"

#define P1 RawProcessor.imgdata.idata
#define S RawProcessor.imgdata.sizes
#define C RawProcessor.imgdata.color
#define T RawProcessor.imgdata.thumbnail
#define P2 RawProcessor.imgdata.other
#define OUT RawProcessor.imgdata.params

pfs::Frame* readRawIntoPfsFrame(const char *filename, const char *tempdir, LuminanceOptions *options, bool writeOnDisk)
{  
  LibRaw RawProcessor;
  int ret;

  OUT.filtering_mode = LIBRAW_FILTERING_AUTOMATIC;
  OUT.output_bps = 16;

  OUT.gamm[0] = 1/2.4;   //sRGB
  OUT.gamm[1] = 12.92;   //sRGB
 
  if( options->four_color_rgb)
    OUT.four_color_rgb = 1;

  if( options->do_not_use_fuji_rotate)
    OUT.use_fuji_rotate = 0; // do not rotate or strech pixels on fuji cameras - default = 1 (rotate)
 
  OUT.user_qual = options->user_qual; 
  OUT.med_passes = options->med_passes; 

  if(options->wb_method == 0) // Camera WB
    OUT.use_camera_wb = 1;
  else if(options->wb_method == 1) // Auto WB
    OUT.use_auto_wb = 1;
  else { // Manual WB
    OUT.user_mul[0] = options->user_mul_0;
    OUT.user_mul[1] = options->user_mul_1;
    OUT.user_mul[2] = options->user_mul_2;
    OUT.user_mul[3] = options->user_mul_3;
  }

  if(options->highlights < 3)
    OUT.highlight = options->highlights;
  else
    OUT.highlight = options->highlights + options->level; 
  
  OUT.no_auto_bright = !options->auto_bright;
  OUT.bright = options->brightness;

  if (options->use_black)
    OUT.user_black = options->user_black;

  if (options->use_sat)
    OUT.user_sat = options->user_sat;

  if (options->use_noise)
    OUT.threshold = options->threshold;

  if (options->use_chroma) {
    OUT.aber[0] = options->aber_0;
    OUT.aber[2] = options->aber_2;
  }

  if( (ret = RawProcessor.open_file(filename)) != LIBRAW_SUCCESS) {
    std::cout << "Error Opening RAW File" << std::endl;
    return NULL;
  }

  if( (ret = RawProcessor.unpack()) != LIBRAW_SUCCESS) {
    std::cout << "Error Unpacking RAW File" << std::endl;
    return NULL;
  }

  if( (ret = RawProcessor.dcraw_process()) != LIBRAW_SUCCESS) {
    std::cout << "Error Processing RAW File" << std::endl;
    return NULL;
  }

  std::cout << "Width: " << S.width << " Height: " << S.height << std::endl;
  std::cout << "iWidth: " << S.iwidth << " iHeight: " << S.iheight << std::endl;
  std::cout << "Make: " << P1.make << std::endl;
  std::cout << "Model: " << P1.model << std::endl;
  std::cout << "ISO: " << P2.iso_speed << std::endl;
  std::cout << "Shutter: " << P2.shutter << std::endl;
  std::cout << "Aperture: " << P2.aperture << std::endl;
  std::cout << "Focal Length: " << P2.focal_len << std::endl;

  libraw_processed_image_t *image = RawProcessor.dcraw_make_mem_image(&ret);

  if( image == NULL) {
    std::cout << "Memory Error RAW File" << std::endl;
    return NULL;
  }

  int W = image->width;
  int H = image->height;
  
  pfs::DOMIO pfsio;
  pfs::Frame *frame = pfsio.createFrame( W, H );

  if( frame == NULL) {
    return NULL;
  }

  pfs::Channel *Xc, *Yc, *Zc;
  frame->createXYZChannels( Xc, Yc, Zc );

  pfs::Array2D* X = Xc->getChannelData();
  pfs::Array2D* Y = Yc->getChannelData();
  pfs::Array2D* Z = Zc->getChannelData();
  
  int d = 0;

  for(int j = 0; j < H; j++) {
    for(int i = 0; i < W; i++) {
      (*X)(i,j) = image->data[d]   + 256.0*image->data[d+1]; 
      (*Y)(i,j) = image->data[d+2] + 256.0*image->data[d+3];
      (*Z)(i,j) = image->data[d+4] + 256.0*image->data[d+5];
      d += 6;
    }
  }

  std::cout << "Data size: " << image->data_size << " " << W*H*3*2 << std::endl;
  std::cout << "d: " << d << std::endl;
  std::cout << "W: " << image->width << " H: " << image->height << std::endl;


  if (writeOnDisk)   // for align_image_stack and thumbnails
  {
    QString fname(filename);
    QString tmpdir(tempdir);
    QFileInfo qfi(fname);
    QString outname = tmpdir + "/" + qfi.baseName() + ".tiff";

    RawProcessor.dcraw_ppm_tiff_writer(outname.toAscii().constData());

    if( (ret = RawProcessor.unpack_thumb() ) == LIBRAW_SUCCESS)
    {
      QString suffix = T.tformat == LIBRAW_THUMBNAIL_JPEG ? "thumb.jpg" : "thumb.ppm"; 
      QString thumbname = tmpdir + "/" + qfi.baseName() + "." + suffix;
      RawProcessor.dcraw_thumb_writer(thumbname.toAscii().constData());
    }
    std::cout << "Filename: " << filename << std::endl;
    std::cout << "Outname: " << outname.toAscii().constData() << std::endl;
  }

  LibRaw::dcraw_clear_mem(image);
  RawProcessor.recycle();
  return frame;
}
