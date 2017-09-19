/**
 * @file
 * @brief PFS library - core API interfaces
 *
 * Classes for reading and writing a stream of PFS frames.
 *
 * Note on the design of pfs library API: pfs library API makes
 * extensive usage of interfaces - classes that have only virtual
 * methods. This way no private fields are visible for the client
 * programs. Everything that is private is hidden in .cpp file rather
 * than the header .h. For example, pfs library uses STL to store some
 * internal data, but no STL class can be found the header file
 * pfs.h. Such design should hopefully give less problems when
 * extending and updating the library.
 *
 * This file is a part of PFSTOOLS package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#ifndef PFS_H
#define PFS_H

//#include <string>
//#include <Libpfs/exception.h>

//#define PFSEOL "\x0a"
//#define PFSEOLCH '\x0a'

//#define MAX_RES 65535
//#define MAX_CHANNEL_NAME 32
//#define MAX_TAG_STRING 1024
//#define MAX_CHANNEL_COUNT 1024

/**
 * All classes and function from PFS library reside in pfs namespace.
 */
namespace pfs {}

#endif
