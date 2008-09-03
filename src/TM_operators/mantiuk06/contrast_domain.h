/**
 * @brief Contrast mapping TMO
 *
 * From:
 * 
 * Rafal Mantiuk, Karol Myszkowski, Hans-Peter Seidel.
 * A Perceptual Framework for Contrast Processing of High Dynamic Range Images
 * In: ACM Transactions on Applied Perception 3 (3), pp. 286-308, 2006
 * http://www.mpi-inf.mpg.de/~mantiuk/contrast_domain/
 *
 * This file is a part of Qtpfsgui package (taken from the PFSTMO package).
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2007 Grzegorz Krawczyk
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
 * @author Radoslaw Mantiuk, <radoslaw.mantiuk@gmail.com>
 *
 * $Id: contrast_domain.h,v 1.3 2007/06/15 10:28:42 rafm Exp $
 */

#include "../../Libpfs/array2d.h"


/** Progress reporting callback function:
 *    void report_progress( int progress )
 *  'progress' parameter grows from 0 to 100
 */
typedef void(*progress_callback)(int progress);


/**
 * @brief: Tone mapping algorithm [Mantiuk2006]
 *
 * @param R red channel
 * @param G green channel
 * @param B blue channel
 * @param Y luminance channel
 * @param contrastFactor contrast scaling factor (in 0-1 range)
 * @param saturationFactor color desaturation (in 0-1 range)
 * @param bcg true if to use BiConjugate Gradients, false if to use Conjugate Gradients
 * @param itmax maximum number of iterations for convergence (typically 50)
 * @param tol tolerence to get within for convergence (typically 1e-3)
 * @param progress_cb callback function that reports progress
 */
void tmo_mantiuk06_contmap( int cols, int rows, float* R, float* G, float* B, float* Y,
			    float contrastFactor, float saturationFactor, float detailFactor, bool bcg,
			    int itmax = 200, float tol = 1e-3, progress_callback progress_cb  = NULL);
