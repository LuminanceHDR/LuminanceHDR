/**
 * @file tmo_reinhard02.h
 * @brief Tone map luminance channel using Reinhard02 model
 *
 * This file is a part of Qtpfsgui package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003-2007 Grzegorz Krawczyk
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
 *
 * $Id: tmo_reinhard02.h,v 1.2 2008/09/04 12:46:49 julians37 Exp $
 */

#ifndef _tmo_reinhard02_h_
#define _tmo_reinhard02_h_

/*
 * @brief Photographic tone-reproduction
 *
 * @param width image width
 * @param height image height
 * @param Y input luminance
 * @param L output tonemapped intensities
 * @param use_scales true: local version, false: global version of TMO
 * @param key maps log average luminance to this value (default: 0.18)
 * @param phi sharpening parameter (defaults to 1 - no sharpening)
 * @param num number of scales to use in computation (default: 8)
 * @param low size in pixels of smallest scale (should be kept at 1)
 * @param high size in pixels of largest scale (default 1.6^8 = 43)
 */
// extern "C" {
void tmo_reinhard02(unsigned int width, unsigned int height,
  const float *Y, float *L,
  bool use_scales, float key, float phi,
  int num, int low, int high, bool temporal_coherent );

pfs::Frame* pfstmo_reinhard02 (pfs::Frame* inpfsframe, float _key, float _phi, int _num, int _low, int _high, bool _use_scales );
// }
#endif /* _tmo_reinhard02_h_ */
