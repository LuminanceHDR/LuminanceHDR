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

//! \brief Base class for C++ exceptions inside the library
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef PFS_EXCEPTION_H
#define PFS_EXCEPTION_H

#include <stdexcept>

namespace pfs {

/**
 * General exception class used to throw exceptions from pfs library.
 */
class Exception : public std::runtime_error {
   public:
    Exception(const std::string &message) : std::runtime_error(message) {}

    Exception(const char *message) : std::runtime_error(message) {}
};
}

#endif  // PFS_EXCEPTION_H
