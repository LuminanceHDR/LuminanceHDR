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

#ifndef PFS_TIFFWRITER_H
#define PFS_TIFFWRITER_H

#include <QObject>
#include <QImage>
#include <tiffio.h>

namespace pfs
{
class Frame;
class Array2D;
}

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

#endif  // PFS_TIFFWRITER_H
