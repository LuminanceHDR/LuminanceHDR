/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Franco Comida
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

//! \brief FITS file format reader for 3 images rapresenting red, green and blue channels
//! \author Franco Comida <fcomida@users.sourceforge.net>

#ifndef PFS_IO_FITSREADER3CH_H
#define PFS_IO_FITSREADER3CH_H

#include <CCfits>
#include <boost/scoped_ptr.hpp>

#include <string>
//#include <Libpfs/params.h>
//#include <Libpfs/io/framereader.h>
#include <Libpfs/io/ioexception.h>
#include <Libpfs/utils/resourcehandlerstdio.h>

using namespace CCfits;

namespace pfs {
class Frame;

namespace io {

class FitsReader3Ch
{
public:
    FitsReader3Ch(const std::string& redChannel, const std::string& greenChannel, const std::string& blueChannel);

    bool isOpen() const
    { return m_fileRed.get(); }

    void open();
    void close();
    void read(Frame &frame);

private:
    std::string m_redChannel;
    std::string m_greenChannel;
    std::string m_blueChannel;
    boost::scoped_ptr<FITS> m_fileRed;
    boost::scoped_ptr<FITS> m_fileGreen;
    boost::scoped_ptr<FITS> m_fileBlue;
    PHDU *m_imageRed;
    PHDU *m_imageGreen;
    PHDU *m_imageBlue;
};

}   // io
}   // pfs

#endif
