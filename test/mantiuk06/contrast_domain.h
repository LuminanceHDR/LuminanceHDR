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
 * This file is a part of LuminanceHDR package, based on pfstmo.
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
 * @author Rafal Mantiuk, <mantiuk@gmail.com>
 * Updated 2007/12/17 by Ed Brambley <E.J.Brambley@damtp.cam.ac.uk>
 *
 * $Id: contrast_domain.h,v 1.7 2008/06/16 22:17:47 rafm Exp $
 */
#ifndef CONTRAST_DOMAIN_H
#define CONTRAST_DOMAIN_H

#include "TonemappingOperators/pfstmo.h"
#include "Common/ProgressHelper.h"

namespace test_mantiuk06
{
typedef struct pyramid_s {
  int rows;
  int cols;
  float* Gx;
  float* Gy;
  struct pyramid_s* next;
  struct pyramid_s* prev;
} pyramid_t;

void contrast_equalization(pyramid_t *pp, const float contrastFactor);

void transform_to_luminance(pyramid_t* pyramid, float* x, ProgressHelper *ph);
void matrix_subtract(const int n, const float* const a, float* const b);
void matrix_copy(const int n, const float* const a, float* const b);
void matrix_multiply_const(const int n, float* const a, const float val);
float* matrix_alloc(const int size);
void matrix_free(float* m);
float matrix_DotProduct(const int n, const float* const a, const float* const b);
void matrix_zero(const int n, float* const m);
void calculate_and_add_divergence(const int rows, const int cols, const float* const Gx, const float* const Gy, float* const divG);
void pyramid_calculate_divergence(pyramid_t* pyramid);
void pyramid_calculate_divergence_sum(pyramid_t* pyramid, float* divG_sum);
void calculate_scale_factor(const int n, const float* const G, float* const C);
void pyramid_calculate_scale_factor(const pyramid_t* pyramid, pyramid_t* pC);
void scale_gradient(const int n, float* G, const float* C);
void pyramid_scale_gradient(pyramid_t* pyramid, const pyramid_t* pC);
void pyramid_free(pyramid_t* pyramid);
pyramid_t* pyramid_allocate(const int cols, const int rows);
void calculate_gradient(const int cols, const int rows, const float* const lum, float* const Gx, float* const Gy);
void pyramid_calculate_gradient(pyramid_t* pyramid, const float* lum);
void solveX(const int n, const float* const b, float* const x);
void multiplyA(pyramid_t* px, pyramid_t* pyramid, const float* x, float* divG_sum);
void lincg(pyramid_t* pyramid, pyramid_t* pC, const float* const b, float* const x, const int itmax, const float tol, ProgressHelper *ph);
float lookup_table(const int n, const float* const in_tab, const float* const out_tab, const float val);
void transform_to_R(const int n, float* const G, float detail_factor);
void pyramid_transform_to_R(pyramid_t* pyramid, float detail_factor);
void transform_to_G(const int n, float* const R, float detail_factor);
void pyramid_transform_to_G(pyramid_t* pyramid, float detail_factor);
void pyramid_gradient_multiply(pyramid_t* pyramid, const float val);

void matrix_upsample(const int outCols, const int outRows, const float* const in, float* const out);
void matrix_downsample(const int inCols, const int inRows, const float* const data, float* const res);

/**
 * @brief: Tone mapping algorithm [Mantiuk2006]
 *
 * @param R red channel
 * @param G green channel
 * @param B blue channel
 * @param Y luminance channel
 * @param contrastFactor contrast scaling factor (in 0-1 range)
 * @param saturationFactor color desaturation (in 0-1 range)
 * @param itmax maximum number of iterations for convergence (typically 50)
 * @param tol tolerence to get within for convergence (typically 1e-3)
 * @param progress_cb callback function that reports progress
 * @return PFSTMO_OK if tone-mapping was sucessful, PFSTMO_ABORTED if
 * it was stopped from a callback function and PFSTMO_ERROR if an
 * error was encountered.
 */
int tmo_mantiuk06_contmap( int cols, int rows, float* R, float* G, float* B, float* Y,
                          float contrastFactor, float saturationFactor, float detailFactor,
                          int itmax = 200, float tol = 1e-3, ProgressHelper *ph = NULL);

}

#endif

