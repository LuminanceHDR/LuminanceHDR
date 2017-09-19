/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
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

#ifndef PFS_IO_IOEXCEPTION_H
#define PFS_IO_IOEXCEPTION_H

#include <Libpfs/exception.h>

namespace pfs {
namespace io {

class InvalidFile : public Exception {
   public:
    InvalidFile(const std::string &message) : Exception(message) {}
};

class InvalidHeader : public Exception {
   public:
    InvalidHeader(const std::string &message) : Exception(message) {}
};

class ReadException : public Exception {
   public:
    ReadException(const std::string &message) : Exception(message) {}
};

class WriteException : public Exception {
   public:
    WriteException(const std::string &message) : Exception(message) {}
};

class UnsupportedFormat : public Exception {
   public:
    UnsupportedFormat(const std::string &message) : Exception(message) {}
};
}
}

#endif  // PFS_IO_IOEXCEPTION_H
