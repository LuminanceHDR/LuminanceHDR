/**
 * @brief Tiff facilities
 * 
 * This file is a part of Qtpfsgui package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2006 Giuseppe Rota
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 * slightly modified by Giuseppe Rota <grota@sourceforge.net> for qtpfsgui
 */

#ifndef PFSTIFF_H
#define PFSTIFF_H

#include <tiffio.h>
#include "../Libpfs/array2d.h"
#include "../Libpfs/pfs.h"
#include <QObject>
#include <QImage>


class TiffReader : public QObject {
Q_OBJECT

  TIFF* tif;
  uint32 width, height;

  uint16 comp;                  /// compression type
  uint16 phot;                  /// type of photometric data
  enum {FLOATLOGLUV, FLOAT, WORD, BYTE} TypeOfData; //FLOAT is the wasting space one, FLOATLOGLUV is Greg Ward's format
  uint16 bps;                   /// bits per sample
  uint16 nSamples;              /// number of channels in tiff file (only 1-3 are used)
  bool has_alpha;
  double stonits;               /// scale factor to get nit values

public:
  TiffReader( const char* filename );
  ~TiffReader() {}

  int getWidth() const { return width; }
  int getHeight() const { return height; }

  bool is8bitTiff() { return TypeOfData==BYTE; }
  bool is16bitTiff() { return TypeOfData==WORD; }
  bool is32bitTiff() { return TypeOfData==FLOAT; }
  bool isLogLuvTiff() { return (TypeOfData==FLOATLOGLUV); }

  pfs::Frame* readIntoPfsFrame(); //from 8,16,32,logluv TIFF to pfs::Frame
  QImage* readIntoQImage();

signals: //For ProgressDialog
  void maximumValue(int);
  void nextstep(int);
};


class TiffWriter : public QObject {
Q_OBJECT

private:
  TIFF* tif;
  pfs::Channel *R,*G,*B;
  QImage *ldrimage;
  uint32 width,height;
public:
  TiffWriter( const char* filename, pfs::Frame *f );
  TiffWriter( const char* filename, QImage *ldrimage );

  int write8bitTiff(); //write 8bit Tiff from QImage
//   int write16bitTiff(); //write 16bit Tiff from pfs::Frame
  int writeFloatTiff(); //write 32bit float Tiff from pfs::Frame
  int writeLogLuvTiff(); //write LogLuv Tiff from pfs::Frame
signals: //For ProgressDialog
  void maximumValue(int);
  void nextstep(int);
};

#endif
