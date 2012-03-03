/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2010 Davide Anastasia
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
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 * This file contains path necessary to let Luminance work correctly in FreeBSD
 * Other architecture specific definition can be stored in this file afterwords.
 * From a suggestion of Joao Rocha Braga Filho <goffredo@gmail.com>, mantainer of
 * Luminance HDR for FreeBSD
 */

#ifdef __FreeBSD__
#define BASEDIR  "/usr/local/"
#else
#define BASEDIR "/usr/"
#endif

