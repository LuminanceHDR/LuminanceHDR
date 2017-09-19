/*
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Davide Anastasia
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

#ifndef RESOURCEHANDLERSTDIO_H
#define RESOURCEHANDLERSTDIO_H

//! \file resourcehandlerstdio.h
//! \brief This file contains resource handlers for cstdio
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \date 2012 05 05
//! \since 2.3.0-beta1

#include <Libpfs/utils/resourcehandler.h>
#include <cstdio>
#include <iostream>

namespace pfs {
namespace utils {

struct CleanUpStdIoFile {
    static inline void cleanup(FILE *p) {
        if (p) {
            fclose(p);
        }
    }
};

typedef ResourceHandler<FILE, CleanUpStdIoFile> ScopedStdIoFile;

}  // utils
}  // pfs

#endif  // RESOURCEHANDLERSTDIO_H
