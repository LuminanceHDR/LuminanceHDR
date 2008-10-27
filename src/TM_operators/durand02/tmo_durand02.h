/**
 * @file tmo_durand02.h
 * @brief Local tone mapping operator based on bilateral filtering.
 * Durand et al. 2002
 *
 * Fast Bilateral Filtering for the Display of High-Dynamic-Range Images.
 * F. Durand and J. Dorsey.
 * In ACM Transactions on Graphics, 2002.
 *
 * This file is a part of Qtpfsgui package, based on pfstmo.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Grzegorz Krawczyk
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
 * $Id: tmo_durand02.h,v 1.3 2008/09/09 00:56:49 rafm Exp $
 */

#ifndef _tmo_durand02_h_
#define _tmo_durand02_h_
#include "../../Libpfs/pfs.h"
#include "../pfstmo.h"

/*
 * @brief Fast bilateral filtering
 *
 * @param width image width
 * @param height image height
 * @param R red channel
 * @param G green channel
 * @param B blue channel
 * @param sigma_s sigma for spatial kernel 
 * @param sigma_r sigma for range kernel
 * @param baseContrast contrast of the base layer
 * @param downsample down sampling factor for speeding up fast-bilateral (1..20)
 */
void tmo_durand02(unsigned int width, unsigned int height,
  float *R, float *G, float *B,
  float sigma_s, float sigma_r, float baseContrast, int downsample/*,
  pfstmo_progress_callback progress_cb*/ );

pfs::Frame* pfstmo_durand02(pfs::Frame* inpfsframe, float _sigma_s, float _sigma_r, float _baseContrast);

#endif /* _tmo_durand02_h_ */
