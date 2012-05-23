/**
 * @brief Tiff facilities
 * 
 * This file is a part of LuminanceHDR package.
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
 * slightly modified by Giuseppe Rota <grota@sourceforge.net> for luminance
 */

#ifndef PFSTIFF_H
#define PFSTIFF_H

#include <QObject>
#include <QImage>
#include <tiffio.h>

#include "Libpfs/array2d.h"
#include "Libpfs/frame.h"

class TiffReader : public QObject
{
    Q_OBJECT

    TIFF *tif;

    uint32 width;
    uint32 height;

    uint16 comp;                  /// compression type
    uint16 phot;                  /// type of photometric data
    enum    //FLOAT is the wasting space one, FLOATLOGLUV is Greg Ward's format
    {
        FLOATLOGLUV,
        FLOAT,
        WORD,
        BYTE
    } TypeOfData;
    enum
    {
        RGB,
        CMYK
    } ColorSpace;
    uint16 bps;                   /// bits per sample
    uint16 nSamples;              /// number of channels in tiff file (only 1-3 are used)
    bool has_alpha;
    double stonits;               /// scale factor to get nit values

    bool writeOnDisk;
    QString fileName;
    QString tempFilesPath;

public:
    TiffReader( const char* filename, const char *tempfilespath, bool writeOnDisk );
    // ~TiffReader() {}

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

class TiffWriter : public QObject
{
    Q_OBJECT

private:
	TIFF *tif;

    QImage *ldrimage;
    const quint16 *pixmap;
    pfs::Frame* pfsFrame;
    uint32 width;
    uint32 height;

public:
    TiffWriter( const char* filename, pfs::Frame *f );
    TiffWriter( const char* filename, QImage *ldrimage );
    TiffWriter( const char* filename, const quint16 *pixmap, int w, int h);

    //! \brief write 8bit Tiff from QImage
    int write8bitTiff();
    //! \brief write 16bit Tiff from 16 bits pixmap
    int write16bitTiff();
    //! \brief write 32bit float Tiff from pfs::Frame
    int writeFloatTiff();
    //! \brief write LogLuv Tiff from pfs::Frame
    int writeLogLuvTiff();
    //! \brief write 16bit Tiff from pfs::Frame
    int writePFSFrame16bitTiff();

signals: //For ProgressDialog
    void maximumValue(int);
    void nextstep(int);
};

#endif
