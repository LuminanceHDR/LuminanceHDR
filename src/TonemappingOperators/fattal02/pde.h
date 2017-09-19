/**
 * @file pde.h
 * @brief Solving Partial Differential Equations
 *
 * Full Multigrid Algorithm and Successive Overrelaxation.
 *
 * @author Grzegorz Krawczyk
 *
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
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
 * $Id: pde.h,v 1.2 2008/09/04 12:46:49 julians37 Exp $
 */

#ifndef FMG_PDE_H
#define FMG_PDE_H

#include <Libpfs/array2d_fwd.h>

namespace pfs {
class Progress;
}

/**
 * @brief solve pde using full multrigrid algorithm
 *
 * @param F array with divergence
 * @param U [out] sollution
 */
void solve_pde_multigrid(pfs::Array2Df *F, pfs::Array2Df *U, pfs::Progress &ph);

/**
 * @brief solve poisson pde (Laplace U = F) using discrete cosine transform
 *
 * @param F array of the right hand side (contains div G in this example)
 * @param U [out] solution
 * @param adjust_bound, adjust boundary values of F to make pde solvable
 */
void solve_pde_fft(pfs::Array2Df &F, pfs::Array2Df &U, pfs::Progress &ph,
                   bool adjust_bound = false);

/**
 * @brief returns the residual error of the solution U, ie norm(Laplace U - F)
 *
 * @param F [in] right hand side
 * @param U [in] solution
 */
float residual_pde(pfs::Array2Df &U, pfs::Array2Df &F);

#endif
